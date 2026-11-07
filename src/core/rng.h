/* PCG32 random numbers (pcg-c-basic). Deterministic on every platform. */
#ifndef PH_RNG_H
#define PH_RNG_H

#include <stdint.h>

typedef struct { uint64_t state, inc; } Rng;

void     rng_seed(Rng *r, uint64_t seed, uint64_t seq);   /* pcg32_srandom_r */
uint32_t rng_u32(Rng *r);                                 /* pcg32_random_r */
uint32_t rng_below(Rng *r, uint32_t bound);               /* [0, bound), unbiased, bound > 0 */
int32_t  rng_range(Rng *r, int32_t lo, int32_t hi);       /* inclusive, lo <= hi */
int      rng_permille(Rng *r, int32_t permille);          /* 1 with probability permille/1000 */

#endif
