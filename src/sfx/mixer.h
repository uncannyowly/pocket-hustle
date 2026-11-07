/* Six-voice mixer over pre-rendered effects. No locking: the platform wraps
 * mixer_play and mixer_fill in its own audio lock. No allocation after init. */
#ifndef PH_MIXER_H
#define PH_MIXER_H

#include "sfx/sfx.h"

enum { MIX_RATE = 44100, MIX_VOICES = 6 };

typedef struct {
	int16_t *buf[NUM_SFX];
	int len[NUM_SFX];
	int vid[MIX_VOICES], vpos[MIX_VOICES];	/* vid < 0: voice is free */
	int muted;
} Mixer;

int  mixer_init(Mixer *m);		/* renders every preset at MIX_RATE; 0 or -1 */
void mixer_free(Mixer *m);
void mixer_play(Mixer *m, SfxId id);	/* a free voice, else the one closest to its end */
void mixer_fill(Mixer *m, int16_t *stereo, int frames);	/* saturating; zeros when muted */

#endif
