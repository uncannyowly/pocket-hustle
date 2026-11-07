#include "sfx/sfx.h"

#include <math.h>

#define TWO_PI 6.28318530718f
#define AMP 30000.0f

/* Fields: wave, freq, slide, accel, duty, duty sweep, vib depth, vib speed,
 * arp mults, arp times, attack, sustain, punch, decay, volume, noise seed. */
const SfxParams SFX_PRESETS[NUM_SFX] = {
	/* MOVE: short cursor blip */
	{ WAVE_SQUARE, 660, 0, 0, 0.5f, 0, 0, 0, {0, 0}, {0, 0},
	  0.002f, 0.020f, 0.2f, 0.013f, 0.45f, 1 },
	/* OK: 523 Hz then up to 784 Hz */
	{ WAVE_SQUARE, 523, 0, 0, 0.5f, 0, 0, 0, {1.499f, 0}, {0.05f, 0},
	  0.002f, 0.078f, 0.2f, 0.040f, 0.5f, 2 },
	/* BACK: 392 Hz sliding down about 140 Hz */
	{ WAVE_SQUARE, 392, -2000, 0, 0.5f, 0, 0, 0, {0, 0}, {0, 0},
	  0.002f, 0.040f, 0.1f, 0.030f, 0.5f, 3 },
	/* DENY: low 130 Hz buzz */
	{ WAVE_SQUARE, 130, 0, 0, 0.35f, 0.2f, 0, 0, {0, 0}, {0, 0},
	  0.002f, 0.090f, 0.1f, 0.038f, 0.6f, 4 },
	/* CASH: 988, 1319, 1760 Hz arpeggio */
	{ WAVE_SQUARE, 988, 0, 0, 0.5f, 0, 0, 0, {1.335f, 1.781f}, {0.045f, 0.09f},
	  0.002f, 0.100f, 0.2f, 0.078f, 0.45f, 5 },
	/* HURT: 200 ms noise burst */
	{ WAVE_NOISE, 7000, -8000, 0, 0.5f, 0, 0, 0, {0, 0}, {0, 0},
	  0.0f, 0.040f, 0.4f, 0.160f, 0.6f, 0x1234567u },
	/* SIREN: saw wail, one slow deep vibrato cycle in 0.5 s */
	{ WAVE_SAW, 620, 0, 0, 0.5f, 0, 0.4f, 2.0f, {0, 0}, {0, 0},
	  0.010f, 0.400f, 0.0f, 0.090f, 0.4f, 7 },
	/* NEWS: pleasant two-note sine chime, A5 then E6 */
	{ WAVE_SINE, 880, 0, 0, 0.5f, 0, 0, 0, {1.498f, 0}, {0.09f, 0},
	  0.005f, 0.115f, 0.1f, 0.220f, 0.6f, 8 },
};

static uint32_t xorshift(uint32_t *s)
{
	uint32_t x = *s;

	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return *s = x;
}

static float clampf(float v, float lo, float hi)
{
	return v < lo ? lo : v > hi ? hi : v;
}

int sfx_render(const SfxParams *p, int rate, int16_t *out, int max)
{
	float secs = p->attack + p->sustain + p->decay;
	int n = (int)(secs * rate), i;
	uint32_t seed = p->noise_seed ? p->noise_seed : 1u;
	float phase = 0, held = 0;

	if (n > max)
		n = max;
	for (i = 0; i < n; i++) {
		float t = (float)i / (float)rate;
		float f = p->freq + p->slide * t + 0.5f * p->slide_accel * t * t;
		float env, s, duty;
		int k;

		for (k = 0; k < 2; k++)
			if (p->arp_time[k] > 0 && t >= p->arp_time[k])
				f *= p->arp_mult[k];
		f *= 1.0f + p->vib_depth * sinf(TWO_PI * p->vib_speed * t);
		f = clampf(f, 20.0f, (float)rate * 0.45f);

		if (t < p->attack)
			env = t / p->attack;
		else if (t < p->attack + p->sustain) {
			float u = (t - p->attack) / p->sustain;
			env = 1.0f + p->punch * (1.0f - u);
		} else
			env = 1.0f - (t - p->attack - p->sustain) / p->decay;
		env = clampf(env, 0.0f, 1.0f + p->punch);

		phase += f / (float)rate;
		if (phase >= 1.0f) {
			phase -= 1.0f;
			if (p->wave == WAVE_NOISE)
				held = (float)(xorshift(&seed) >> 8) / 8388608.0f - 1.0f;
		}
		duty = clampf(p->duty + p->duty_sweep * t, 0.05f, 0.95f);
		switch (p->wave) {
		case WAVE_SQUARE: s = phase < duty ? 1.0f : -1.0f; break;
		case WAVE_SAW: s = 2.0f * phase - 1.0f; break;
		case WAVE_SINE: s = sinf(TWO_PI * phase); break;
		default: s = held; break;
		}
		out[i] = (int16_t)(s * env * p->volume * AMP);
	}
	return n;
}
