#include "ui/hold.h"

void hold_init(Hold *h)
{
	h->active = 0;
	h->since = 0;
}

int hold_step(Hold *h, int down, uint32_t now_ms)
{
	if (!down) {
		h->active = 0;
		return 0;
	}
	if (!h->active) {
		h->active = 1;
		h->since = now_ms;
		return 0;
	}
	return (uint32_t)(now_ms - h->since) >= HOLD_MS;
}
