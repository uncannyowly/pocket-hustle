/* Lender portraits, the cop, the vault and the chase street, ported from the mockup. */
#include "gfx/art.h"
#include "gfx/font.h"
#include "gfx/palette.h"

#include <stdio.h>

#define R(x, y, w, h, col) cv_rect(c, x, y, w, h, col)
#define D(x, y, r_, col) cv_disc(c, x, y, r_, col)
#define RD art_rd

typedef struct { uint32_t bg, skin, dk, shirt; } Look;

static const Look LOOK[ART_CITIES] = {
	{0x3a2556u, 0xe2a878u, 0xb9805au, 0x3a7bd5u}, {0x7a3a26u, 0xc98e62u, 0x9c6a46u, 0xc0392bu},
	{0x2b5f86u, 0xf0c49au, 0xc99a74u, 0x6b7280u}, {0x18234au, 0xd9a57cu, 0xad7f5cu, 0x2c3e50u},
	{0x9c3a7au, 0xa86b45u, 0x7f4f31u, 0x18b5a4u}, {0x3c1d5cu, 0x8a5636u, 0x6a3f26u, 0x3a2a5au},
	{0x4f5d6au, 0xf1c7a0u, 0xc99f7cu, 0xb23a3au},
	{0x8a3b1fu, 0xf6d3b4u, 0xd0a584u, 0xe8e0d0u}, {0x5f7f8fu, 0xbf8458u, 0x976438u, 0x6a4a8au},
	{0xc9507au, 0x7a4a2eu, 0x5a3420u, 0x2a2a3au}, {0x3b5a4au, 0xeec3a0u, 0xc99a74u, 0xb03a2eu},
	{0x2a3f6au, 0xd49a6eu, 0xad7a52u, 0xc23a1eu}, {0x6aa0c8u, 0x9a6040u, 0x74442au, 0x7a5a2eu},
	{0x2fa3a8u, 0xe39a78u, 0xb9704fu, 0xf4f0e0u}, {0xe08a2au, 0xb67a52u, 0x8f5a38u, 0xe0307au},
	{0x1f8fb8u, 0xcf9468u, 0xa67046u, 0xff7a5au}, {0xf0a860u, 0xf0b890u, 0xc88e68u, 0x2fb0c0u},
	{0x7a2a3au, 0x8a5030u, 0x683a20u, 0xf0e8d8u},
	{0x3a8ac0u, 0xe8b48au, 0xbf8a62u, 0xf2f2f2u}, {0x2a6aa8u, 0x9a6440u, 0x74462au, 0xc8302au},
};

