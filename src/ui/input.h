/* Button ids (Scorched's) and edge detection with D-pad auto-repeat. */
#ifndef PH_INPUT_H
#define PH_INPUT_H

#include <stdint.h>

enum { PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT, PAD_A, PAD_B, PAD_X, PAD_Y,
       PAD_L1, PAD_R1, PAD_L2, PAD_R2, PAD_SELECT, PAD_START, PAD_MENU, PAD_COUNT };
#define PAD_BIT(b) (1u << (b))

typedef struct { uint32_t held, due[PAD_COUNT]; } Repeat;

/* Buttons that fire now. A button fires on its press edge; the D-pad (only) fires again
 * 300 ms after the press and every 80 ms after that while held. */
uint32_t input_step(Repeat *r, uint32_t held, uint32_t now_ms);

#endif
