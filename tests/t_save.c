#include <stdlib.h>
#include <unistd.h>

#include "check.h"
#include "tests.h"
#include "helpers.h"
#include "core/save.h"

static char dir[64];

static void path_in(char *out, size_t n, const char *name)
{
	snprintf(out, n, "%s/%s", dir, name);
}

static long read_file(const char *path, unsigned char *buf, long max)
{
	FILE *f = fopen(path, "rb");
	long n;
	if (!f)
		return -1;
	n = (long)fread(buf, 1, (size_t)max, f);
	fclose(f);
	return n;
}

static void write_bytes(const char *path, const unsigned char *buf, long n)
{
	FILE *f = fopen(path, "wb");
	if (!f)
		return;
	if (n > 0)
		fwrite(buf, 1, (size_t)n, f);
	fclose(f);
}

static Loc at(int city, int hood)
{
	Loc l;
	l.city = (uint8_t)city;
	l.hood = (uint8_t)hood;
	return l;
}

static void played_game(Game *g)
{
	Params p = test_params();
	p.spike_permille = 200;
	p.crash_permille = 200;
	p.rumor_permille = 500;
	game_new(g, &p, 7, 30, "Saver");
	g->cash = 100000;
	game_buy(g, 0, 3);
	game_move(g, at(g->loc.city, 1));
	game_move(g, at(2, 0));
	game_move(g, at(2, 3));
}

static void test_crc32_check_value(void)
{
	CHECK_EQ(crc32_buf("123456789", 9), 0xCBF43926u);
}

static void test_save_round_trip(void)
{
	static unsigned char a[200000], b[200000];
	char p1[128], p2[128];
	Game g, g2;
	long na, nb;
	path_in(p1, sizeof p1, "one.sav");
	path_in(p2, sizeof p2, "two.sav");
	played_game(&g);
	CHECK_EQ(save_write(&g, p1), 0);
	memset(&g2, 0x5a, sizeof g2);
	CHECK_EQ(save_read(&g2, p1), 0);
	CHECK_EQ(g2.nmsgs, 0);
	CHECK_EQ(save_write(&g2, p2), 0);
	na = read_file(p1, a, sizeof a);
	nb = read_file(p2, b, sizeof b);
	CHECK(na > 16);
	CHECK_EQ(na, nb);
	CHECK(na == nb && memcmp(a, b, (size_t)na) == 0);
	CHECK_EQ(game_move(&g, at(2, 5)), OK);
	CHECK_EQ(game_move(&g2, at(2, 5)), OK);
	CHECK(memcmp(g.price, g2.price, sizeof g.price) == 0);
	CHECK_EQ(g.cash, g2.cash);
	CHECK_EQ(g.day, g2.day);
}

/* Fields added after the first save format must be written and read, not left at zero. */
static void test_save_keeps_bank_fee_fields(void)
{
	char p1[128];
	static Game g, g2;
	path_in(p1, sizeof p1, "fee.sav");
	played_game(&g);
	g.p.bank_fee_pct = 7;
	g.p.bank_fee_min = 11;
	g.stats.bank_fees_paid = 123;
	CHECK_EQ(save_write(&g, p1), 0);
	memset(&g2, 0, sizeof g2);
	CHECK_EQ(save_read(&g2, p1), 0);
	CHECK_EQ(g2.p.bank_fee_pct, 7);
	CHECK_EQ(g2.p.bank_fee_min, 11);
	CHECK_EQ(g2.stats.bank_fees_paid, 123);
}

static void test_truncated_save(void)
{
	static unsigned char a[200000];
	char p1[128], p2[128];
	Game g, target, snap;
	long n, len, ok = 1;
	path_in(p1, sizeof p1, "full.sav");
	path_in(p2, sizeof p2, "cut.sav");
	played_game(&g);
	save_write(&g, p1);
	n = read_file(p1, a, sizeof a);
	played_game(&target);
	memcpy(&snap, &target, sizeof snap);
	for (len = 0; len < n; len += 7) {
		write_bytes(p2, a, len);
		if (save_read(&target, p2) != -2)
			ok = 0;
	}
	CHECK(ok);
	CHECK(same_game(&target, &snap));
}