void art_portrait(Canvas *c, int city, int x, int y)
{
	const Look *l;
	int k;
#define Q(a, b, w, h, col) cv_rect(c, x + (a) * 2, y + (b) * 2, (w) * 2, (h) * 2, col)

	if (city < 0 || city >= ART_CITIES)
		return;
	l = &LOOK[city];
	Q(0, 0, 28, 28, l->bg);
	Q(4, 22, 20, 6, l->shirt);
	Q(2, 24, 24, 4, l->shirt);
	Q(11, 19, 6, 3, l->dk);
	Q(8, 7, 12, 13, l->skin);
	Q(9, 6, 10, 1, l->skin);
	Q(9, 20, 10, 1, l->skin);
	Q(7, 11, 1, 4, l->skin);
	Q(20, 11, 1, 4, l->skin);
	Q(10, 12, 2, 2, 0x1a1424u);
	Q(16, 12, 2, 2, 0x1a1424u);
	Q(12, 17, 4, 1, 0x5a2a2au);
	switch (city) {
	case 0:
		Q(10, 5, 8, 1, 0x9aa0b8u); Q(8, 3, 12, 2, 0xa9aec2u); Q(6, 1, 16, 2, 0xb4b9ccu); Q(5, 0, 18, 1, 0xc4c8d8u);
		Q(8, 2, 6, 1, 0xe2e5efu); Q(11, 4, 6, 1, 0xe2e5efu);
		Q(9, 11, 4, 3, 0x101018u); Q(15, 11, 4, 3, 0x101018u); Q(13, 12, 2, 1, 0x101018u);
		Q(10, 11, 1, 1, 0x7fb8ffu); Q(16, 11, 1, 1, 0x7fb8ffu); Q(16, 16, 1, 1, 0x5a2a2au);
		for (k = 0; k < 5; k++)
			Q(9 + k * 2, 22, 1, 1, 0xffd04au);
		break;
	case 1:
		Q(9, 1, 10, 5, 0x7a4a24u); Q(9, 5, 10, 1, 0x2a1a10u); Q(4, 6, 20, 2, 0x7a4a24u); Q(3, 5, 2, 1, 0x7a4a24u);
		Q(23, 5, 2, 1, 0x7a4a24u); Q(10, 16, 8, 1, 0x5a3418u); Q(9, 17, 2, 1, 0x5a3418u); Q(17, 17, 2, 1, 0x5a3418u);
		Q(13, 22, 2, 2, 0xc0c8d0u); Q(13, 24, 2, 4, 0x2a1a10u);
		break;
	case 2:
		Q(9, 1, 10, 1, 0xffd04au); Q(8, 2, 1, 1, 0xffd04au); Q(19, 2, 1, 1, 0xffd04au); Q(9, 3, 10, 1, 0xffd04au);
		Q(8, 6, 12, 2, 0x3a2a1eu); Q(5, 20, 3, 4, 0x5b6270u); Q(20, 20, 3, 4, 0x5b6270u);
		Q(12, 23, 1, 3, 0xe8e8e8u); Q(15, 23, 1, 3, 0xe8e8e8u); Q(12, 17, 5, 1, 0x5a2a2au); Q(17, 16, 1, 1, 0x5a2a2au);
		break;
	case 3:
		Q(7, 4, 14, 3, 0x4b3b2au); Q(6, 7, 9, 1, 0x3a2d20u); Q(9, 10, 4, 1, 0x2a2018u); Q(15, 10, 4, 1, 0x2a2018u);
		Q(16, 17, 4, 1, 0xe0c08au); Q(11, 22, 6, 2, 0xe8e8e8u);
		break;
	case 4:
		Q(5, 3, 18, 5, 0xff7eb6u); Q(4, 6, 3, 10, 0xff7eb6u); Q(21, 6, 3, 10, 0xff7eb6u); Q(7, 4, 2, 1, 0xffb3d4u);
		Q(15, 3, 3, 1, 0xffb3d4u); Q(9, 11, 4, 3, 0xff3f8eu); Q(15, 11, 4, 3, 0xff3f8eu); Q(13, 12, 2, 1, 0xff3f8eu);
		Q(6, 16, 1, 2, 0xffd04au); Q(21, 16, 1, 2, 0xffd04au); Q(12, 17, 4, 1, 0xd6336cu);
		break;
	case 5: {
		static const uint32_t bc[3] = {0x7b2fbeu, 0xffd04au, 0x3fbf6au};

		Q(7, 4, 14, 4, 0x2b1b12u); Q(6, 7, 2, 10, 0x2b1b12u); Q(20, 7, 2, 10, 0x2b1b12u); Q(8, 11, 12, 3, 0x7b2fbeu);
		Q(8, 10, 12, 1, 0xffd04au); Q(10, 12, 2, 1, 0x1a1424u); Q(16, 12, 2, 1, 0x1a1424u); Q(19, 6, 1, 5, 0x3fbf6au);
		for (k = 0; k < 6; k++)
			Q(8 + k * 2, 22, 1, 1, bc[k % 3]);
		break;
	}
	case 6:
		Q(8, 3, 12, 4, 0xe07a2fu); Q(8, 6, 12, 1, 0xc0601fu); Q(13, 1, 2, 2, 0xf4d0a8u); Q(8, 15, 12, 6, 0x8a5a2bu);
		Q(9, 21, 10, 1, 0x8a5a2bu); Q(12, 16, 4, 1, 0x5a3418u);
		for (k = 0; k < 7; k++)
			Q(4 + k * 3, 22, 1, 6, 0x2a1a1au);
		Q(2, 25, 24, 1, 0x2a1a1au);
		break;
	case 7:	/* Brisket Brenda: big blond hair, apron, a smoked brisket */
		Q(5, 0, 18, 3, 0xe8c15au); Q(4, 3, 20, 5, 0xe8c15au); Q(3, 6, 3, 10, 0xe8c15au); Q(22, 6, 3, 10, 0xe8c15au);
		Q(8, 2, 3, 1, 0xf6e08au); Q(15, 4, 4, 1, 0xf6e08au); Q(8, 8, 12, 1, 0xd0a840u);
		Q(8, 22, 12, 6, 0xb03030u); Q(9, 23, 10, 1, 0xe8e0d0u); Q(12, 25, 4, 1, 0xe8e0d0u);
		Q(20, 23, 7, 4, 0x5a2e1au); Q(21, 24, 5, 1, 0xc0506au); Q(21, 26, 5, 1, 0x3a1c10u);
		break;
	case 8:	/* Sourdough Sam: green beanie, beard, a scored loaf */
		Q(8, 3, 12, 4, 0x2f6f4fu); Q(8, 6, 12, 1, 0x1f4f38u); Q(13, 1, 2, 2, 0xd8d8c0u);
		Q(9, 15, 10, 5, 0x6a4a2au); Q(12, 17, 4, 1, 0x5a2a2au); Q(8, 12, 1, 4, 0x6a4a2au); Q(19, 12, 1, 4, 0x6a4a2au);
		Q(16, 22, 10, 5, 0xc98a3au); Q(17, 22, 8, 1, 0xdca64eu); Q(18, 23, 5, 1, 0xf0d9a0u); Q(20, 25, 5, 1, 0xa86e2au);
		Q(19, 24, 4, 1, 0xf0d9a0u);
		break;
	case 9:	/* Influencer Ivy: ring light glow, shades, a phone */
		Q(4, 2, 20, 1, 0xfff2c8u); Q(3, 3, 1, 18, 0xfff2c8u); Q(24, 3, 1, 18, 0xfff2c8u); Q(5, 1, 18, 1, 0xffd0e4u);
		Q(2, 4, 1, 14, 0xffa8cfu); Q(25, 4, 1, 14, 0xffa8cfu);
		Q(7, 3, 14, 4, 0x1e1218u); Q(5, 6, 3, 14, 0x1e1218u); Q(20, 6, 3, 14, 0x1e1218u);
		Q(9, 11, 4, 3, 0x101018u); Q(15, 11, 4, 3, 0x101018u); Q(13, 11, 2, 1, 0x101018u);
		Q(10, 11, 1, 1, 0xffb3d4u); Q(16, 11, 1, 1, 0xffb3d4u); Q(11, 17, 6, 1, 0xffffffu);
		Q(21, 15, 4, 7, 0x20202au); Q(22, 16, 2, 5, 0x7fd0ffu);
		break;
	case 10:	/* Espresso Ezra: messy hair, round specs, flannel, a coffee cup */
		Q(7, 4, 14, 3, 0x4a3020u); Q(6, 6, 2, 5, 0x4a3020u); Q(20, 6, 2, 5, 0x4a3020u); Q(9, 3, 3, 1, 0x4a3020u);
		Q(9, 10, 4, 1, 0x2a2a2au); Q(9, 13, 4, 1, 0x2a2a2au); Q(9, 10, 1, 4, 0x2a2a2au); Q(12, 10, 1, 4, 0x2a2a2au);
		Q(15, 10, 4, 1, 0x2a2a2au); Q(15, 13, 4, 1, 0x2a2a2au); Q(15, 10, 1, 4, 0x2a2a2au); Q(18, 10, 1, 4, 0x2a2a2au);
		Q(13, 11, 2, 1, 0x2a2a2au); Q(9, 18, 10, 2, 0xc9a07cu);
		for (k = 0; k < 6; k++)
			Q(4 + k * 4, 22, 1, 6, 0x1a1a1au);
		Q(2, 25, 24, 1, 0x1a1a1au);
		Q(21, 19, 5, 1, 0x3a2a1au); Q(21, 20, 5, 6, 0xf4efe6u); Q(21, 22, 5, 2, 0x8a5a2bu);
		Q(22, 16, 1, 2, 0xddddddu); Q(24, 17, 1, 2, 0xddddddu);
		break;
	case 11:	/* Three-Way Wes: side part, mustache, chili-red apron, a bowl */
		Q(8, 4, 12, 3, 0x3a2418u); Q(8, 3, 8, 1, 0x3a2418u); Q(7, 6, 1, 4, 0x3a2418u); Q(20, 6, 1, 4, 0x3a2418u);
		Q(10, 16, 8, 1, 0x3a2418u); Q(9, 17, 2, 1, 0x3a2418u); Q(17, 17, 2, 1, 0x3a2418u);
		Q(8, 20, 1, 3, 0xf2f2f2u); Q(19, 20, 1, 3, 0xf2f2f2u);
		Q(9, 24, 10, 3, 0xe8e0d0u); Q(10, 27, 8, 1, 0xc8c0b0u); Q(10, 23, 8, 1, 0x7a2a10u);
		Q(11, 22, 6, 1, 0xf2c14au); Q(10, 21, 1, 1, 0xf2c14au);
		break;
	case 12:	/* Propeller Pat: aviator cap, goggles, scarf, a propeller on top */
		Q(7, 3, 14, 5, 0x6b4423u); Q(6, 7, 2, 8, 0x6b4423u); Q(20, 7, 2, 8, 0x6b4423u); Q(8, 7, 12, 1, 0x4a2e16u);
		Q(9, 1, 10, 1, 0xc0c0c8u); Q(13, 0, 2, 3, 0xe0e0e0u);
		Q(9, 10, 4, 4, 0x2a2a2au); Q(15, 10, 4, 4, 0x2a2a2au); Q(13, 11, 2, 1, 0x2a2a2au);
		Q(10, 11, 2, 2, 0x9fd8ffu); Q(16, 11, 2, 2, 0x9fd8ffu); Q(10, 11, 1, 1, 0xffffffu); Q(16, 11, 1, 1, 0xffffffu);
		Q(8, 20, 12, 2, 0xf2f2f2u); Q(20, 21, 4, 5, 0xf2f2f2u);
		break;
	case 13:	/* Snapper Sally: tan bucket hat with a lure, sunburn, a red snapper */
		Q(7, 2, 14, 5, 0xd8c07au); Q(4, 6, 20, 2, 0xd8c07au); Q(8, 8, 12, 1, 0xb89a54u); Q(7, 5, 14, 1, 0x3a6a4au);
		Q(8, 3, 2, 1, 0xe04a3au);
		Q(8, 14, 3, 2, 0xd0503au); Q(17, 14, 3, 2, 0xd0503au); Q(13, 14, 2, 2, 0xd0503au);
		Q(16, 23, 8, 3, 0xe04a4au); Q(24, 22, 2, 5, 0xe04a4au); Q(17, 24, 1, 1, 0xffffffu); Q(17, 25, 6, 1, 0xf5b0a0u);
		break;
	case 14:	/* Zonkey Zoe: black and white striped zonkey hat with ears, braids */
		for (k = 0; k < 6; k++)
			Q(8 + k * 2, 3, 2, 4, k & 1 ? 0x1a1a1au : 0xf2f2f2u);
		Q(8, 6, 12, 1, 0x1a1a1au); Q(8, 0, 2, 4, 0xf2f2f2u); Q(18, 0, 2, 4, 0x1a1a1au); Q(9, 1, 1, 2, 0xf0a0b0u);
		Q(18, 1, 1, 2, 0xf0a0b0u); Q(6, 7, 2, 12, 0x1a1018u); Q(20, 7, 2, 12, 0x1a1018u); Q(6, 19, 2, 2, 0xe0307au);
		Q(20, 19, 2, 2, 0xe0307au); Q(13, 5, 2, 1, 0xf2f2f2u);
		break;
	case 15:	/* Reef Rita: sun-bleached hair, dive mask on the forehead, a snorkel */
		Q(7, 3, 14, 3, 0xe8c870u); Q(6, 5, 2, 10, 0xe8c870u); Q(20, 5, 2, 10, 0xe8c870u);
		Q(8, 6, 12, 4, 0x1a2a3au); Q(9, 7, 10, 2, 0x7fe0ffu); Q(13, 7, 2, 2, 0x1a2a3au); Q(10, 7, 2, 1, 0xdff8ffu);
		Q(7, 7, 1, 2, 0x1a1a1au); Q(20, 7, 1, 2, 0x1a1a1au);
		Q(22, 2, 2, 9, 0xffd04au); Q(22, 1, 3, 1, 0xffd04au); Q(22, 11, 1, 1, 0xff7a5au);
		break;
	case 16:	/* Timeshare Tim: slicked hair, big white grin, polo, a clipboard */
		Q(8, 4, 12, 3, 0x3a2a1au); Q(9, 3, 10, 1, 0x3a2a1au); Q(8, 6, 2, 2, 0x3a2a1au); Q(10, 4, 3, 1, 0x6a5238u);
		Q(11, 17, 6, 1, 0xffffffu); Q(10, 16, 1, 1, 0xffffffu); Q(17, 16, 1, 1, 0xffffffu);
		Q(9, 21, 3, 2, 0xffffffu); Q(16, 21, 3, 2, 0xffffffu); Q(6, 24, 3, 2, 0xffffffu);
		Q(19, 16, 8, 10, 0x8a5a2bu); Q(20, 17, 6, 8, 0xf5f5f0u); Q(22, 16, 2, 1, 0xc0c0c8u);
		Q(21, 19, 4, 1, 0x8a8a98u); Q(21, 21, 4, 1, 0x8a8a98u); Q(21, 23, 3, 1, 0x8a8a98u);
		break;
	case 17:	/* Banda Bea: black brimmed hat with a gold band, earrings, a sash */
		Q(9, 1, 10, 5, 0x2a2a32u); Q(3, 6, 22, 2, 0x2a2a32u); Q(9, 5, 10, 1, 0xe8b030u); Q(3, 7, 22, 1, 0xe8b030u);
		Q(6, 8, 2, 10, 0x140c10u); Q(20, 8, 2, 10, 0x140c10u); Q(6, 16, 1, 2, 0xe8b030u); Q(21, 16, 1, 2, 0xe8b030u);
		Q(9, 11, 4, 1, 0x140c10u); Q(15, 11, 4, 1, 0x140c10u); Q(12, 17, 4, 1, 0xa03a4au);
		for (k = 0; k < 5; k++) {
			Q(5 + k * 4, 22 + k, 5, 2, 0xd02a8au);
			Q(5 + k * 4, 23 + k, 1, 1, 0xe8b030u);
		}
		break;
	case 18:	/* Deep Dish Dom: slicked hair, a big mustache, a red star tee, a deep slice */
		Q(8, 4, 12, 3, 0x2a1a14u); Q(7, 6, 2, 4, 0x2a1a14u); Q(19, 6, 2, 4, 0x2a1a14u); Q(10, 3, 6, 1, 0x4a3226u);
		Q(9, 15, 10, 2, 0x2a1a14u); Q(8, 16, 2, 2, 0x2a1a14u); Q(18, 16, 2, 2, 0x2a1a14u);
		Q(7, 22, 1, 1, 0xe03a3au); Q(5, 23, 5, 1, 0xe03a3au); Q(6, 24, 3, 1, 0xe03a3au);	/* a flag star */
		Q(5, 25, 5, 1, 0xe03a3au); Q(7, 26, 1, 1, 0xe03a3au);
		Q(17, 21, 9, 5, 0xd89a4au); Q(18, 21, 7, 1, 0xc8402au); Q(18, 22, 7, 2, 0xf0d890u);
		Q(17, 26, 9, 1, 0xa86a2au); Q(19, 22, 1, 1, 0xc8402au); Q(22, 23, 1, 1, 0xc8402au);
		break;
	case 19:	/* Coney Connie: big curls, a paper diner cap, hoops, a coney dog with the works */
		Q(6, 4, 16, 4, 0x1e1410u); Q(5, 7, 3, 11, 0x1e1410u); Q(20, 7, 3, 11, 0x1e1410u); Q(4, 10, 2, 6, 0x1e1410u);
		Q(22, 10, 2, 6, 0x1e1410u);
		Q(8, 2, 12, 3, 0xf4f4f4u); Q(9, 1, 10, 1, 0xf4f4f4u); Q(8, 4, 12, 1, 0xc8302au);
		Q(6, 17, 1, 2, 0xe8c040u); Q(21, 17, 1, 2, 0xe8c040u); Q(12, 17, 4, 1, 0xc02a4au);
		Q(9, 22, 10, 1, 0xf2f2f2u);
		Q(16, 22, 11, 4, 0xe0b070u); Q(15, 21, 13, 1, 0xa0402au); Q(17, 20, 9, 1, 0x7a3a1au);
		Q(18, 19, 1, 1, 0xf0d020u); Q(20, 20, 1, 1, 0xf0d020u); Q(22, 19, 1, 1, 0xf0d020u); Q(24, 20, 1, 1, 0xf0d020u);
		Q(19, 20, 1, 1, 0xf8f8f0u); Q(23, 20, 1, 1, 0xf8f8f0u);
		break;
	}
#undef Q
}

