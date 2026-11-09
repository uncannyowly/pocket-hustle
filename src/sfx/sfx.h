/* Parametric sound effects: a small sfxr-style synth. Floats are fine here. */
#ifndef PH_SFX_H
#define PH_SFX_H

#include <stdint.h>

/* WIN is the fanfare for a big payoff (a big sale, getting away from the cops); FAIL is the
 * sting for a real setback (jail, a ban, a mugging). */
typedef enum { SFX_MOVE, SFX_OK, SFX_BACK, SFX_DENY, SFX_CASH, SFX_HURT, SFX_SIREN, SFX_NEWS,
	       SFX_WIN, SFX_FAIL, NUM_SFX } SfxId;
typedef enum { WAVE_SQUARE, WAVE_SAW, WAVE_SINE, WAVE_NOISE } Wave;

typedef struct {
	Wave wave;
	float freq, slide, slide_accel;		/* Hz, Hz per second, Hz per second squared */
	float duty, duty_sweep;			/* square only, 0..1 */
	float vib_depth, vib_speed;		/* fraction of freq, Hz */
	float arp_mult[3], arp_time[3];		/* frequency jumps at these times (s); 0 = unused */
	float attack, sustain, punch, decay;	/* seconds; punch boosts the start of sustain */
	float volume;				/* 0..1 */
	uint32_t noise_seed;
} SfxParams;

extern const SfxParams SFX_PRESETS[NUM_SFX];

/* Mono 16-bit render. Returns the sample count, at most max. */
int sfx_render(const SfxParams *p, int rate, int16_t *out, int max);

#endif
