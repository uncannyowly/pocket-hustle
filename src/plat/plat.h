/* Platform layer: one 640 x 480 screen, a button pad (keyboard on the PC) with an optional
 * mouse, and one stereo audio stream. plat_sdl.c is the PC backend; the Miyoo backend
 * (plat_miyoo.c) comes in Plan 3. Pixels are XRGB8888. */
#ifndef PH_PLAT_H
#define PH_PLAT_H

#include <stdint.h>

#include "ui/input.h"		/* PAD_* ids and PAD_BIT */

typedef struct { uint32_t *pixels; int w, h, pitch; } Framebuffer;	/* 640 x 480 XRGB8888; pitch in pixels */
enum { PLAT_MAX_CLICKS = 8 };
typedef struct {
	uint32_t held;
	int nclicks;				/* left clicks since the last poll, in arrival order */
	struct { int x, y; } click[PLAT_MAX_CLICKS];	/* framebuffer pixels */
	int quit;
} PlatEvents;
typedef void (*AudioCallback)(int16_t *stereo, int frames, void *user);

int  plat_init(int fullscreen);
void plat_shutdown(void);
Framebuffer *plat_framebuffer(void);
void plat_present(void);
/* Drains the event queue. held is the pad buttons down now (a press and release within one
 * poll still shows as held once). click[0..nclicks) are the left clicks, in order. At most PLAT_MAX_CLICKS are
 * returned per poll; polling stops there and the rest stay queued for the next poll. */
void plat_poll(PlatEvents *ev);
uint32_t plat_ms(void);
void plat_sleep_ms(int ms);

/* Opens a stereo int16 stream; cb runs on the audio thread, so wrap shared state in
 * plat_audio_lock/unlock. Returns 0, or -1 when there is no audio (the game goes on silent). */
int  plat_audio_open(int rate, AudioCallback cb, void *user);
void plat_audio_lock(void);
void plat_audio_unlock(void);
void plat_audio_close(void);

#endif
