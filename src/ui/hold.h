/* Holding a button for two seconds (MENU quits on the Mini). Pure; the caller supplies the clock. */
#ifndef PH_HOLD_H
#define PH_HOLD_H

#include <stdint.h>

enum { HOLD_MS = 2000 };
typedef struct { int active; uint32_t since; } Hold;

void hold_init(Hold *h);
/* down: the button is held now. Returns 1 once it has been held HOLD_MS without a release
 * (and on every call after, until released). now_ms may wrap. */
int hold_step(Hold *h, int down, uint32_t now_ms);

#endif
