/* fbview: shows the Miyoo build's stand-in framebuffer file (PH_FAKEFB=WxH:file) in a window,
 * about 20 times a second, at 1x. The file holds the picture rotated 180 degrees as the panel
 * wants it, XRGB8888; this undoes the rotation. It exits when the window is closed (or on
 * Esc), and when the watched process is gone (--pid N).
 *   fbview FILE [WxH] [--pid N] [--title TEXT]      (WxH defaults to 640x480) */
#include <SDL.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum { FRAME_MS = 50 };

int main(int argc, char **argv)
{
	const char *path = NULL, *title = "pocket-hustle (emulated Miyoo)";
	int w = 640, h = 480, i, fd = -1, running = 1;
	long pid = 0;
	uint32_t *raw, *pix;
	size_t n;
	SDL_Window *win;
	SDL_Renderer *ren;
	SDL_Texture *tex;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--pid") && i + 1 < argc)
			pid = atol(argv[++i]);
		else if (!strcmp(argv[i], "--title") && i + 1 < argc)
			title = argv[++i];
		else if (!path)
			path = argv[i];
		else if (sscanf(argv[i], "%dx%d", &w, &h) != 2 || w <= 0 || h <= 0) {
			fprintf(stderr, "usage: fbview FILE [WxH] [--pid N] [--title TEXT]\n");
			return 2;
		}
	}
	if (!path) {
		fprintf(stderr, "usage: fbview FILE [WxH] [--pid N] [--title TEXT]\n");
		return 2;
	}
	n = (size_t)w * (size_t)h;
	raw = malloc(n * 4);
	pix = malloc(n * 4);
	if (!raw || !pix || SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "fbview: %s\n", raw && pix ? SDL_GetError() : "out of memory");
		return 1;
	}
	win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, 0);
	ren = win ? SDL_CreateRenderer(win, -1, 0) : NULL;
	tex = ren ? SDL_CreateTexture(ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, w, h) : NULL;
	if (!tex) {
		fprintf(stderr, "fbview: %s\n", SDL_GetError());
		return 1;
	}
	memset(pix, 0, n * 4);
	while (running) {
		SDL_Event ev;

		while (SDL_PollEvent(&ev))
			if (ev.type == SDL_QUIT || (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE))
				running = 0;
		if (pid > 0 && kill((pid_t)pid, 0) != 0 && errno == ESRCH)
			running = 0;
		if (fd < 0)
			fd = open(path, O_RDONLY);	/* the game creates it a moment after it starts */
		if (fd >= 0 && pread(fd, raw, n * 4, 0) == (ssize_t)(n * 4))
			for (i = 0; i < (int)n; i++)	/* a half 180 degree turn: the last pixel first */
				pix[i] = raw[n - 1 - (size_t)i];
		SDL_UpdateTexture(tex, NULL, pix, w * 4);
		SDL_RenderClear(ren);
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
		SDL_Delay(FRAME_MS);
	}
	if (fd >= 0)
		close(fd);
	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	free(raw);
	free(pix);
	return 0;
}
