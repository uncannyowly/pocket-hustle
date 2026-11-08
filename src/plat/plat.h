/* Platform layer: one 640 x 480 screen, a button pad (keyboard on the PC) with an optional
 * mouse, and one stereo audio stream. plat_sdl.c is the PC backend; the Miyoo backend
 * (plat_miyoo.c) comes in Plan 3. Pixels are XRGB8888. */
#ifndef PH_PLAT_H
#define PH_PLAT_H

#include <stdint.h>

#include "ui/input.h"		/* PAD_* ids and PAD_BIT */

typedef struct { uint32_t *pixels; int w, h, pitch; } Framebuffer;	/* 640 x 480 XRGB8888; pitch in pixels */
typedef struct { uint32_t held; int clicked, cx, cy; int quit; } PlatEvents;	/* cx, cy in framebuffer pixels */
typedef void (*AudioCallback)(int16_t *stereo, int frames, void *user);

int  plat_init(int fullscreen);
void plat_shutdown(void);
Framebuffer *plat_framebuffer(void);
void plat_present(void);
/* Drains the event queue. held is the pad buttons down now (a press and release within one
 * poll still shows as held once). clicked is a left click since the last poll, at (cx, cy). */
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