void art_cop(Canvas *c, int x, int base, int sc, int boss)
{
	int top = base - 28 * sc;
	uint32_t nv = 0x1b2a5au, nd = 0x111b3du, uni = boss ? 0x2a4a8au : 0x24407au, sk = 0xd9a066u;
#define Q(a, b, w, h, col) cv_rect(c, x + (a) * sc, top + (b) * sc, (w) * sc, (h) * sc, col)

	Q(5, 0, 10, 3, nv); Q(9, 1, 2, 1, 0xffd04au); Q(3, 3, 14, 1, nd); Q(6, 4, 8, 7, sk);
	Q(7, 6, 2, 1, 0x141018u); Q(11, 6, 2, 1, 0x141018u);
	if (boss) {
		Q(7, 5, 2, 1, 0x141018u); Q(11, 5, 2, 1, 0x141018u); Q(7, 8, 6, 1, 0x5a3418u);
	}
	Q(9, 9, 2, 1, 0x5a2a2au); Q(8, 11, 4, 1, 0xb8844eu);
	Q(4, 12, 12, 9, uni); Q(6, 14, 2, 2, 0xffd04au); Q(4, 19, 12, 1, 0x0a0a12u); Q(9, 19, 2, 1, 0xffd04au);
	Q(2, 12, 2, 8, uni); Q(16, 12, 2, 8, uni); Q(2, 20, 2, 1, sk); Q(16, 20, 2, 1, sk);
	Q(5, 21, 4, 7, nd); Q(11, 21, 4, 7, nd); Q(4, 27, 5, 1, 0x0a0a12u); Q(11, 27, 5, 1, 0x0a0a12u);
	if (boss) {
		Q(16, 14, 2, 1, 0xffd04au); Q(16, 16, 2, 1, 0xffd04au); Q(17, 18, 3, 3, 0xff8ac0u); Q(18, 19, 1, 1, 0x3a1a2au);
	}
#undef Q
}

