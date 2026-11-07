#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "check.h"
#include "tests.h"
#include "ui/app.h"

static char tmp[64];

static void fresh_dir(void)
{
	system("mkdir -p build");
	snprintf(tmp, sizeof tmp, "build/app-test-XXXXXX");
	if (!mkdtemp(tmp))
		tmp[0] = '\0';
}

static void put_file(const char *name, const char *text)
{
	char path[600];
	FILE *f;
	snprintf(path, sizeof path, "%s/%s", tmp, name);
	f = fopen(path, "w");
	if (f) {
		fputs(text, f);
		fclose(f);
	}
}

static void save_env(const char *k, char *out, size_t n, int *had)
{
	const char *v = getenv(k);
	*had = v != NULL;
	snprintf(out, n, "%s", v ? v : "");
}

static void restore_env(const char *k, const char *v, int had)
{
	if (had)
		setenv(k, v, 1);
	else
		unsetenv(k);
}

static void test_default_dir_xdg(void)
{
	char x[512], h[512], out[512];
	int hx, hh;
	save_env("XDG_DATA_HOME", x, sizeof x, &hx);
	save_env("HOME", h, sizeof h, &hh);
	setenv("XDG_DATA_HOME", "/tmp/xdgtest", 1);
	setenv("HOME", "/tmp/hometest", 1);
	CHECK_EQ(app_default_dir(out, sizeof out), 0);
	CHECK_STR(out, "/tmp/xdgtest/pocket-hustle");
	restore_env("XDG_DATA_HOME", x, hx);
	restore_env("HOME", h, hh);
}

static void test_default_dir_home(void)
{
	char x[512], h[512], out[512];
	int hx, hh;
	save_env("XDG_DATA_HOME", x, sizeof x, &hx);
	save_env("HOME", h, sizeof h, &hh);
	unsetenv("XDG_DATA_HOME");
	setenv("HOME", "/tmp/hometest", 1);
	CHECK_EQ(app_default_dir(out, sizeof out), 0);
	CHECK_STR(out, "/tmp/hometest/.local/share/pocket-hustle");
	restore_env("XDG_DATA_HOME", x, hx);
	restore_env("HOME", h, hh);
}

static void test_load_empty_dir(void)
{
	static App a;
	fresh_dir();
	CHECK_EQ(app_init(&a, tmp), 0);
	app_load(&a);
	CHECK_EQ(a.has_game, 0);
	CHECK_EQ(a.sound_on, 1);
	CHECK_EQ(a.last_rank, -1);
	CHECK_EQ(a.scores.n[0] + a.scores.n[1] + a.scores.n[2] + a.scores.n[3], 0);
}

static void test_resume_after_move(void)
{
	static App a, b;
	Loc dest;
	fresh_dir();
	app_init(&a, tmp);
	CHECK_EQ(app_new_game(&a, 30, "Tester"), 0);
	dest = a.g.loc;
	dest.hood = (uint8_t)((dest.hood + 1) % HOODS_PER_CITY);
	CHECK_EQ(game_move(&a.g, dest), 0);
	CHECK_EQ(app_after_action(&a, 1, PH_TRADING), -1);
	app_init(&b, tmp);
	app_load(&b);
	CHECK_EQ(b.has_game, 1);
	CHECK_EQ(b.g.day, a.g.day);
	CHECK_EQ(b.g.cash, a.g.cash);
}

static void test_record_once(void)
{
	static App a, b;
	char path[600];
	int n = 0;
	fresh_dir();
	app_init(&a, tmp);
	app_new_game(&a, 15, "Tester");
	while (a.g.phase != PH_OVER && n++ < 200) {
		if (a.g.phase == PH_CHASE) {
			game_chase(&a.g, CH_SURRENDER);
		} else {
			Loc d = a.g.loc;
			d.hood = (uint8_t)((d.hood + 1) % HOODS_PER_CITY);
			game_move(&a.g, d);
		}
	}
	CHECK_EQ(a.g.phase, PH_OVER);
	CHECK(app_after_action(&a, 1, PH_TRADING) >= -1);
	CHECK_EQ(a.recorded, 1);
	CHECK_EQ(a.has_game, 0);
	CHECK_EQ(a.scores.n[0], 1);
	CHECK_EQ(app_after_action(&a, 1, PH_TRADING), -1);
	CHECK_EQ(a.scores.n[0], 1);
	snprintf(path, sizeof path, "%s/save.dat", tmp);
	CHECK(access(path, F_OK) != 0);
	app_init(&b, tmp);
	app_load(&b);
	CHECK_EQ(b.scores.n[0], 1);
	CHECK_EQ(b.has_game, 0);
}

static void test_new_game_seeds_advance(void)
{
	static App a;
	uint64_t s1, s2;
	app_init(&a, NULL);
	CHECK_STR(a.save_path, "");
	app_new_game(&a, 30, "A");
	s1 = a.g.seed;
	app_new_game(&a, 30, "A");
	s2 = a.g.seed;
	CHECK(s1 != s2);
}

static void test_options_round_trip(void)
{
	static App a, b;
	fresh_dir();
	app_init(&a, tmp);
	a.sound_on = 0;
	CHECK_EQ(app_options_write(&a), 0);
	app_init(&b, tmp);
	app_load(&b);
	CHECK_EQ(b.sound_on, 0);
}

static void test_options_garbage(void)
{
	static App a;
	fresh_dir();
	put_file("options.txt", "\x01\x02 junk without equals\n\xff\xfe");
	app_init(&a, tmp);
	app_load(&a);
	CHECK_EQ(a.sound_on, 1);
}

void tests_app(void)
{
	test_default_dir_xdg();
	test_default_dir_home();
	test_load_empty_dir();
	test_resume_after_move();
	test_record_once();
	test_new_game_seeds_advance();
	test_options_round_trip();
	test_options_garbage();
}
