/* Checks that plat_poll hands back every click, in order, and keeps the extras queued:
 * pushes SDL mouse events (run with the dummy video driver) and polls. */
#include <SDL.h>
#include <stdio.h>

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
	plat_shutdown();
	if (!fails)
		puts("clicks: ok");
	return fails ? 1 : 0;
}