void art_vault(Canvas *c, int x0, int y0, uint32_t t)
{
	int k;

	D(x0, y0, 40, 0x3d4352u);
	D(x0, y0, 37, 0x8a93a8u);
	D(x0, y0, 33, 0x6c7588u);
	for (k = 0; k < 12; k++) {
		int a = RD(256LL * k, 12);

		D(x0 + RD(35LL * icos(a), 1024), y0 + RD(35LL * isin(a), 1024), 1, 0xc9cfdbu);
	}
	for (k = 0; k < 3; k++) {
		/* angle = frame * 0.01 rad + k * pi / 3, in 1/256 steps; 1 rad = 10430/256 steps */
		long long q8 = (long long)t * 10430 / 100 + (long long)k * 10923;
		int a = RD(q8, 256);
		int dx = RD(24LL * icos(a), 1024), dy = RD(24LL * isin(a), 1024);

		cv_line(c, x0 - dx, y0 - dy, x0 + dx, y0 + dy, 0xd8dde7u);
		cv_line(c, x0 - dx + 1, y0 - dy, x0 + dx + 1, y0 + dy, 0xd8dde7u);
		D(x0 + dx, y0 + dy, 3, 0xd8dde7u);
		D(x0 - dx, y0 - dy, 3, 0xd8dde7u);
	}
	D(x0, y0, 8, 0xaab2c2u);
	D(x0, y0, 5, 0xffd04au);
	R(x0 + 40, y0 - 14, 4, 8, 0x2a2f3au);
	R(x0 + 40, y0 + 6, 4, 8, 0x2a2f3au);
}

