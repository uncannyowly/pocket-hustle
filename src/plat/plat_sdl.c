/* PC backend: SDL2 window, streaming texture, keyboard, mouse and an audio callback. */
#include "plat/plat.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { W = 640, H = 480 };

static Framebuffer fb;
static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Texture *tex;
static SDL_AudioDeviceID audio_dev;
static int is_fullscreen;
static uint8_t keys[SDL_NUM_SCANCODES];	/* 1 while down */
static int tab_pad = PAD_R1;			/* what the Tab key was when it went down */

static void set_fullscreen(int on)
{
	is_fullscreen = on;
	SDL_SetWindowFullscreen(win, on ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

int plat_init(int fullscreen)
{
	fb.w = W;
	fb.h = H;
	fb.pitch = W;
	fb.pixels = calloc((size_t)W * H, 4);
	if (!fb.pixels)
		return -1;
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0 && SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL: %s\n", SDL_GetError());
		return -1;
	}
	win = SDL_CreateWindow("Pocket Hustle", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			       W * 2, H * 2, SDL_WINDOW_RESIZABLE);
	if (!win) {
		fprintf(stderr, "SDL: %s\n", SDL_GetError());
		return -1;
	}
	SDL_SetWindowMinimumSize(win, W, H);
	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!ren)
		ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC);
	if (!ren)
		ren = SDL_CreateRenderer(win, -1, 0);
	if (!ren) {
		fprintf(stderr, "SDL: %s\n", SDL_GetError());
		return -1;
	}
	SDL_RenderSetLogicalSize(ren, W, H);
	SDL_RenderSetIntegerScale(ren, SDL_TRUE);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, W, H);
	if (!tex) {
		fprintf(stderr, "SDL: %s\n", SDL_GetError());
		return -1;
	}
	SDL_ShowCursor(SDL_ENABLE);
	if (fullscreen)
		set_fullscreen(1);
	return 0;
}

void plat_shutdown(void)
{
	plat_audio_close();
	if (tex)
		SDL_DestroyTexture(tex);
	if (ren)
		SDL_DestroyRenderer(ren);
	if (win)
		SDL_DestroyWindow(win);
	tex = NULL;
	ren = NULL;
	win = NULL;
	SDL_Quit();
	free(fb.pixels);
	fb.pixels = NULL;
}

Framebuffer *plat_framebuffer(void)
{
	return &fb;
}

void plat_present(void)
{
	SDL_UpdateTexture(tex, NULL, fb.pixels, fb.pitch * 4);
	SDL_RenderClear(ren);
	SDL_RenderCopy(ren, tex, NULL, NULL);
	SDL_RenderPresent(ren);
}

/* The pad button a key stands for, or -1. Tab is handled apart (R1, or L1 with Shift). */
static int key_pad(SDL_Scancode s)
{
	switch (s) {
	case SDL_SCANCODE_UP: return PAD_UP;
	case SDL_SCANCODE_DOWN: return PAD_DOWN;
	case SDL_SCANCODE_LEFT: return PAD_LEFT;
	case SDL_SCANCODE_RIGHT: return PAD_RIGHT;
	case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: case SDL_SCANCODE_SPACE: return PAD_A;
	case SDL_SCANCODE_ESCAPE: case SDL_SCANCODE_BACKSPACE: return PAD_B;
	case SDL_SCANCODE_RIGHTBRACKET: return PAD_R1;
	case SDL_SCANCODE_LEFTBRACKET: return PAD_L1;
	case SDL_SCANCODE_Y: return PAD_Y;
	case SDL_SCANCODE_X: return PAD_X;
	case SDL_SCANCODE_C: return PAD_SELECT;
	case SDL_SCANCODE_M: return PAD_START;
	default: return -1;
	}
}

static uint32_t held_now(void)
{
	uint32_t held = 0;
	int s;

	for (s = 0; s < SDL_NUM_SCANCODES; s++)
		if (keys[s] && key_pad((SDL_Scancode)s) >= 0)
			held |= PAD_BIT(key_pad((SDL_Scancode)s));
	if (keys[SDL_SCANCODE_TAB])
		held |= PAD_BIT(tab_pad);
	return held;
}

void plat_poll(PlatEvents *ev)
{
	SDL_Event e;
	uint32_t tapped = 0;	/* pressed during this poll, so a quick tap is not lost */

	ev->clicked = 0;
	ev->quit = 0;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			ev->quit = 1;
			break;
		case SDL_KEYDOWN: {
			SDL_Scancode s = e.key.keysym.scancode;
			int alt = (e.key.keysym.mod & KMOD_ALT) != 0;

			if (e.key.repeat)	/* input_step does the repeating */
				break;
			if (s == SDL_SCANCODE_F11 || (alt && (s == SDL_SCANCODE_RETURN || s == SDL_SCANCODE_KP_ENTER))) {
				set_fullscreen(!is_fullscreen);
				break;
			}
			if (s >= SDL_NUM_SCANCODES)
				break;
			if (s == SDL_SCANCODE_TAB) {
				tab_pad = (e.key.keysym.mod & KMOD_SHIFT) ? PAD_L1 : PAD_R1;
				tapped |= PAD_BIT(tab_pad);
			} else if (key_pad(s) >= 0)
				tapped |= PAD_BIT(key_pad(s));
			keys[s] = 1;
			break;
		}
		case SDL_KEYUP:
			if (e.key.keysym.scancode < SDL_NUM_SCANCODES)
				keys[e.key.keysym.scancode] = 0;
			break;
		case SDL_WINDOWEVENT:
			if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
				memset(keys, 0, sizeof keys);
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (e.button.button == SDL_BUTTON_LEFT) {
				float lx, ly;
				int cx, cy;

				SDL_RenderWindowToLogical(ren, e.button.x, e.button.y, &lx, &ly);
				cx = (int)lx;
				cy = (int)ly;
				if (cx >= 0 && cx < W && cy >= 0 && cy < H) {	/* not the letterbox bars */
					ev->clicked = 1;
					ev->cx = cx;
					ev->cy = cy;
				}
			}
			break;
		default:
			break;
		}
	}
	ev->held = held_now() | tapped;
}

uint32_t plat_ms(void)
{
	return SDL_GetTicks();
}

void plat_sleep_ms(int ms)
{
	SDL_Delay((Uint32)ms);
}

static AudioCallback audio_cb;
static void *audio_user;

static void sdl_audio(void *unused, Uint8 *stream, int bytes)
{
	(void)unused;
	audio_cb((int16_t *)stream, bytes / 4, audio_user);
}

int plat_audio_open(int rate, AudioCallback cb, void *user)
{
	SDL_AudioSpec want, have;

	if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
		return -1;
	SDL_zero(want);
	want.freq = rate;
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = 1024;
	want.callback = sdl_audio;
	audio_cb = cb;
	audio_user = user;
	audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);	/* 0: SDL converts to what we ask for */
	if (!audio_dev)
		return -1;
	SDL_PauseAudioDevice(audio_dev, 0);
	return 0;
}

void plat_audio_lock(void) { if (audio_dev) SDL_LockAudioDevice(audio_dev); }
void plat_audio_unlock(void) { if (audio_dev) SDL_UnlockAudioDevice(audio_dev); }
void plat_audio_close(void)
{
	if (audio_dev)
		SDL_CloseAudioDevice(audio_dev);
	audio_dev = 0;
}
