#include "check.h"
#include "tests.h"
#include "sfx/sfx.h"
#include "sfx/mixer.h"

#include <stdlib.h>
#include <string.h>

#define MAXS (MIX_RATE * 2)

static int16_t scratch[MAXS];

static void test_render_lengths(void)
{
	int id;

	for (id = 0; id < NUM_SFX; id++) {
		const SfxParams *p = &SFX_PRESETS[id];
		float secs = p->attack + p->sustain + p->decay;
		int want = (int)(secs * MIX_RATE);
		int n = sfx_render(p, MIX_RATE, scratch, MAXS);

		CHECK(n >= want - 1 && n <= want + 1);
		CHECK(secs < 1.5f);
	}
}

static void test_render_deterministic(void)
{
	static int16_t a[MAXS], b[MAXS];
	int id;

	for (id = 0; id < NUM_SFX; id++) {
		int na = sfx_render(&SFX_PRESETS[id], MIX_RATE, a, MAXS);
		int nb = sfx_render(&SFX_PRESETS[id], MIX_RATE, b, MAXS);

		CHECK_EQ(na, nb);
		CHECK(memcmp(a, b, (size_t)na * sizeof a[0]) == 0);
	}
}

static void test_render_audible(void)
{
	int id, i;

	for (id = 0; id < NUM_SFX; id++) {
		int n = sfx_render(&SFX_PRESETS[id], MIX_RATE, scratch, MAXS);
		int peak = 0;

		for (i = 0; i < n; i++) {
			int v = abs(scratch[i]);
			if (v > peak)
				peak = v;
		}
		CHECK(peak > 2000);
		CHECK(peak < 32767);
	}
}

static void test_mixer_silence(void)
{
	Mixer m;
	int16_t out[64];
	int i, nz = 0;

	CHECK_EQ(mixer_init(&m), 0);
	memset(out, 0x55, sizeof out);
	mixer_fill(&m, out, 32);
	for (i = 0; i < 64; i++)
		nz += out[i] != 0;
	CHECK_EQ(nz, 0);
	mixer_free(&m);
}

static void test_mixer_plays_once(void)
{
	static int16_t out[MAXS * 2];
	Mixer m;
	int i, frames, nz = 0, lr = 0, tail = 0;

	CHECK_EQ(mixer_init(&m), 0);
	CHECK(m.len[SFX_MOVE] > 0);
	mixer_play(&m, SFX_MOVE);
	frames = m.len[SFX_MOVE] + 200;
	mixer_fill(&m, out, frames);
	for (i = 0; i < 200; i++)
		nz += out[2 * i] != 0;
	for (i = 0; i < frames; i++)
		lr += out[2 * i] != out[2 * i + 1];
	for (i = m.len[SFX_MOVE]; i < frames; i++)
		tail += out[2 * i] != 0 || out[2 * i + 1] != 0;
	CHECK(nz > 100);
	CHECK_EQ(lr, 0);
	CHECK_EQ(tail, 0);
	mixer_free(&m);
}

static void test_mixer_saturates(void)
{
	static int16_t big[100];
	static int16_t neg[100];
	Mixer m;
	int16_t out[20];
	int i, bad = 0;

	memset(&m, 0, sizeof m);	/* test-owned buffers: never passed to mixer_free */
	for (i = 0; i < 100; i++) {
		big[i] = 30000;
		neg[i] = -30000;
	}
	m.buf[SFX_MOVE] = big;
	m.len[SFX_MOVE] = 100;
	m.buf[SFX_OK] = neg;
	m.len[SFX_OK] = 100;
	for (i = 0; i < MIX_VOICES; i++) {
		m.vid[i] = SFX_MOVE;
		m.vpos[i] = 0;
	}
	mixer_fill(&m, out, 10);
	for (i = 0; i < 20; i++)
		bad += out[i] != 32767;
	CHECK_EQ(bad, 0);

	for (i = 0; i < MIX_VOICES; i++) {
		m.vid[i] = SFX_OK;
		m.vpos[i] = 0;
	}
	mixer_fill(&m, out, 10);
	bad = 0;
	for (i = 0; i < 20; i++)
		bad += out[i] != -32768;
	CHECK_EQ(bad, 0);
}

static void test_mixer_voice_steal(void)
{
	Mixer m;
	int i;

	CHECK_EQ(mixer_init(&m), 0);
	CHECK(m.len[SFX_SIREN] > 1000);
	for (i = 0; i < MIX_VOICES; i++) {
		m.vid[i] = SFX_SIREN;
		m.vpos[i] = 100 * (i + 1);	/* voice 5 is nearest its end */
	}
	mixer_play(&m, SFX_MOVE);
	CHECK_EQ(m.vid[5], SFX_MOVE);
	CHECK_EQ(m.vpos[5], 0);
	for (i = 0; i < 5; i++) {
		CHECK_EQ(m.vid[i], SFX_SIREN);
		CHECK_EQ(m.vpos[i], 100 * (i + 1));
	}
	mixer_free(&m);
}

static void test_mixer_muted(void)
{
	Mixer m;
	int16_t out[200];
	int i, nz = 0;

	CHECK_EQ(mixer_init(&m), 0);
	mixer_play(&m, SFX_SIREN);
	m.muted = 1;
	memset(out, 0x55, sizeof out);
	mixer_fill(&m, out, 100);
	for (i = 0; i < 200; i++)
		nz += out[i] != 0;
	CHECK_EQ(nz, 0);
	CHECK_EQ(m.vpos[0], 100);
	mixer_free(&m);
}

void tests_sfx(void)
{
	test_render_lengths();
	test_render_deterministic();
	test_render_audible();
	test_mixer_silence();
	test_mixer_plays_once();
	test_mixer_saturates();
	test_mixer_voice_steal();
	test_mixer_muted();
}
