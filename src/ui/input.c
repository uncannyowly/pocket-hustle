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
