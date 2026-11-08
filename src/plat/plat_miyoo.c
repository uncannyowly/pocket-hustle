/*
 * Miyoo Mini backend, adapted from the Scorched Earth port's platform_miyoo.c (same author, MIT).
 *
 * Video: the 640 x 480 picture is drawn into a heap buffer, then centered and rotated by 180
 * degrees (the panel is mounted upside down) into the hidden page of the double buffered
 * /dev/fb0, and the display is panned to it. The panel's own size is used, so a Mini with a
 * 752 x 560 panel gets the picture centered with black margins.
 *
 * Buttons: read straight from evdev (/dev/input/event0), not grabbed, so OnionOS keeps its
 * POWER and volume handling. Holding MENU for two seconds sets PlatEvents.quit.
 *
 * Audio: the SigmaStar MI_AO driver, loaded with dlopen (the launcher stops OnionOS's
 * audioserver first), at OnionOS's volume and mute. Without it the game plays silent.
 *
 * Environment (for emulated tests):
 *   PH_FAKEFB=WxH:file   a stand-in framebuffer file instead of /dev/fb0
 *   PH_INPUT=dev[:dev]   evdev devices (default /dev/input/event0)
 */
#include "plat/plat.h"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <mi_ao.h>
#include <mi_sys.h>

#include "gfx/panel.h"
#include "ui/hold.h"

enum { W = 640, H = 480, MAX_INPUTS = 4 };

static Framebuffer fb;
static int fb_fd = -1;
static uint8_t *fb_mem;
static size_t fb_size;
static struct fb_var_screeninfo var, orig_var;
static struct fb_fix_screeninfo fix;
static int have_orig;
static int pages = 1, page, fake;

static int in_fd[MAX_INPUTS], in_count;
static uint32_t held_now, tapped;	/* tapped: pressed since the last poll, so a quick tap is not lost */
static Hold menu_hold;
static volatile sig_atomic_t sig_quit;

static void on_signal(int sig)
{
	(void)sig;
	sig_quit = 1;
}

static int open_fake(const char *spec)
{
	const char *path = strchr(spec, ':');
	unsigned w, h;

	if (path == NULL || sscanf(spec, "%ux%u", &w, &h) != 2)
		return -1;
	fb_fd = open(path + 1, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fb_fd < 0)
		return -1;
	memset(&var, 0, sizeof var);
	memset(&fix, 0, sizeof fix);
	var.xres = var.xres_virtual = w;
	var.yres = var.yres_virtual = h;
	var.bits_per_pixel = 32;
	fix.line_length = w * 4;
	fix.smem_len = w * h * 4;
	if (ftruncate(fb_fd, fix.smem_len) != 0)
		return -1;
	fake = 1;
	return 0;
}

static int open_fb(void)
{
	const char *fakespec = getenv("PH_FAKEFB");

	if (fakespec != NULL && fakespec[0] != 0)
		return open_fake(fakespec);

	fb_fd = open("/dev/fb0", O_RDWR);
	if (fb_fd < 0) {
		fprintf(stderr, "fb0: %s\n", strerror(errno));
		return -1;
	}
	if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &var) < 0)
		return -1;
	orig_var = var;
	have_orig = 1;

	/* ask for 32bpp with two pages to flip between */
	var.bits_per_pixel = 32;
	var.yres_virtual = var.yres * 2;
	var.yoffset = 0;
	if (ioctl(fb_fd, FBIOPUT_VSCREENINFO, &var) < 0) {
		var = orig_var;
		ioctl(fb_fd, FBIOPUT_VSCREENINFO, &var);
	}
	if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &var) < 0 ||
	    ioctl(fb_fd, FBIOGET_FSCREENINFO, &fix) < 0)
		return -1;
	if (var.bits_per_pixel != 32) {
		fprintf(stderr, "fb0: unsupported depth %u\n", var.bits_per_pixel);
		return -1;
	}
	return 0;
}

static void open_inputs(void)
{
	const char *env = getenv("PH_INPUT");
	char *list = strdup(env != NULL && env[0] ? env : "/dev/input/event0");
	char *save = NULL, *dev;

	if (list == NULL)
		return;
	for (dev = strtok_r(list, ":", &save); dev != NULL && in_count < MAX_INPUTS;
	     dev = strtok_r(NULL, ":", &save)) {
		int fd = open(dev, O_RDONLY | O_NONBLOCK);

		if (fd >= 0)
			in_fd[in_count++] = fd;
		else
			fprintf(stderr, "input %s: %s\n", dev, strerror(errno));
	}
	free(list);
}

