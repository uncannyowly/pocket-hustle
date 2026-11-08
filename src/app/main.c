/* pocket-hustle for the PC: the screens of src/ui on the SDL2 platform of src/plat.
 *   pocket-hustle [--data DIR] [--seed N] [--fullscreen] [--mute] [--param NAME=VALUE]...
 *                 [--frames N --keys "TOKENS" [--pace MS [--nowait]]] [--record FILE]
 * The scripted mode and the play-test options (src/app/main.c is shared by the PC and the Miyoo):
 *   --param    sets a wild card or crew param for the next new game (see params_set); repeatable
 *   --pace MS  a scripted press every MS ms in real time (--frames is still counted at the
 *              default pace of one press per SCRIPT_EVERY frames); --nowait skips the sleeping
 *   --record   writes the 320 x 240 canvas as raw RGB24, one frame per 20 Hz tick, to FILE or "-"
 *   --summary  writes the final state as one line to FILE when the run ends (see write_summary) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "gfx/canvas.h"
#include "plat/plat.h"
#include "sfx/mixer.h"
#include "ui/app.h"
#include "ui/input.h"
#include "ui/ui.h"

enum { TICK_MS = 50, MAX_CATCHUP = 5, SCRIPT_EVERY = 3, MAX_KEYS = 4096, MAX_PARAMS = 16 };

static Mixer mixer;

static void audio_cb(int16_t *stereo, int frames, void *user)
{
	(void)user;
	mixer_fill(&mixer, stereo, frames);
}

static void usage(void)
{
	fprintf(stderr, "usage: pocket-hustle [--data DIR] [--seed N] [--fullscreen] [--mute] [--param NAME=VALUE]... "
			"[--frames N --keys \"TOKENS\" [--pace MS [--nowait]]] [--record FILE] [--summary FILE]\n");
}

/* "A D START" to pad ids; the count, or -1 on an unknown token. */
static int parse_keys(const char *s, int *pads)
{
	char tok[16];
	int n = 0;

	for (;;) {
		size_t len;

		while (*s == ' ')
			s++;
		if (!*s)
			return n;
		for (len = 0; s[len] && s[len] != ' '; len++)
			;
		if (len >= sizeof tok || n >= MAX_KEYS)
			return -1;
		memcpy(tok, s, len);
		tok[len] = '\0';
		if ((pads[n] = input_token(tok)) < 0) {
			fprintf(stderr, "pocket-hustle: unknown key token \"%s\"\n", tok);
			return -1;
		}
		n++;
		s += len;
	}
}

typedef struct { char name[40]; long long value; } ParamArg;

/* "NAME=VALUE" into a ParamArg, checked against a default Params; 0, or -1 with the reason on stderr. */
static int parse_param(const char *s, ParamArg *out)
{
	const char *eq = strchr(s, '=');
	char *end;
	Params trial = params_default();

	if (!eq || eq == s || (size_t)(eq - s) >= sizeof out->name || !eq[1]) {
		fprintf(stderr, "pocket-hustle: --param wants NAME=VALUE, not \"%s\"\n", s);
		return -1;
	}
	memcpy(out->name, s, (size_t)(eq - s));
	out->name[eq - s] = '\0';
	out->value = strtoll(eq + 1, &end, 10);
	if (*end) {
		fprintf(stderr, "pocket-hustle: --param %s: \"%s\" is not a whole number\n", out->name, eq + 1);
		return -1;
	}
	if (params_set(&trial, out->name, out->value) != 0) {
		fprintf(stderr, "pocket-hustle: --param %s=%s: not a wild card param, or the value does not fit\n",
			out->name, eq + 1);
		return -1;
	}
	return 0;
}

/* The game's final state as one line, for the round scripts to check:
 * "day=6 city=0 cash=441 bank=0 debt=6613 over=0" (debt is everything owed, over 1 once the game has ended). */
static int write_summary(const char *path, const Game *g)
{
	FILE *f = fopen(path, "w");
	long long debt = 0;
	int c;

	if (!f)
		return -1;
	for (c = 0; c < NUM_CITIES; c++)
		debt += g->debt[c];
	fprintf(f, "day=%d city=%d cash=%lld bank=%lld debt=%lld over=%d\n", (int)g->day, (int)g->loc.city,
		(long long)g->cash, (long long)g->bank, debt, g->phase == PH_OVER);
	return fclose(f);
}

