#include "check.h"
#include "tests.h"
#include "helpers.h"
#include "content/text.h"

static void test_money_format(void)
{
	char b[64];
	fmt_money(0, b, sizeof b);
	CHECK_STR(b, "$0");
	fmt_money(1234, b, sizeof b);
	CHECK_STR(b, "$1,234");
	fmt_money(-5500, b, sizeof b);
	CHECK_STR(b, "-$5,500");
	fmt_money(999, b, sizeof b);
	CHECK_STR(b, "$999");
	fmt_money(MONEY_MAX, b, sizeof b);
	CHECK_STR(b, "$1,000,000,000,000,000");
}

static void game_for_text(Game *g)
{
	Params p = test_params();
	new_test_game(g, &p, 30);
}

static void test_every_kind_has_text(void)
{
	Game g;
	Msg m;
	char b[256];
	int k;
	game_for_text(&g);
	for (k = 0; k < NUM_MSG_KINDS; k++) {
		memset(&m, 0, sizeof m);
		m.kind = (uint8_t)k;
		m.amount = 1;
		CHECK(text_msg(&g, &m, b, sizeof b) > 0);
		if (strchr(b, '{') || strchr(b, '}'))
			fprintf(stderr, "kind %d left a placeholder: %s\n", k, b);
		CHECK(!strchr(b, '{') && !strchr(b, '}'));
	}
}

static void test_placeholders_allowed(void)
{
	int k, i;
	for (k = 0; k < NUM_MSG_KINDS; k++) {
		const TextLine *lines;
		int n = text_lines((MsgKind)k, &lines);
		for (i = 0; i < n; i++) {
			const char *s = lines[i].t;
			while ((s = strchr(s, '{')) != NULL) {
				char ph[32];
				const char *e = strchr(s, '}');
				size_t len = e ? (size_t)(e - s - 1) : 0;
				CHECK(e != NULL && len > 0 && len < sizeof ph);
				if (!e || len == 0 || len >= sizeof ph)
					break;
				memcpy(ph, s + 1, len);
				ph[len] = 0;
				if (!text_allowed((MsgKind)k, ph))
					fprintf(stderr, "kind %d: {%s} not allowed in: %s\n", k, ph, lines[i].t);
				CHECK(text_allowed((MsgKind)k, ph));
				s = e + 1;
			}
		}
	}
}

static int count_city(MsgKind k, int city)
{
	const TextLine *lines;
	int n = text_lines(k, &lines), i, c = 0;
	for (i = 0; i < n; i++)
		if (lines[i].city == city)
			c++;
	return c;
}

static void test_line_counts(void)
{
	static const MsgKind three[] = {
		M_SPIKE, M_CRASH, M_COPS, M_ESCAPED, M_MUGGED, M_GOONS, M_JAILED, M_DROPPED,
		M_HIT_YOU, M_YOU_MISSED,
	};
	const TextLine *lines;
	size_t i;
	int k, c;
	for (k = 0; k < NUM_MSG_KINDS; k++)
		CHECK(text_lines((MsgKind)k, &lines) >= 1);
	for (i = 0; i < sizeof three / sizeof three[0]; i++)
		CHECK(text_lines(three[i], &lines) >= 3);
	CHECK(count_city(M_RUMOR_JOKE, -1) >= 14);
	for (c = 0; c < NUM_CITIES; c++)
		CHECK(count_city(M_RUMOR_JOKE, c) >= 3);
	for (k = 0; k < NUM_MSG_KINDS; k++)
		if (k != M_RUMOR_JOKE)
			for (c = 0; c < NUM_CITIES; c++)
				CHECK_EQ(count_city((MsgKind)k, c), 0);
}

static void test_city_jokes(void)
{
	Game g;
	Msg m;
	int v, ok = 1, miami = 0;
	game_for_text(&g);
	memset(&m, 0, sizeof m);
	m.kind = M_RUMOR_JOKE;
	m.a = 4;
	for (v = 0; v < 1000; v++) {
		const TextLine *l;
		m.variant = (uint16_t)v;
		l = text_pick(&g, &m);
		if (!l || (l->city != -1 && l->city != 4))
			ok = 0;
		if (l && l->city == 4)
			miami = 1;
	}
	CHECK(ok);
	CHECK(miami);
}

static int a_range(MsgKind k)
{
	switch (k) {
	case M_SPIKE: case M_CRASH: case M_DROPPED: case M_RUMOR_TIP: return NUM_GOODS;
	case M_FLEW: case M_GOONS: case M_RUMOR_JOKE: return NUM_CITIES;
	case M_RODE: return HOODS_PER_CITY;
	case M_BOUGHT_GEAR: return NUM_GEAR;
	default: return 1;
	}
}

static void test_lengths_worst_case(void)
{
	Game g;
	Msg m;
	char b[512];
	int k, city, hood, a, bi, v, worst = 0;
	game_for_text(&g);
	strcpy(g.name, "WWWWWWWWWWWWWWW");
	for (k = 0; k < NUM_MSG_KINDS; k++) {
		const TextLine *lines;
		int n = text_lines((MsgKind)k, &lines);
		int nb = k == M_RUMOR_TIP ? NUM_LOCS : 1;
		for (city = 0; city < NUM_CITIES; city++)
			for (hood = 0; hood < HOODS_PER_CITY; hood++)
				for (a = 0; a < a_range((MsgKind)k); a++)
					for (bi = 0; bi < nb; bi++)
						for (v = 0; v < n * NUM_CITIES; v++) {
							size_t len;
							g.loc.city = (uint8_t)city;
							g.loc.hood = (uint8_t)hood;
							memset(&m, 0, sizeof m);
							m.kind = (uint8_t)k;
							m.a = (int16_t)(a_range((MsgKind)k) > 1 ? a : 32767);
							m.b = (int16_t)bi;
							m.c = 1;
							m.amount = MONEY_MAX;
							m.variant = (uint16_t)v;
							len = text_msg(&g, &m, b, sizeof b);
							if ((int)len > worst) worst = (int)len;
							if (len > 120)
								fprintf(stderr, "too long (%d): %s\n", (int)len, b);
						}
	}
	CHECK(worst <= 120);
}

void tests_text(void)
{
	test_money_format();
	test_every_kind_has_text();
	test_placeholders_allowed();
	test_line_counts();
	test_city_jokes();
	test_lengths_worst_case();
}
