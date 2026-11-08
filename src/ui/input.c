#include <string.h>

#include "ui/input.h"

enum { REPEAT_FIRST_MS = 300, REPEAT_NEXT_MS = 80 };

uint32_t input_step(Repeat *r, uint32_t held, uint32_t now_ms)
{
	uint32_t fire = 0;
	int b;

	for (b = 0; b < PAD_COUNT; b++) {
		uint32_t bit = PAD_BIT(b);
		if (!(held & bit))
			continue;
		if (!(r->held & bit)) {
			fire |= bit;
			r->due[b] = now_ms + REPEAT_FIRST_MS;
		} else if (b <= PAD_RIGHT && (int32_t)(now_ms - r->due[b]) >= 0) {
			fire |= bit;
			r->due[b] += REPEAT_NEXT_MS;
			if ((int32_t)(now_ms - r->due[b]) >= 0)
				r->due[b] = now_ms + REPEAT_NEXT_MS;
		}
	}
	r->held = held;
	return fire;
}

int input_token(const char *tok)
{
	static const struct { const char *name; int pad; } TOKENS[] = {
		{"U", PAD_UP}, {"D", PAD_DOWN}, {"L", PAD_LEFT}, {"R", PAD_RIGHT},
		{"A", PAD_A}, {"B", PAD_B}, {"X", PAD_X}, {"Y", PAD_Y},
		{"L1", PAD_L1}, {"R1", PAD_R1}, {"SEL", PAD_SELECT}, {"START", PAD_START},
	};
	size_t i;

	for (i = 0; i < sizeof TOKENS / sizeof TOKENS[0]; i++)
		if (strcmp(tok, TOKENS[i].name) == 0)
			return TOKENS[i].pad;
	return -1;
}
