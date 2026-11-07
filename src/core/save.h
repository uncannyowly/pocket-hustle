/* Save files and high scores. Both are written atomically (temp file, fsync, rename,
 * fsync the directory) so a power-off never leaves half a file behind. */
#ifndef PH_SAVE_H
#define PH_SAVE_H

#include <stddef.h>

#include "core/game.h"

typedef struct { char name[16]; int64_t score; uint8_t end; } HiScore;
typedef struct { HiScore t[4][10]; uint8_t n[4]; } HiTable;	/* one table per game length */

int      days_index(int days);			/* 15,30,60,90 -> 0..3, else -1 */
int      hiscore_insert(HiTable *t, int days, const char *name, int64_t score, EndReason end);
						/* rank 0..9, or -1 if it didn't make the table */
uint32_t crc32_buf(const void *p, size_t n);	/* IEEE, reflected, poly 0xEDB88320 */
int      atomic_write_file(const char *path, const void *buf, size_t n);	/* 0 or -1 */
int      save_write(const Game *g, const char *path);	/* 0 or -1 */
int      save_read(Game *g, const char *path);		/* 0; -1 missing; -2 corrupt or other version */
int      hiscores_write(const HiTable *t, const char *path);
int      hiscores_read(HiTable *t, const char *path);	/* on error *t is zeroed; -1 or -2 */

#endif
