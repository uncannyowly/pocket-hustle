#include "sfx/mixer.h"

#include <stdlib.h>

#define MAX_RENDER (MIX_RATE * 3 / 2)

int mixer_init(Mixer *m)
{
	static int16_t tmp[MAX_RENDER];
	int id, v, i;

	m->muted = 0;
	for (v = 0; v < MIX_VOICES; v++) {
		m->vid[v] = -1;
		m->vpos[v] = 0;
	}
	for (id = 0; id < NUM_SFX; id++) {
		m->buf[id] = NULL;
		m->len[id] = 0;
	}
	for (id = 0; id < NUM_SFX; id++) {
		int n = sfx_render(&SFX_PRESETS[id], MIX_RATE, tmp, MAX_RENDER);

		m->buf[id] = malloc((size_t)(n > 0 ? n : 1) * sizeof(int16_t));
		if (!m->buf[id]) {
			mixer_free(m);
			return -1;
		}
		for (i = 0; i < n; i++)
			m->buf[id][i] = tmp[i];
		m->len[id] = n;
	}
	return 0;
}

void mixer_free(Mixer *m)
{
	int id;

	for (id = 0; id < NUM_SFX; id++) {
		free(m->buf[id]);
		m->buf[id] = NULL;
		m->len[id] = 0;
	}
	for (id = 0; id < MIX_VOICES; id++)
		m->vid[id] = -1;
}

void mixer_play(Mixer *m, SfxId id)
{
	int v, pick = 0, best = 0x7fffffff;

	if ((int)id < 0 || id >= NUM_SFX || !m->buf[id])
		return;
	for (v = 0; v < MIX_VOICES; v++) {
		int left = m->vid[v] < 0 ? -1 : m->len[m->vid[v]] - m->vpos[v];

		if (left < best) {
			best = left;
			pick = v;
		}
	}
	m->vid[pick] = (int)id;
	m->vpos[pick] = 0;
}

void mixer_fill(Mixer *m, int16_t *stereo, int frames)
{
	int f, v;

	for (f = 0; f < frames; f++) {
		int sum = 0;

		for (v = 0; v < MIX_VOICES; v++) {
			int id = m->vid[v];

			if (id < 0)
				continue;
			if (m->vpos[v] >= m->len[id]) {
				m->vid[v] = -1;
				continue;
			}
			sum += m->buf[id][m->vpos[v]++];
		}
		if (m->muted)
			sum = 0;
		if (sum > 32767)
			sum = 32767;
		else if (sum < -32768)
			sum = -32768;
		stereo[2 * f] = stereo[2 * f + 1] = (int16_t)sum;
	}
}
