/* The recurring characters, and the list of every content string for the lint tests. */
#include "content/content.h"
#include "content/text.h"

const char *const COP_NAME = "Sgt. Buzzkill";
const char *const RUMOR_NAME = "Podcast Pete";

static int put(const char **out, int max, int n, const char *s)
{
	if (n < max)
		out[n] = s;
	return n + 1;
}

int content_strings(const char **out, int max)
{
	int n = 0, i, c;

	for (i = 0; i < NUM_GOODS; i++) {
		n = put(out, max, n, GOODS[i].name);
		n = put(out, max, n, GOODS[i].blurb);
	}
	for (c = 0; c < NUM_CITIES; c++) {
		n = put(out, max, n, CITIES[c].name);
		n = put(out, max, n, CITIES[c].nickname);
		n = put(out, max, n, CITIES[c].short_name);
		n = put(out, max, n, CITIES[c].code);
		n = put(out, max, n, CITIES[c].transit);
		n = put(out, max, n, CITIES[c].airport);
		n = put(out, max, n, CITIES[c].lender);
		for (i = 0; i < HOODS_PER_CITY; i++)
			n = put(out, max, n, CITIES[c].hoods[i]);
	}
	for (i = 0; i < NUM_CHEMS; i++) {
		n = put(out, max, n, CHEMS[i].name);
		n = put(out, max, n, CHEMS[i].blurb);
	}
	for (i = 0; i < NUM_GEAR; i++) {
		n = put(out, max, n, GEAR[i].name);
		n = put(out, max, n, GEAR[i].blurb);
	}
	n = put(out, max, n, COP_NAME);
	n = put(out, max, n, RUMOR_NAME);
	for (c = 0; c < NUM_MSG_KINDS; c++) {
		const TextLine *lines;
		int k = text_lines((MsgKind)c, &lines);
		for (i = 0; i < k; i++)
			n = put(out, max, n, lines[i].t);
	}
	return n;
}
