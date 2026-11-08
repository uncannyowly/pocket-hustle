/* Turning the core's messages into words. Every line is original game text. */
#ifndef PH_TEXT_H
#define PH_TEXT_H

#include <stddef.h>

#include "core/game.h"

typedef struct { int8_t city; const char *t; } TextLine;	/* city -1 = any city, -2 = border crossings only (M_FLEW with c = 1) */

/* Formats m into buf (always NUL-terminated, truncated to n). Returns the length. */
size_t text_msg(const Game *g, const Msg *m, char *buf, size_t n);

/* $1,234 and -$5,500. */
void fmt_money(int64_t v, char *buf, size_t n);

/* For tests and the content lint. */
int             text_lines(MsgKind k, const TextLine **lines);	/* count; *lines = the kind's lines */
const TextLine *text_pick(const Game *g, const Msg *m);	/* the line text_msg uses */
int             text_allowed(MsgKind k, const char *ph);	/* 1 if {ph} may appear in kind k */

#endif
