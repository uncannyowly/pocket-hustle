/* ph-sim: bots play many games; the report shows how the rules feel, and with
 * --targets compares that against numbers observed in another game.
 *   ph-sim --games N --seed S --days D --bot greedy|cautious [--targets FILE] [--strict] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bots.h"
#include "report.h"

typedef struct { int32_t *v; size_t n, cap; } Vec;

static void vpush(Vec *a, int32_t x)
{
	if (a->n == a->cap) {
		a->cap = a->cap ? a->cap * 2 : 1024;
		a->v = realloc(a->v, a->cap * sizeof *a->v);
		if (!a->v) { perror("realloc"); exit(1); }
	}
	a->v[a->n++] = x;
}

static int cmp_i32(const void *a, const void *b)
{
	int32_t x = *(const int32_t *)a, y = *(const int32_t *)b;
	return x < y ? -1 : x > y;
}

static int cmp_i64(const void *a, const void *b)
{
	int64_t x = *(const int64_t *)a, y = *(const int64_t *)b;
	return x < y ? -1 : x > y;
}

typedef struct { char key[64]; double v; } Kv;
static Kv ours[64];
static int nours;

static void put(const char *key, double v, const char *fmt)
{
	snprintf(ours[nours].key, sizeof ours[nours].key, "%s", key);
	ours[nours++].v = v;
	printf("%s ", key);
	printf(fmt, v);
	printf("\n");
}

static double find_ours(const char *key, int *found)
{
	int i;
	for (i = 0; i < nours; i++)
		if (!strcmp(ours[i].key, key)) {
			*found = 1;
			return ours[i].v;
		}
	*found = 0;
	return 0;
}

static int must_differ(const char *key)
{
	return !strcmp(key, "start_cash") || !strcmp(key, "start_debt") ||
	       !strcmp(key, "debt_daily_pct") || !strcmp(key, "bank_daily_pct") || !strcmp(key, "capacity");
}

static int bad;

static void cmp_line(const char *key, double o, double t, int differ)
{
	Verdict v = compare_value(o, t, differ);
	if (v != V_OK)
		bad++;
	printf("cmp %s ours=%.3f theirs=%.3f %s\n", key, o, t, verdict_name(v));
}

static void compare_targets(const char *path, Vec *prices)
{
	char line[256];
	FILE *f = fopen(path, "r");

	if (!f) {
		perror(path);
		exit(2);
	}
	while (fgets(line, sizeof line, f)) {
		char key[64], name[256];
		double t;
		int found, i;

		line[strcspn(line, "\r\n")] = 0;
		if (!strncmp(line, "price ", 6)) {
			char *hi_s = strrchr(line, ' '), *lo_s;
			if (!hi_s) continue;
			*hi_s++ = 0;
			lo_s = strrchr(line, ' ');
			if (!lo_s || lo_s < line + 6) continue;
			*lo_s++ = 0;
			snprintf(name, sizeof name, "%s", line + 6);
			for (i = 0; i < NUM_GOODS; i++) {
				char k[96];
				if (!target_matches_good(name, GOODS[i].name) || prices[i].n == 0)
					continue;
				snprintf(k, sizeof k, "price_lo:%s", GOODS[i].name);
				cmp_line(k, prices[i].v[0], atof(lo_s), 1);
				snprintf(k, sizeof k, "price_hi:%s", GOODS[i].name);
				cmp_line(k, prices[i].v[prices[i].n - 1], atof(hi_s), 1);
			}
			continue;
		}
		if (sscanf(line, "%63s %lf", key, &t) != 2)
			continue;
		{
			double o = find_ours(key, &found);
			if (found)
				cmp_line(key, o, t, must_differ(key));
		}
	}
	fclose(f);
}

int main(int argc, char **argv)
{
	Params p = params_default();
	int games = 1000, days = 30, strict = 0, i, g, k;
	uint64_t seed = 1;
	BotKind bot = BOT_GREEDY;
	const char *targets = NULL;
	int64_t *scores;
	Vec prices[NUM_GOODS];
	double moves = 0, chases = 0, spikes = 0, crashes = 0, mugged = 0, goons = 0, jailed = 0, dead = 0, sum = 0;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--games") && i + 1 < argc) games = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--seed") && i + 1 < argc) seed = strtoull(argv[++i], NULL, 10);
		else if (!strcmp(argv[i], "--days") && i + 1 < argc) days = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--bot") && i + 1 < argc) {
			i++;
			if (!strcmp(argv[i], "greedy")) bot = BOT_GREEDY;
			else if (!strcmp(argv[i], "cautious")) bot = BOT_CAUTIOUS;
			else { fprintf(stderr, "unknown bot %s\n", argv[i]); return 2; }
		} else if (!strcmp(argv[i], "--targets") && i + 1 < argc) targets = argv[++i];
		else if (!strcmp(argv[i], "--strict")) strict = 1;
		else {
			fprintf(stderr, "usage: ph-sim --games N --seed S --days D --bot greedy|cautious [--targets FILE] [--strict]\n");
			return 2;
		}
	}
	if (games < 1) games = 1;
	scores = calloc((size_t)games, sizeof *scores);
	memset(prices, 0, sizeof prices);

	for (g = 0; g < games; g++) {
		static Game gm;
		int steps = 0, last_day = 0;
		if (game_new(&gm, &p, seed + (uint64_t)g, days, "Bot") != OK) {
			fprintf(stderr, "--days must be 15, 30, 60 or 90\n");
			return 2;
		}
		while (gm.phase != PH_OVER && steps++ < 100000) {
			if (gm.day != last_day) {
				for (k = 0; k < NUM_GOODS; k++)
					if (gm.price[k] > 0)
						vpush(&prices[k], gm.price[k]);
				last_day = gm.day;
			}
			bot_play_day(&gm, bot);
		}
		scores[g] = game_net_worth(&gm);
		sum += (double)scores[g];
		dead += gm.end == END_DEAD;
		moves += gm.stats.moves;
		chases += gm.stats.cop_chases;
		spikes += gm.stats.spikes_seen;
		crashes += gm.stats.crashes_seen;
		mugged += gm.stats.times_mugged;
		goons += gm.stats.goon_visits;
		jailed += gm.stats.times_jailed;
	}
	qsort(scores, (size_t)games, sizeof *scores, cmp_i64);
	for (k = 0; k < NUM_GOODS; k++)
		if (prices[k].n)
			qsort(prices[k].v, prices[k].n, sizeof *prices[k].v, cmp_i32);
	if (moves < 1)
		moves = 1;

	printf("games %d\ndays %d\nbot %s\n", games, days, bot == BOT_GREEDY ? "greedy" : "cautious");
	put("score_p10", (double)scores[games / 10], "%.0f");
	put("score_p50", (double)scores[games / 2], "%.0f");
	put("score_p90", (double)scores[games * 9 / 10], "%.0f");
	put("score_mean", sum / games, "%.0f");
	put("dead_pct", 100.0 * dead / games, "%.1f");
	put("cop_chases", chases / games, "%.2f");
	put("spikes", spikes / games, "%.2f");
	put("crashes", crashes / games, "%.2f");
	put("mugged", mugged / games, "%.2f");
	put("goons", goons / games, "%.2f");
	put("jailed", jailed / games, "%.2f");
	put("start_cash", (double)p.start_cash, "%.0f");
	put("start_debt", (double)p.start_debt, "%.0f");
	put("debt_daily_pct", CITIES[p.start_city].lender_rate_bp / 100.0, "%.2f");
	put("bank_daily_pct", p.bank_rate_bp / 100.0, "%.2f");
	put("capacity", (double)p.start_capacity, "%.0f");
	put("spikes_per_move", spikes / moves, "%.3f");
	put("crashes_per_move", crashes / moves, "%.3f");
	put("cops_per_move", chases / moves, "%.3f");
	put("mugs_per_move", mugged / moves, "%.3f");
	for (k = 0; k < NUM_GOODS; k++) {
		Vec *a = &prices[k];
		if (!a->n)
			printf("price %s - - -\n", GOODS[k].name);
		else
			printf("price %s %d %d %d\n", GOODS[k].name, (int)a->v[0], (int)a->v[a->n / 2], (int)a->v[a->n - 1]);
	}
	if (targets)
		compare_targets(targets, prices);
	return strict && bad ? 1 : 0;
}