static void test_bitflip_save(void)
{
	static unsigned char a[200000];
	char p1[128];
	Game g, target, snap;
	long n;
	path_in(p1, sizeof p1, "flip.sav");
	played_game(&g);
	save_write(&g, p1);
	n = read_file(p1, a, sizeof a);
	a[16 + 100] ^= 0x01;
	write_bytes(p1, a, n);
	played_game(&target);
	memcpy(&snap, &target, sizeof snap);
	CHECK_EQ(save_read(&target, p1), -2);
	CHECK(same_game(&target, &snap));
}

static void test_other_version(void)
{
	static unsigned char a[200000];
	char p1[128];
	Game g;
	long n;
	path_in(p1, sizeof p1, "ver.sav");
	played_game(&g);
	save_write(&g, p1);
	n = read_file(p1, a, sizeof a);
	a[4] = (unsigned char)(a[4] + 1);
	write_bytes(p1, a, n);
	CHECK_EQ(save_read(&g, p1), -2);
}

/* A save whose CRC is right but whose values could never come from the rules (edited by
 * hand) must be rejected, or play would overflow. */
static void test_save_rejects_out_of_range_values(void)
{
	char p1[128];
	Game g, target;
	int k, ok = 1;
	path_in(p1, sizeof p1, "range.sav");
	for (k = 0; k < 11; k++) {
		played_game(&g);
		switch (k) {
		case 0: g.cash = -5; break;
		case 1: g.debt[0] = MONEY_MAX + 1; break;
		case 2: g.bank = -1; break;
		case 3: g.health = 500; break;
		case 4: g.stash[0] = -1; break;
		case 5: g.p.mug_permille = 5000; break;
		case 6: g.p.flight_lo = 900; g.p.flight_hi = 100; break;
		case 7: g.capacity = 2000000000; break;
		case 8: g.p.bank_fee_pct = 101; break;
		case 9: g.p.bank_fee_min = -1; break;
		case 10: g.stats.bank_fees_paid = -1; break;
		}
		save_write(&g, p1);
		played_game(&target);
		if (save_read(&target, p1) != -2) {
			fprintf(stderr, "mutation %d loaded\n", k);
			ok = 0;
		}
	}
	CHECK(ok);
	played_game(&g);
	save_write(&g, p1);
	CHECK_EQ(save_read(&target, p1), 0);	/* an honest save still loads */
}

static void test_save_keeps_events(void)
{
	char p1[128];
	static Game g, g2;

	path_in(p1, sizeof p1, "ev.sav");
	played_game(&g);
	g.hist_ev[2][1] = EV_SPIKE;
	g.hist_ev[3][2] = EV_CRASH;
	CHECK_EQ(save_write(&g, p1), 0);
	memset(&g2, 0, sizeof g2);
	CHECK_EQ(save_read(&g2, p1), 0);
	CHECK(memcmp(g.hist_ev, g2.hist_ev, sizeof g.hist_ev) == 0);
	CHECK_EQ(g2.hist_ev[2][1], EV_SPIKE);
	CHECK_EQ(g2.hist_ev[3][2], EV_CRASH);
	g.hist_ev[4][3] = 3;			/* not an event: refused */
	CHECK_EQ(save_write(&g, p1), 0);
	CHECK_EQ(save_read(&g2, p1), -2);
	g.hist_ev[4][3] = -1;
	CHECK_EQ(save_write(&g, p1), 0);
	CHECK_EQ(save_read(&g2, p1), -2);
}

static void test_missing_save(void)
{
	char p1[128];
	Game g;
	path_in(p1, sizeof p1, "nope.sav");
	CHECK_EQ(save_read(&g, p1), -1);
}

