/* Checks that plat_poll hands back every click, in order, and keeps the extras queued:
 * pushes SDL mouse events (run with the dummy video driver) and polls. */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#include "plat/plat.h"

static int fails;

static void push_click(int x, int y)
{
	SDL_Event e;

	SDL_zero(e);
	e.type = SDL_MOUSEBUTTONDOWN;
	e.button.button = SDL_BUTTON_LEFT;
	e.button.x = x;
	e.button.y = y;
	SDL_PushEvent(&e);
	e.type = SDL_MOUSEBUTTONUP;
	SDL_PushEvent(&e);
}

static void expect(int cond, const char *what)
{
	if (!cond) {
		fprintf(stderr, "clicks: FAIL %s\n", what);
		fails++;
	}
}

static void push_key(SDL_Scancode sc)
{
	SDL_Event e;

	SDL_zero(e);
	e.type = SDL_KEYDOWN;
	e.key.keysym.scancode = sc;
	e.key.keysym.sym = SDL_GetKeyFromScancode(sc);
	SDL_PushEvent(&e);
	e.type = SDL_KEYUP;
	SDL_PushEvent(&e);
}

static void push_text(const char *t)
{
	SDL_Event e;

	SDL_zero(e);
	e.type = SDL_TEXTINPUT;
	e.text.windowID = 1;
	snprintf(e.text.text, sizeof e.text.text, "%s", t);
	SDL_PushEvent(&e);
}

/* Some SDL2 builds (sdl2-compat over SDL3) crash when a synthetic SDL_TEXTINPUT is pushed and
 * polled. Probe that in a child process first; 1 when pushed text can be polled safely. */
static int text_push_works(void)
{
	pid_t pid = fork();
	int st = 0;

	if (pid == 0) {
		SDL_Event e;

		SDL_zero(e);
		e.type = SDL_TEXTINPUT;
		e.text.text[0] = 'm';
		SDL_PushEvent(&e);
		while (SDL_PollEvent(&e))
			;
		_exit(0);
	}
	if (pid < 0 || waitpid(pid, &st, 0) < 0)
		return 0;
	return WIFEXITED(st) && WEXITSTATUS(st) == 0;
}

/* With text input on, typed characters arrive as text and the keys that make them are not
 * buttons (M is START otherwise). Enter and the arrows stay buttons. */
static void check_text_input(void)
{
	PlatEvents ev;
	int text_ok = text_push_works();

	if (!text_ok)
		puts("clicks: this SDL cannot poll a pushed SDL_TEXTINPUT (sdl2-compat); text arrival check skipped");
	plat_text_input(1);
	plat_poll(&ev);
	if (text_ok)
		push_text("m");
	push_key(SDL_SCANCODE_M);
	push_key(SDL_SCANCODE_SPACE);
	push_key(SDL_SCANCODE_Y);
	plat_poll(&ev);
	if (text_ok)
		expect(strcmp(ev.text, "m") == 0, "text input: the typed m arrives as text");
	expect(!(ev.held & PAD_BIT(PAD_START)), "text input: M is not START");
	expect(!(ev.held & PAD_BIT(PAD_A)), "text input: Space is not A");
	expect(!(ev.held & PAD_BIT(PAD_Y)), "text input: Y is not a button");
	push_key(SDL_SCANCODE_RETURN);
	push_key(SDL_SCANCODE_BACKSPACE);
	plat_poll(&ev);
	expect(ev.held & PAD_BIT(PAD_A), "text input: Enter is still A");
	expect(ev.held & PAD_BIT(PAD_B), "text input: Backspace is still B");
	expect(ev.text[0] == '\0', "text input: no text when none was typed");
	if (text_ok) {
		push_text("a[b]c");
		plat_poll(&ev);
		expect(strcmp(ev.text, "abc") == 0, "text input: brackets stay buttons, not text");
	}

	plat_text_input(0);
	plat_poll(&ev);
	push_key(SDL_SCANCODE_M);
	if (text_ok)
		push_text("m");
	plat_poll(&ev);
	expect(ev.held & PAD_BIT(PAD_START), "text input off: M is START");
	expect(ev.text[0] == '\0', "text input off: no text is delivered");
	if (!fails)
		puts("clicks: text input on/off: ok");
}

int main(void)
{
	PlatEvents ev;
	int i;

	if (plat_init(0) != 0)
		return 1;
	plat_poll(&ev);			/* settle any startup events */
	push_click(200, 100);
	push_click(600, 400);
	push_click(1000, 800);
	plat_poll(&ev);
	expect(ev.nclicks == 3, "three clicks in one poll");
	if (ev.nclicks == 3) {
		expect(ev.click[0].x < ev.click[1].x && ev.click[1].x < ev.click[2].x, "x in arrival order");
		expect(ev.click[0].y < ev.click[1].y && ev.click[1].y < ev.click[2].y, "y in arrival order");
		printf("clicks: 3 in order: (%d,%d) (%d,%d) (%d,%d)\n", ev.click[0].x, ev.click[0].y,
		       ev.click[1].x, ev.click[1].y, ev.click[2].x, ev.click[2].y);
	}
	for (i = 0; i < PLAT_MAX_CLICKS + 3; i++)
		push_click(100 + i * 10, 100);
	plat_poll(&ev);
	expect(ev.nclicks == PLAT_MAX_CLICKS, "capped at PLAT_MAX_CLICKS");
	plat_poll(&ev);
	expect(ev.nclicks == 3, "the extras stay queued for the next poll");
	plat_poll(&ev);
	expect(ev.nclicks == 0, "then nothing");
	check_text_input();
	plat_shutdown();
	if (!fails)
		puts("clicks: ok");
	return fails ? 1 : 0;
}