/* One canvas as RGB24 to f. */
static int write_rgb(FILE *f, const Canvas *cv)
{
	static uint8_t buf[CV_W * CV_H * 3];
	int i;

	for (i = 0; i < CV_W * CV_H; i++) {
		buf[i * 3] = (uint8_t)(cv->px[i] >> 16);
		buf[i * 3 + 1] = (uint8_t)(cv->px[i] >> 8);
		buf[i * 3 + 2] = (uint8_t)cv->px[i];
	}
	return fwrite(buf, 1, sizeof buf, f) == sizeof buf ? 0 : -1;
}

int main(int argc, char **argv)
{
	static App app;
	static Ui ui;
	static Canvas cv;
	char dir[512] = "";
	const char *data = NULL;
	int fullscreen = 0, mute = 0, scripted = 0, nkeys = 0, pads[MAX_KEYS];
	int nparams = 0, every = SCRIPT_EVERY, nowait = 0;
	long frames = 0, frame = 0, pace_ms = 0;
	ParamArg params[MAX_PARAMS];
	const char *record_path = NULL, *summary_path = NULL;
	FILE *record = NULL;
	uint64_t seed = (uint64_t)time(NULL);
	int i, ticks = 0, audio = 0, redraw = 1, quit = 0, status = 0;
	uint32_t last_tick, now, wall;
	Repeat rep;
	PlatEvents ev;
	Framebuffer *fb;

	memset(&rep, 0, sizeof rep);
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--data") && i + 1 < argc)
			data = argv[++i];
		else if (!strcmp(argv[i], "--seed") && i + 1 < argc)
			seed = strtoull(argv[++i], NULL, 10);
		else if (!strcmp(argv[i], "--fullscreen"))
			fullscreen = 1;
		else if (!strcmp(argv[i], "--mute"))
			mute = 1;
		else if (!strcmp(argv[i], "--frames") && i + 1 < argc)
			frames = atol(argv[++i]), scripted = 1;
		else if (!strcmp(argv[i], "--keys") && i + 1 < argc) {
			if ((nkeys = parse_keys(argv[++i], pads)) < 0)
				return 2;
		} else if (!strcmp(argv[i], "--param") && i + 1 < argc) {
			if (nparams >= MAX_PARAMS) {
				fprintf(stderr, "pocket-hustle: at most %d --param\n", (int)MAX_PARAMS);
				return 2;
			}
			if (parse_param(argv[++i], &params[nparams]) != 0)
				return 2;
			nparams++;
		} else if (!strcmp(argv[i], "--pace") && i + 1 < argc) {
			pace_ms = atol(argv[++i]);
			if (pace_ms < 1) {
				usage();
				return 2;
			}
		} else if (!strcmp(argv[i], "--nowait"))
			nowait = 1;
		else if (!strcmp(argv[i], "--record") && i + 1 < argc)
			record_path = argv[++i];
		else if (!strcmp(argv[i], "--summary") && i + 1 < argc)
			summary_path = argv[++i];
		else {
			usage();
			return 2;
		}
	}
	if ((nkeys && !scripted) || ((pace_ms || nowait) && !scripted) || (nowait && !pace_ms)) {
		usage();
		return 2;
	}

	if (!data) {
		if (app_default_dir(dir, sizeof dir) == 0)
			data = dir;
		else
			fprintf(stderr, "pocket-hustle: no data directory (no HOME); nothing will be saved\n");
	}
	if (app_init(&app, data) != 0)
		fprintf(stderr, "pocket-hustle: cannot use the data directory \"%s\"; nothing will be saved\n",
			data ? data : "");
	app_load(&app);
	app.next_seed = seed;
	for (i = 0; i < nparams; i++)
		params_set(&app.params, params[i].name, params[i].value);
	if (!params_sane(&app.params)) {
		fprintf(stderr, "pocket-hustle: the --param values together are not a playable setup\n");
		return 2;
	}
	if (pace_ms) {	/* a press every pace_ms: whole ticks, never faster than the default */
		every = (int)((pace_ms + TICK_MS / 2) / TICK_MS);
		if (every < SCRIPT_EVERY)
			every = SCRIPT_EVERY;
		frames = frames / SCRIPT_EVERY * every;
	}
	if (record_path) {
		record = !strcmp(record_path, "-") ? stdout : fopen(record_path, "wb");
		if (!record) {
			fprintf(stderr, "pocket-hustle: cannot write \"%s\"\n", record_path);
			return 2;
		}
	}
	ui_init(&ui, &app);

	if (plat_init(fullscreen) != 0)
		return 1;
	fb = plat_framebuffer();
	if (mixer_init(&mixer) == 0)
		audio = plat_audio_open(MIX_RATE, audio_cb, NULL) == 0;
	else
		fprintf(stderr, "pocket-hustle: no sound effects\n");

	now = plat_ms();
	last_tick = scripted ? 0 : now;	/* scripted: the virtual clock starts at 0, so ticks run (toasts end, the skyline moves) */
	wall = now;
	while (!quit) {
		uint32_t fire;
		int pressed = 0, b;

		plat_text_input(ui_wants_text(&ui) && !scripted);
		plat_poll(&ev);
		if (ev.quit)
			break;
		if (scripted) {	/* a virtual clock: one tick per frame, one press every few frames */
			now = (uint32_t)frame * TICK_MS;
			ev.held = 0;
			ev.nclicks = 0;
			ev.text[0] = '\0';
			if (frame % every == 0 && frame / every < nkeys)
				ev.held = PAD_BIT(pads[frame / every]);
		} else
			now = plat_ms();

		if (ev.text[0]) {	/* typed names, before the buttons: Enter then lands on START HUSTLING */
			ui_text(&ui, ev.text);
			pressed = 1;
		}
		fire = input_step(&rep, ev.held, now);
		for (b = 0; b < PAD_COUNT; b++)
			if (fire & PAD_BIT(b)) {
				ui_press(&ui, b);
				pressed = 1;
			}
		for (i = 0; i < ev.nclicks; i++) {
			if (pressed)	/* a press or click voids the hit regions: redraw so the next one lands */
				ui_draw(&ui, &cv);
			ui_click(&ui, ev.click[i].x / 2, ev.click[i].y / 2);
			pressed = 1;
		}
		if (pressed)
			redraw = 1;

		for (i = 0, ticks = 0; (int32_t)(now - last_tick) >= TICK_MS && i < MAX_CATCHUP; i++) {
			ui_tick(&ui);
			last_tick += TICK_MS;
			redraw = 1;
			ticks++;
		}
		if ((int32_t)(now - last_tick) >= TICK_MS)	/* fell far behind: do not replay it all */
			last_tick = now;

		if (audio) {
			int id;

			plat_audio_lock();
			mixer.muted = mute || !app.sound_on;
			while ((id = ui_take_sfx(&ui)) >= 0)
				mixer_play(&mixer, (SfxId)id);
			plat_audio_unlock();
		} else
			while (ui_take_sfx(&ui) >= 0)
				;

		if (ui_quit(&ui))
			quit = 1;
		if (redraw) {
			ui_draw(&ui, &cv);
			cv_scale2x(&cv, fb->pixels, fb->pitch);
			plat_present();
			redraw = 0;
		} else
			plat_sleep_ms(2);
		for (i = 0; record && i < ticks; i++)	/* one recorded frame per 20 Hz tick */
			if (write_rgb(record, &cv) != 0) {
				fprintf(stderr, "pocket-hustle: the recording could not be written\n");
				fclose(record);
				record = NULL;
				status = 1;
			}
		if (scripted && pace_ms && !nowait) {	/* real time: sleep out the rest of this frame */
			uint32_t spent = plat_ms() - wall;

			if (spent < TICK_MS)
				plat_sleep_ms((int)(TICK_MS - spent));
			wall = plat_ms();
		}

		frame++;
		if (scripted && frame >= frames)
			break;
	}

	if (app.has_game && (app_save_now(&app) != 0 || app.io_failed)) {
		fprintf(stderr, "pocket-hustle: the game could not be saved\n");
		status = 1;
	}
	if (summary_path && (app.has_game || app.g.phase == PH_OVER) && write_summary(summary_path, &app.g) != 0) {
		fprintf(stderr, "pocket-hustle: cannot write \"%s\"\n", summary_path);
		status = 1;
	}
	if (record && fclose(record) != 0)
		status = 1;
	plat_audio_close();
	mixer_free(&mixer);
	plat_shutdown();
	return status;
}
