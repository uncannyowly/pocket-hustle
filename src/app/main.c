/* pocket-hustle for the PC: the screens of src/ui on the SDL2 platform of src/plat.
 *   pocket-hustle [--data DIR] [--seed N] [--fullscreen] [--mute] [--frames N --keys "TOKENS"] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "gfx/canvas.h"
#include "plat/plat.h"
#include "sfx/mixer.h"
#include "ui/app.h"
#include "ui/input.h"
#include "ui/ui.h"

enum { TICK_MS = 50, MAX_CATCHUP = 5, SCRIPT_EVERY = 3, MAX_KEYS = 512 };

static Mixer mixer;

static void audio_cb(int16_t *stereo, int frames, void *user)
{
	(void)user;
	mixer_fill(&mixer, stereo, frames);
}

static void usage(void)
{
	fprintf(stderr, "usage: pocket-hustle [--data DIR] [--seed N] [--fullscreen] [--mute] "
			"[--frames N --keys \"TOKENS\"]\n");
}

/* "A D START" to pad ids; the count, or -1 on an unknown token. */
static int parse_keys(const char *s, int *pads)
{
	char tok[16];
	int n = 0;

	for (;;) {
		size_t len;

		while (*s == ' ')
			s++;
		if (!*s)
			return n;
		for (len = 0; s[len] && s[len] != ' '; len++)
			;
		if (len >= sizeof tok || n >= MAX_KEYS)
			return -1;
		memcpy(tok, s, len);
		tok[len] = '\0';
		if ((pads[n] = input_token(tok)) < 0) {
			fprintf(stderr, "pocket-hustle: unknown key token \"%s\"\n", tok);
			return -1;
		}
		n++;
		s += len;
	}
}

int main(int argc, char **argv)
{
	static App app;
	static Ui ui;
	static Canvas cv;
	char dir[512] = "";
	const char *data = NULL;
	int fullscreen = 0, mute = 0, scripted = 0, nkeys = 0, pads[MAX_KEYS];
	long frames = 0, frame = 0;
	uint64_t seed = (uint64_t)time(NULL);
	int i, audio = 0, redraw = 1, quit = 0;
	uint32_t last_tick, now;
	Repeat rep;
	PlatEvents ev;
	Framebuffer *fb;

	memset(&rep, 0, sizeof rep);
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--data") && i + 1 < argc)
			data = argv[++i];
		else if (!strcmp(argv[i], "--seed") && i + 1 < argc)
			seed = strtoull(argv[++i], NULL, 10);
		else if (!strcmp(argv[i], "--fullscreen"))
			fullscreen = 1;
		else if (!strcmp(argv[i], "--mute"))
			mute = 1;
		else if (!strcmp(argv[i], "--frames") && i + 1 < argc)
			frames = atol(argv[++i]), scripted = 1;
		else if (!strcmp(argv[i], "--keys") && i + 1 < argc) {
			if ((nkeys = parse_keys(argv[++i], pads)) < 0)
				return 2;
		} else {
			usage();
			return 2;
		}
	}
	if (nkeys && !scripted) {
		usage();
		return 2;
	}

	if (!data) {
		if (app_default_dir(dir, sizeof dir) == 0)
			data = dir;
		else
			fprintf(stderr, "pocket-hustle: no data directory (no HOME); nothing will be saved\n");
	}
	if (app_init(&app, data) != 0)
		fprintf(stderr, "pocket-hustle: cannot use the data directory \"%s\"; nothing will be saved\n",
			data ? data : "");
	app_load(&app);
	app.next_seed = seed;
	ui_init(&ui, &app);

	if (plat_init(fullscreen) != 0)
		return 1;
	fb = plat_framebuffer();
	if (mixer_init(&mixer) == 0)
		audio = plat_audio_open(MIX_RATE, audio_cb, NULL) == 0;
	else
		fprintf(stderr, "pocket-hustle: no sound effects\n");

	now = plat_ms();
	last_tick = now;
	while (!quit) {
		uint32_t fire;
		int pressed = 0, b;

		plat_poll(&ev);
		if (ev.quit)
			break;
		if (scripted) {	/* a virtual clock: one tick per frame, one press every few frames */
			now = (uint32_t)frame * TICK_MS;
			ev.held = 0;
			ev.clicked = 0;
			if (frame % SCRIPT_EVERY == 0 && frame / SCRIPT_EVERY < nkeys)
				ev.held = PAD_BIT(pads[frame / SCRIPT_EVERY]);
		} else
			now = plat_ms();

		fire = input_step(&rep, ev.held, now);
		for (b = 0; b < PAD_COUNT; b++)
			if (fire & PAD_BIT(b)) {
				ui_press(&ui, b);
				pressed = 1;
			}
		if (ev.clicked) {
			if (pressed)	/* a press voids the hit regions: redraw so the click lands */
				ui_draw(&ui, &cv);
			ui_click(&ui, ev.cx / 2, ev.cy / 2);
			pressed = 1;
		}
		if (pressed)
			redraw = 1;

		for (i = 0; (int32_t)(now - last_tick) >= TICK_MS && i < MAX_CATCHUP; i++) {
			ui_tick(&ui);
			last_tick += TICK_MS;
			redraw = 1;
		}
		if ((int32_t)(now - last_tick) >= TICK_MS)	/* fell far behind: do not replay it all */
			last_tick = now;

		if (audio) {
			int id;

			plat_audio_lock();
			mixer.muted = mute || !app.sound_on;
			while ((id = ui_take_sfx(&ui)) >= 0)
				mixer_play(&mixer, (SfxId)id);
			plat_audio_unlock();
		} else
			while (ui_take_sfx(&ui) >= 0)
				;

		if (ui_quit(&ui))
			quit = 1;
		if (redraw) {
			ui_draw(&ui, &cv);
			cv_scale2x(&cv, fb->pixels, fb->pitch);
			plat_present();
			redraw = 0;
		} else
			plat_sleep_ms(2);

		frame++;
		if (scripted && frame >= frames)
			break;
	}

	if (app.has_game && app_save_now(&app) != 0)
		fprintf(stderr, "pocket-hustle: the game could not be saved\n");
	plat_audio_close();
	mixer_free(&mixer);
	plat_shutdown();
	return 0;
}
