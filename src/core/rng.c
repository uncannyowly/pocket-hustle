/* PCG32, minimal version from pcg-c-basic (Melissa O'Neill, Apache 2.0 / MIT). */
#include "core/rng.h"

void rng_seed(Rng *r, uint64_t seed, uint64_t seq)
{
	r->state = 0u;
	r->inc = (seq << 1u) | 1u;
	rng_u32(r);
	r->state += seed;
	rng_u32(r);
}

uint32_t rng_u32(Rng *r)
{
	uint64_t old = r->state;
	uint32_t xorshifted, rot;

	r->state = old * 6364136223846793005ULL + r->inc;
	xorshifted = (uint32_t)(((old >> 18u) ^ old) >> 27u);
	rot = (uint32_t)(old >> 59u);
	return (xorshifted >> rot) | (xorshifted << ((32u - rot) & 31u));
}

uint32_t rng_below(Rng *r, uint32_t bound)
{
	uint32_t threshold = (0u - bound) % bound;

	for (;;) {
		uint32_t v = rng_u32(r);
		if (v >= threshold)
			return v % bound;
	}
}

int32_t rng_range(Rng *r, int32_t lo, int32_t hi)
{
	uint32_t span = (uint32_t)((int64_t)hi - (int64_t)lo) + 1u;

	if (span == 0u)		/* the whole 32-bit range */
		return (int32_t)rng_u32(r);
	return (int32_t)((int64_t)lo + (int64_t)rng_below(r, span));
}

int rng_permille(Rng *r, int32_t permille)
{
	if (permille <= 0)
		return 0;
	if (permille >= 1000)
		return 1;
	return rng_below(r, 1000u) < (uint32_t)permille;
}
