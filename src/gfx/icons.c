/* The mockup's pixel icons (the table I, ICOL and arrowIcon). */
#include "gfx/art.h"
#include "gfx/palette.h"

static uint32_t icol(char ch)
{
	switch (ch) {
	case 'y': return PAL_GOLD;
	case 'o': return PAL_GOLD_DK;
	case 'm': return PAL_MINT;
	case 'c': return PAL_CORAL;
	case 'b': return 0x3f6fb0u;
	case 'w': return PAL_WHITE;
	case 'k': return PAL_INK;
	case 'd': return PAL_DIM;
	case 'h': return PAL_HOT;
	case 'g': return 0xa8b0c6u;
	case 's': return 0x5a6274u;
	case 'n': return PAL_NIGHT;
	default: return 0;
	}
}

typedef struct { int rows; const char *row[7]; } IconDef;

static const IconDef ICONS[NUM_ICONS] = {
	[ICON_HEART] = {6, {".cc.cc.", "ccccccc", "ccccccc", ".ccccc.", "..ccc..", "...c..."}},
	[ICON_COIN] = {7, {"..ooo..", ".oyyyo.", "oyywyyo", "oyyoyyo", "oyyoyyo", ".oyyyo.", "..ooo.."}},
	[ICON_BANK] = {7, {"...m...", "..mmm..", ".mmmmm.", ".m.m.m.", ".m.m.m.", ".m.m.m.", "mmmmmmm"}},
	[ICON_FIN] = {7, {"...c....", "...cc...", "..ccc...", "..cccc..", ".ccccc..", "cccccccc", "b.b.b.b."}},
	[ICON_BOLT] = {7, {"...yy", "..yy.", ".yyyy", "..yy.", ".yy..", "yy...", "y...."}},
	[ICON_UP] = {5, {"..#..", ".###.", "#####", "..#..", "..#.."}},
	[ICON_DOWN] = {5, {"..#..", "..#..", "#####", ".###.", "..#.."}},
	[ICON_MIC] = {6, {".nnn.", "nwnwn", "nnnnn", ".nnn.", "..n..", "nnnnn"}},
	[ICON_BANG] = {6, {"..n..", "..n..", "..n..", "..n..", ".....", "..n.."}},
	[ICON_TRAIN] = {5, {"nnnnnnn", "n.n.n.n", "nnnnnnn", "nnnnnnn", ".n...n."}},
	[ICON_SUN] = {7, {"n..n..n", ".nnnnn.", ".nnnnn.", "nnnnnnn", ".nnnnn.", ".nnnnn.", "n..n..n"}},
	[ICON_CHART] = {5, {"......n", ".....n.", "n...n..", ".n.n...", "..n...."}},
	[ICON_CHECK] = {5, {"......m", ".....m.", "m...m..", ".m.m...", "..m...."}},
	[ICON_STAR] = {5, {"...y...", "..yyy..", "yyyyyyy", ".yyyyy.", ".yy.yy."}},
};

void art_icon(Canvas *c, IconId id, int x, int y, int scale, uint32_t tint)
{
	const IconDef *d;
	int i, j;

	if ((int)id < 0 || id >= NUM_ICONS)
		return;
	d = &ICONS[id];
	for (j = 0; j < d->rows; j++)
		for (i = 0; d->row[j][i]; i++) {
			char ch = d->row[j][i];

			if (ch == '.')
				continue;
			cv_rect(c, x + i * scale, y + j * scale, scale, scale, ch == '#' ? tint : icol(ch));
		}
}