void art_chase_scene(Canvas *c, int y0, int cops_left, uint32_t t)
{
	static const uint32_t flash[3] = {0x2a0a24u, 0x4a1030u, 0x1a0c2au}, calm[3] = {0x0a0c2cu, 0x10225au, 0x0c0c2au};
	int sx0 = c->cx0, sy0 = c->cy0, sx1 = c->cx1, sy1 = c->cy1;
	int o = y0 - 12, fl = (int)((t >> 2) & 1), x, k, extra, n;
	ArtRng r;

	cv_clip(c, 0, y0, CV_W, y0 + 96);
	if (c->cx0 < sx0) c->cx0 = sx0;
	if (c->cy0 < sy0) c->cy0 = sy0;
	if (c->cx1 > sx1) c->cx1 = sx1;
	if (c->cy1 > sy1) c->cy1 = sy1;
	cv_grad(c, 0, y0, CV_W, 72, fl ? flash : calm, 3);
	art_seed(&r, 5);
	x = 0;
	while (x < CV_W) {
		int w = 14 + art_below(&r, 18), h = 24 + art_below(&r, 36);

		art_bldg(c, x, 84 - h + o, w, 84 + o, 0x0b0a1au, 0x3a3460u, 20, &r);
		x += w + 2;
	}
	R(0, 84 + o, CV_W, 24, 0x1b1a2cu);
	R(0, 84 + o, CV_W, 1, 0x3a3858u);
	for (k = 0; k < CV_W; k += 24)
		R(k, 97 + o, 12, 1, 0x4a4868u);
	R(122, 42 + o, 2, 42, 0x2a2840u);
	R(116, 40 + o, 10, 3, 0x2a2840u);
	R(116, 43 + o, 6, 1, 0xffe8a0u);
	R(10, 90 + o, 64, 12, 0xe8e8f0u);
	R(22, 82 + o, 34, 9, 0xe8e8f0u);
	R(25, 84 + o, 12, 6, 0x3a4a6au);
	R(40, 84 + o, 13, 6, 0x3a4a6au);
	R(10, 95 + o, 64, 2, 0x1b2a5au);
	D(24, 103 + o, 5, 0x101018u);
	D(60, 103 + o, 5, 0x101018u);
	D(24, 103 + o, 2, 0x6a6a7au);
	D(60, 103 + o, 2, 0x6a6a7au);
	R(28, 79 + o, 10, 3, fl ? 0xff3b3bu : 0x5a1a1au);
	R(40, 79 + o, 10, 3, fl ? 0x22225au : 0x3b7bffu);
	if (cops_left > 0)
		art_cop(c, 150, 106 + o, 2, 1);
	extra = cops_left > 1 ? cops_left - 1 : 0;
	n = extra < 5 ? extra : 5;
	for (k = 0; k < n; k++)
		art_cop(c, 206 + k * 22, 106 + o, 1, 0);
	if (extra > 5) {
		char buf[16];

		snprintf(buf, sizeof buf, "+%d", extra - 5);
		tx_outline(c, buf, 316, 70 + o, PAL_WHITE, PAL_BLACK, 1, TX_RIGHT);
	}
	c->cx0 = sx0; c->cy0 = sy0; c->cx1 = sx1; c->cy1 = sy1;
}
