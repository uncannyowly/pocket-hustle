/* Loads a save file and checks it: smoke-check SAVE_FILE MIN_DAY LAST_DAY [RC_OFFER_PERMILLE].
 * Exit 0 when the save reads and is on at least MIN_DAY of a LAST_DAY game (and, when given,
 * the game was started with that chemical offer chance: the --param check). */
#include <stdio.h>
#include <stdlib.h>

#include "core/save.h"

int main(int argc, char **argv)
{
	static Game g;
	int r;

	if (argc != 4 && argc != 5) {
		fprintf(stderr, "usage: smoke-check SAVE_FILE MIN_DAY LAST_DAY [RC_OFFER_PERMILLE]\n");
		return 2;
	}
	r = save_read(&g, argv[1]);
	if (r != 0) {
		fprintf(stderr, "smoke: %s does not load (save_read %d)\n", argv[1], r);
		return 1;
	}
	printf("smoke: %s loads: %s, day %d of %d, phase %d, cash %lld\n", argv[1], g.name, (int)g.day,
	       (int)g.last_day, (int)g.phase, (long long)g.cash);
	if (g.last_day != atoi(argv[3]) || g.day < atoi(argv[2])) {
		fprintf(stderr, "smoke: expected day >= %s of %s\n", argv[2], argv[3]);
		return 1;
	}
	if (argc == 5 && g.p.rc_offer_permille != atoi(argv[4])) {
		fprintf(stderr, "smoke: rc_offer_permille is %d, expected %s\n", (int)g.p.rc_offer_permille, argv[4]);
		return 1;
	}
	return 0;
}