static void test_hiscores_order(void)
{
	HiTable t;
	int i = days_index(30);
	memset(&t, 0, sizeof t);
	CHECK_EQ(i, 1);
	CHECK_EQ(hiscore_insert(&t, 30, "a", 500, END_DAYS_UP), 0);
	CHECK_EQ(hiscore_insert(&t, 30, "b", -2000, END_DEAD), 1);
	CHECK_EQ(hiscore_insert(&t, 30, "c", 9000, END_DAYS_UP), 0);
	CHECK_EQ(t.n[i], 3);
	CHECK_EQ(t.t[i][0].score, 9000);
	CHECK_EQ(t.t[i][1].score, 500);
	CHECK_EQ(t.t[i][2].score, -2000);
	CHECK_STR(t.t[i][2].name, "b");
	CHECK_EQ(t.t[i][2].end, END_DEAD);
	CHECK_EQ(hiscore_insert(&t, 20, "x", 1, END_DAYS_UP), -1);
	CHECK_EQ(days_index(15), 0);
	CHECK_EQ(days_index(90), 3);
}

static void test_hiscores_ties_keep_earlier(void)
{
	HiTable t;
	memset(&t, 0, sizeof t);
	hiscore_insert(&t, 15, "first", 100, END_DAYS_UP);
	CHECK_EQ(hiscore_insert(&t, 15, "second", 100, END_DAYS_UP), 1);
	CHECK_STR(t.t[0][0].name, "first");
}

static void test_hiscores_top_ten(void)
{
	HiTable t;
	int i;
	memset(&t, 0, sizeof t);
	for (i = 0; i < 11; i++)
		hiscore_insert(&t, 60, "p", (i + 1) * 100, END_DAYS_UP);
	CHECK_EQ(t.n[2], 10);
	CHECK_EQ(t.t[2][0].score, 1100);
	CHECK_EQ(t.t[2][9].score, 200);
	CHECK_EQ(hiscore_insert(&t, 60, "low", 150, END_DAYS_UP), -1);
	CHECK_EQ(t.t[2][9].score, 200);
}

static void test_hiscores_round_trip(void)
{
	HiTable t, u;
	char p1[128];
	path_in(p1, sizeof p1, "scores.dat");
	memset(&t, 0, sizeof t);
	hiscore_insert(&t, 90, "Seth", 123456, END_DAYS_UP);
	hiscore_insert(&t, 15, "Corbin", -50, END_DEAD);
	CHECK_EQ(hiscores_write(&t, p1), 0);
	CHECK_EQ(hiscores_read(&u, p1), 0);
	CHECK(memcmp(t.n, u.n, sizeof t.n) == 0);
	CHECK_EQ(u.t[3][0].score, 123456);
	CHECK_STR(u.t[0][0].name, "Corbin");
}

static void test_hiscores_corrupt_file(void)
{
	static const unsigned char junk[] = "this is not a high score file at all";
	HiTable t;
	char p1[128];
	size_t i;
	int zero = 1;
	path_in(p1, sizeof p1, "junk.dat");
	write_bytes(p1, junk, (long)sizeof junk);
	memset(&t, 0x77, sizeof t);
	CHECK(hiscores_read(&t, p1) < 0);
	for (i = 0; i < sizeof t; i++)
		if (((unsigned char *)&t)[i]) zero = 0;
	CHECK(zero);
}

static void test_atomic_write_no_temp_left(void)
{
	char p1[128], tmp[160];
	path_in(p1, sizeof p1, "atomic.bin");
	snprintf(tmp, sizeof tmp, "%s.tmp", p1);
	CHECK_EQ(atomic_write_file(p1, "hello", 5), 0);
	CHECK(access(p1, F_OK) == 0);
	CHECK(access(tmp, F_OK) != 0);
}

void tests_save(void)
{
	snprintf(dir, sizeof dir, "build/tmp-save-XXXXXX");
	if (!mkdtemp(dir)) {
		fprintf(stderr, "mkdtemp failed\n");
		CHECK(0);
		return;
	}
	test_crc32_check_value();
	test_save_round_trip();
	test_save_keeps_bank_fee_fields();
	test_truncated_save();
	test_bitflip_save();
	test_other_version();
	test_save_keeps_events();
	test_missing_save();
	test_save_rejects_out_of_range_values();
	test_hiscores_order();
	test_hiscores_ties_keep_earlier();
	test_hiscores_top_ten();
	test_hiscores_round_trip();
	test_hiscores_corrupt_file();
	test_atomic_write_no_temp_left();
}