int plat_init(int fullscreen)
{
	struct sigaction sa;

	(void)fullscreen;
	if (open_fb() != 0)
		goto fail;
	fb_size = fix.smem_len;
	fb_mem = mmap(NULL, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
	if (fb_mem == MAP_FAILED) {
		fb_mem = NULL;
		goto fail;
	}
	pages = (!fake && var.yres_virtual >= var.yres * 2 &&
		 fb_size >= (size_t)fix.line_length * var.yres * 2) ? 2 : 1;

	fb.w = W;
	fb.h = H;
	fb.pitch = W;
	fb.pixels = calloc((size_t)W * H, 4);
	if (fb.pixels == NULL)
		goto fail;
	open_inputs();
	hold_init(&menu_hold);

	memset(&sa, 0, sizeof sa);
	sa.sa_handler = on_signal;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	return 0;
fail:
	plat_shutdown();
	return -1;
}

void plat_shutdown(void)
{
	int i;

	plat_audio_close();
	if (fb_mem != NULL) {
		if (!fake) {	/* leave the panel blank on page 0 in the original mode */
			memset(fb_mem, 0, fb_size);
			var.yoffset = 0;
			ioctl(fb_fd, FBIOPAN_DISPLAY, &var);
			if (have_orig)
				ioctl(fb_fd, FBIOPUT_VSCREENINFO, &orig_var);
		}
		munmap(fb_mem, fb_size);
		fb_mem = NULL;
	}
	if (fb_fd >= 0)
		close(fb_fd);
	fb_fd = -1;
	for (i = 0; i < in_count; i++)
		close(in_fd[i]);
	in_count = 0;
	free(fb.pixels);
	fb.pixels = NULL;
}

Framebuffer *plat_framebuffer(void)
{
	return &fb;
}

void plat_present(void)
{
	int target = pages == 2 ? page ^ 1 : 0;

	if (fb_mem == NULL || fb.pixels == NULL)
		return;
	panel_blit_rot180((uint32_t *)(fb_mem + (size_t)target * var.yres * fix.line_length),
			  (int)(fix.line_length / 4), (int)var.xres, (int)var.yres,
			  fb.pixels, fb.w, fb.h, fb.pitch);
	if (pages == 2) {
		var.yoffset = (uint32_t)target * var.yres;
		ioctl(fb_fd, FBIOPAN_DISPLAY, &var);
		page = target;
	}
}

static int map_key(int code)
{
	switch (code) {
	case KEY_UP: return PAD_UP;
	case KEY_DOWN: return PAD_DOWN;
	case KEY_LEFT: return PAD_LEFT;
	case KEY_RIGHT: return PAD_RIGHT;
	case KEY_SPACE: return PAD_A;
	case KEY_LEFTCTRL: return PAD_B;
	case KEY_LEFTSHIFT: return PAD_X;
	case KEY_LEFTALT: return PAD_Y;
	case KEY_E: return PAD_L1;
	case KEY_T: return PAD_R1;
	case KEY_TAB: return PAD_L2;
	case KEY_BACKSPACE: return PAD_R2;
	case KEY_RIGHTCTRL: return PAD_SELECT;
	case KEY_ENTER: return PAD_START;
	case KEY_ESC: return PAD_MENU;
	default: return -1;
	}
}

void plat_poll(PlatEvents *ev)
{
	struct input_event buf[16];
	int i;

	for (i = 0; i < in_count; i++) {
		ssize_t n;

		while ((n = read(in_fd[i], buf, sizeof buf)) > 0) {
			int k, count = (int)(n / (ssize_t)sizeof buf[0]);

			for (k = 0; k < count; k++) {
				int b;

				if (buf[k].type != EV_KEY || (b = map_key(buf[k].code)) < 0)
					continue;
				if (buf[k].value == 0)
					held_now &= ~PAD_BIT(b);
				else {	/* 1 press, 2 autorepeat */
					held_now |= PAD_BIT(b);
					tapped |= PAD_BIT(b);
				}
			}
		}
	}
	ev->held = held_now | tapped;
	tapped = 0;
	ev->nclicks = 0;
	ev->text[0] = '\0';
	ev->quit = sig_quit || hold_step(&menu_hold, (held_now & PAD_BIT(PAD_MENU)) != 0, plat_ms());
}

void plat_text_input(int on)
{
	(void)on;
}

uint32_t plat_ms(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint32_t)((uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u);
}

void plat_sleep_ms(int ms)
{
	struct timespec ts;

	if (ms <= 0)
		return;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (long)(ms % 1000) * 1000000L;
	nanosleep(&ts, NULL);
}

/* ---- audio: the SigmaStar MI_AO driver, fed by a mixer thread ---- */

#define AO_DEV 0
#define AO_CHN 0
#define AO_FRAMES 512

static AudioCallback audio_cb;
static void *audio_user;
static pthread_mutex_t audio_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *ao_lib, *sys_lib;
static pthread_t ao_thread;
static volatile int ao_running;
static int ao_open;
static MI_S32 (*p_MI_SYS_Init)(void);
static MI_S32 (*p_MI_AO_SetPubAttr)(MI_AUDIO_DEV, MI_AUDIO_Attr_t *);
static MI_S32 (*p_MI_AO_Enable)(MI_AUDIO_DEV);
static MI_S32 (*p_MI_AO_Disable)(MI_AUDIO_DEV);
static MI_S32 (*p_MI_AO_EnableChn)(MI_AUDIO_DEV, MI_AO_CHN);
static MI_S32 (*p_MI_AO_DisableChn)(MI_AUDIO_DEV, MI_AO_CHN);
static MI_S32 (*p_MI_AO_SendFrame)(MI_AUDIO_DEV, MI_AO_CHN, MI_AUDIO_Frame_t *, MI_S32);
static MI_S32 (*p_MI_AO_SetVolume)(MI_AUDIO_DEV, MI_S32);
static MI_S32 (*p_MI_AO_SetMute)(MI_AUDIO_DEV, MI_BOOL);

/* Onion's logarithmic volume curve (src/common/system/volume.h in OnionUI) */
static int volume_db(int vol)
{
	if (vol < 0)
		vol = 0;
	if (vol > 20)
		vol = 20;
	return vol == 0 ? -60 : (int)lround(48.0 * log10(1.0 + vol)) - 60;
}

/* Onion keeps the volume (0..20) on the card; /appconfigs is the firmware's */
static int system_volume_db(void)
{
	char buf[2048];
	FILE *f = fopen("/mnt/SDCARD/system.json", "r");
	int vol = 10;

	if (f == NULL)
		f = fopen("/appconfigs/system.json", "r");
	if (f != NULL) {
		size_t n = fread(buf, 1, sizeof buf - 1, f);
		char *v;

		buf[n] = 0;
		fclose(f);
		v = strstr(buf, "\"vol\"");
		if (v != NULL && (v = strchr(v, ':')) != NULL)
			vol = atoi(v + 1);
	}
	return volume_db(vol);
}

static void *ao_loop(void *arg)
{
	static int16_t buf[AO_FRAMES * 2];
	MI_AUDIO_Frame_t frame;

	(void)arg;
	while (ao_running) {
		pthread_mutex_lock(&audio_mutex);
		audio_cb(buf, AO_FRAMES, audio_user);
		pthread_mutex_unlock(&audio_mutex);
		memset(&frame, 0, sizeof frame);
		frame.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
		frame.eSoundmode = E_MI_AUDIO_SOUND_MODE_STEREO;
		frame.u32Len = sizeof buf;
		frame.apVirAddr[0] = buf;
		/* blocks while the driver's queue is full, which paces the thread */
		while (ao_running && p_MI_AO_SendFrame(AO_DEV, AO_CHN, &frame, 100) != 0)
			plat_sleep_ms(2);
	}
	return NULL;
}

static void *load_lib(const char *name)
{
	char path[128];
	void *h = dlopen(name, RTLD_NOW | RTLD_GLOBAL);

	if (h == NULL) {
		snprintf(path, sizeof path, "/config/lib/%s", name);
		h = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
	}
	return h;
}

int plat_audio_open(int rate, AudioCallback cb, void *user)
{
	MI_AUDIO_Attr_t attr;
	MI_S32 vol, e1, e2, e3;
	int mute;

	if (fake)
		return -1;	/* emulated tests have no audio hardware */
	audio_cb = cb;
	audio_user = user;

	/* libmi_ao.so uses symbols of SigmaStar's OS wrapper without linking it */
	load_lib("libcam_os_wrapper.so");
	load_lib("libmi_common.so");
	sys_lib = load_lib("libmi_sys.so");
	ao_lib = load_lib("libmi_ao.so");
	if (sys_lib == NULL || ao_lib == NULL) {
		fprintf(stderr, "audio: MI_AO unavailable: %s\n", dlerror());
		return -1;
	}
	p_MI_SYS_Init = (MI_S32 (*)(void))dlsym(sys_lib, "MI_SYS_Init");
	p_MI_AO_SetPubAttr = (MI_S32 (*)(MI_AUDIO_DEV, MI_AUDIO_Attr_t *))dlsym(ao_lib, "MI_AO_SetPubAttr");
	p_MI_AO_Enable = (MI_S32 (*)(MI_AUDIO_DEV))dlsym(ao_lib, "MI_AO_Enable");
	p_MI_AO_Disable = (MI_S32 (*)(MI_AUDIO_DEV))dlsym(ao_lib, "MI_AO_Disable");
	p_MI_AO_EnableChn = (MI_S32 (*)(MI_AUDIO_DEV, MI_AO_CHN))dlsym(ao_lib, "MI_AO_EnableChn");
	p_MI_AO_DisableChn = (MI_S32 (*)(MI_AUDIO_DEV, MI_AO_CHN))dlsym(ao_lib, "MI_AO_DisableChn");
	p_MI_AO_SendFrame = (MI_S32 (*)(MI_AUDIO_DEV, MI_AO_CHN, MI_AUDIO_Frame_t *, MI_S32))dlsym(ao_lib, "MI_AO_SendFrame");
	p_MI_AO_SetVolume = (MI_S32 (*)(MI_AUDIO_DEV, MI_S32))dlsym(ao_lib, "MI_AO_SetVolume");
	p_MI_AO_SetMute = (MI_S32 (*)(MI_AUDIO_DEV, MI_BOOL))dlsym(ao_lib, "MI_AO_SetMute");
	if (!p_MI_SYS_Init || !p_MI_AO_SetPubAttr || !p_MI_AO_Enable || !p_MI_AO_EnableChn ||
	    !p_MI_AO_SendFrame) {
		fprintf(stderr, "audio: MI_AO library lacks the functions it needs\n");
		return -1;
	}

	p_MI_SYS_Init();
	memset(&attr, 0, sizeof attr);
	attr.eSamplerate = (MI_AUDIO_SampleRate_e)rate;
	attr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
	attr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
	attr.eSoundmode = E_MI_AUDIO_SOUND_MODE_STEREO;
	attr.u32FrmNum = 6;
	attr.u32PtNumPerFrm = AO_FRAMES;
	attr.u32ChnCnt = 2;
	e1 = p_MI_AO_SetPubAttr(AO_DEV, &attr);
	e2 = e1 == 0 ? p_MI_AO_Enable(AO_DEV) : -1;
	e3 = e2 == 0 ? p_MI_AO_EnableChn(AO_DEV, AO_CHN) : -1;
	if (e1 != 0 || e2 != 0 || e3 != 0) {
		/* most often the audioserver still owns the device */
		fprintf(stderr, "audio: MI_AO setup failed (attr %#x, enable %#x, channel %#x)\n",
			(unsigned)e1, (unsigned)e2, (unsigned)e3);
		return -1;
	}
	vol = system_volume_db();
	mute = vol <= -60 || access("/mnt/SDCARD/.tmp_update/config/.muteVolume", F_OK) == 0;
	if (p_MI_AO_SetVolume)
		p_MI_AO_SetVolume(AO_DEV, vol);
	if (p_MI_AO_SetMute)
		p_MI_AO_SetMute(AO_DEV, mute ? TRUE : FALSE);
	ao_open = 1;
	ao_running = 1;
	if (pthread_create(&ao_thread, NULL, ao_loop, NULL) != 0) {
		ao_running = 0;
		plat_audio_close();
		return -1;
	}
	fprintf(stderr, "audio: MI_AO at %d Hz, volume %d dB%s\n", rate, (int)vol, mute ? ", muted" : "");
	return 0;
}

void plat_audio_lock(void)
{
	pthread_mutex_lock(&audio_mutex);
}

void plat_audio_unlock(void)
{
	pthread_mutex_unlock(&audio_mutex);
}

void plat_audio_close(void)
{
	if (!ao_open)
		return;
	if (ao_running) {
		ao_running = 0;
		pthread_join(ao_thread, NULL);
	}
	if (p_MI_AO_DisableChn)
		p_MI_AO_DisableChn(AO_DEV, AO_CHN);
	if (p_MI_AO_Disable)
		p_MI_AO_Disable(AO_DEV);
	ao_open = 0;
}
