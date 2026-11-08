/* Save files and high scores: 16-byte header (magic, u16 version, u16 0, u32 payload
 * length, u32 crc32 of the payload), then the payload, all little endian. One field
 * list (io_game) serves both writing and reading, so the two cannot drift apart. */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/save.h"

#define SAVE_VERSION 4		/* 2: bank fee params and stat; 3: market events per day; 4: 18 cities, border fares */
#define HISCORE_VERSION 1
#define MAX_FILE (1L << 20)

typedef struct {
	unsigned char *buf;
	size_t len, cap, pos;
	int reading, bad;
} Io;

static void io_bytes(Io *io, void *p, size_t n)
{
	if (io->bad)
		return;
	if (io->reading) {
		if (io->pos + n > io->len) {
			io->bad = 1;
			return;
		}
		memcpy(p, io->buf + io->pos, n);
		io->pos += n;
	} else {
		if (io->len + n > io->cap) {
			io->bad = 1;
			return;
		}
		memcpy(io->buf + io->len, p, n);
		io->len += n;
	}
}

/* Little-endian unsigned of n bytes, through a uint64_t. */
static uint64_t io_le(Io *io, uint64_t v, int n)
{
	unsigned char b[8];
	int i;

	if (!io->reading)
		for (i = 0; i < n; i++)
			b[i] = (unsigned char)(v >> (8 * i));
	io_bytes(io, b, (size_t)n);
	if (!io->reading || io->bad)
		return v;
	v = 0;
	for (i = 0; i < n; i++)
		v |= (uint64_t)b[i] << (8 * i);
	return v;
}

static void io_u8(Io *io, uint8_t *v) { *v = (uint8_t)io_le(io, *v, 1); }
static void io_i8(Io *io, int8_t *v) { *v = (int8_t)(uint8_t)io_le(io, (uint8_t)*v, 1); }
static void io_i16(Io *io, int16_t *v) { *v = (int16_t)(uint16_t)io_le(io, (uint16_t)*v, 2); }
static void io_i32(Io *io, int32_t *v) { *v = (int32_t)(uint32_t)io_le(io, (uint32_t)*v, 4); }
static void io_i64(Io *io, int64_t *v) { *v = (int64_t)io_le(io, (uint64_t)*v, 8); }
static void io_u64(Io *io, uint64_t *v) { *v = io_le(io, *v, 8); }

static void io_enum(Io *io, int *v)
{
	uint8_t b = (uint8_t)*v;
	io_u8(io, &b);
	*v = b;
}

static void io_params(Io *io, Params *p)
{
	io_i64(io, &p->start_cash); io_i64(io, &p->start_debt);
	io_i32(io, &p->start_city); io_i32(io, &p->start_capacity);
	io_i32(io, &p->min_goods); io_i32(io, &p->absent_permille);
	io_i32(io, &p->spike_permille); io_i32(io, &p->crash_permille);
	io_i32(io, &p->spike_lo_pct); io_i32(io, &p->spike_hi_pct);
	io_i32(io, &p->crash_lo_pct); io_i32(io, &p->crash_hi_pct);
	io_i32(io, &p->bank_rate_bp); io_i32(io, &p->min_payment_pct);
	io_i32(io, &p->bank_fee_pct); io_i64(io, &p->bank_fee_min);
	io_i32(io, &p->enforce_days); io_i32(io, &p->enforce_dmg_lo); io_i32(io, &p->enforce_dmg_hi);
	io_i32(io, &p->cop_base_permille); io_i32(io, &p->cop_per_10k_permille);
	io_i32(io, &p->cop_max_permille);
	io_i32(io, &p->posse_start); io_i32(io, &p->posse_growth); io_i32(io, &p->max_shooters);
	io_i32(io, &p->run_permille); io_i32(io, &p->cop_hit_permille);
	io_i32(io, &p->cop_dmg_lo); io_i32(io, &p->cop_dmg_hi);
	io_i64(io, &p->loot_lo); io_i64(io, &p->loot_hi);
	io_i32(io, &p->drop_permille); io_i32(io, &p->drop_lo_pct); io_i32(io, &p->drop_hi_pct);
	io_i32(io, &p->jail_fine_pct);
	io_i32(io, &p->mug_permille); io_i32(io, &p->mug_lo_pct); io_i32(io, &p->mug_hi_pct);
	io_i32(io, &p->rumor_permille); io_i32(io, &p->rumor_true_permille);
	io_i32(io, &p->rumor_lead_lo); io_i32(io, &p->rumor_lead_hi);
	io_i32(io, &p->hospital_per_hp);
	io_i64(io, &p->flight_lo); io_i64(io, &p->flight_hi);
	io_i64(io, &p->intl_flight_lo); io_i64(io, &p->intl_flight_hi);
}

static void io_game(Io *io, Game *g)
{
	int i, d, phase = (int)g->phase, end = (int)g->end;
	Stats *s = &g->stats;

	io_params(io, &g->p);
	io_u64(io, &g->rng.state); io_u64(io, &g->rng.inc);
	io_u64(io, &g->flavor.state); io_u64(io, &g->flavor.inc);
	io_u64(io, &g->seed);
	io_bytes(io, g->name, sizeof g->name);
	io_i16(io, &g->day); io_i16(io, &g->last_day);
	io_u8(io, &g->loc.city); io_u8(io, &g->loc.hood);
	io_i64(io, &g->cash); io_i64(io, &g->bank);
	for (i = 0; i < NUM_CITIES; i++) io_i64(io, &g->debt[i]);
	for (i = 0; i < NUM_CITIES; i++) io_i16(io, &g->debt_age[i]);
	for (i = 0; i < NUM_GOODS; i++) io_i32(io, &g->stash[i]);
	for (i = 0; i < NUM_GOODS; i++) io_i64(io, &g->stash_paid[i]);
	io_i32(io, &g->capacity); io_i16(io, &g->health); io_i8(io, &g->weapon);
	for (i = 0; i < MAX_GEAR; i++) io_u8(io, &g->gear_owned[i]);
	for (i = 0; i < NUM_GOODS; i++) io_i32(io, &g->price[i]);
	for (i = 0; i < NUM_GOODS; i++)
		for (d = 0; d <= MAX_DAYS; d++)
			io_i32(io, &g->hist[i][d]);
	for (i = 0; i < NUM_GOODS; i++)
		for (d = 0; d <= MAX_DAYS; d++)
			io_i8(io, &g->hist_ev[i][d]);
	io_i16(io, &g->tip.day); io_u8(io, &g->tip.loc.city); io_u8(io, &g->tip.loc.hood);
	io_i8(io, &g->tip.good); io_i8(io, &g->tip.spike);
	io_i16(io, &g->chase.posse); io_i16(io, &g->chase.cops_left);
	io_enum(io, &phase); io_enum(io, &end);
	g->phase = (Phase)phase;
	g->end = (EndReason)end;
	io_i32(io, &s->moves); io_i32(io, &s->flights); io_i32(io, &s->cop_chases);
	io_i32(io, &s->cops_beaten); io_i32(io, &s->escapes); io_i32(io, &s->times_jailed);
	io_i32(io, &s->times_mugged); io_i32(io, &s->times_dropped); io_i32(io, &s->goon_visits);
	io_i32(io, &s->spikes_seen); io_i32(io, &s->crashes_seen); io_i32(io, &s->rumors_true);
	io_i32(io, &s->rumors_joke);
	io_i64(io, &s->best_net_worth); io_i64(io, &s->hospital_spent); io_i64(io, &s->bank_fees_paid);
}

/* A loaded game must make sense, or the rules could index out of bounds or overflow.
 * The CRC catches accidents; this catches values no game could have produced. */
static int game_sane(const Game *g)
{
	int64_t carried = 0;
	int i, d;

	if (!(days_index(g->last_day) >= 0 && g->day >= 1 && g->day <= g->last_day &&
	      g->loc.city < NUM_CITIES && g->loc.hood < HOODS_PER_CITY &&
	      g->weapon >= -1 && g->weapon < NUM_GEAR &&
	      g->phase <= PH_OVER && g->end <= END_QUIT &&
	      g->tip.loc.city < NUM_CITIES && g->tip.loc.hood < HOODS_PER_CITY &&
	      g->tip.good >= 0 && g->tip.good < NUM_GOODS && g->tip.day >= 0 && g->tip.day <= MAX_DAYS &&
	      g->name[sizeof g->name - 1] == 0 && params_sane(&g->p)))
		return 0;
	if (g->cash < 0 || g->cash > MONEY_MAX || g->bank < 0 || g->bank > MONEY_MAX ||
	    g->capacity < 0 || g->capacity > 1000000 || g->health < 0 || g->health > 100 ||
	    g->chase.posse < 0 || g->chase.cops_left < 0 ||
	    g->stats.bank_fees_paid < 0 || g->stats.bank_fees_paid > MONEY_MAX)
		return 0;
	for (i = 0; i < NUM_CITIES; i++)
		if (g->debt[i] < 0 || g->debt[i] > MONEY_MAX || g->debt_age[i] < 0)
			return 0;
	for (i = 0; i < NUM_GOODS; i++) {
		if (g->stash[i] < 0 || g->stash_paid[i] < 0 || g->stash_paid[i] > MONEY_MAX || g->price[i] < 0)
			return 0;
		carried += g->stash[i];
		for (d = 0; d <= MAX_DAYS; d++)
			if (g->hist[i][d] < 0 || g->hist_ev[i][d] < EV_NONE || g->hist_ev[i][d] > EV_CRASH)
				return 0;
	}
	return carried <= g->capacity;
}

static void io_hitable(Io *io, HiTable *t)
{
	int k, i;

	for (k = 0; k < 4; k++) {
		io_u8(io, &t->n[k]);
		for (i = 0; i < 10; i++) {
			int end = t->t[k][i].end;
			io_bytes(io, t->t[k][i].name, sizeof t->t[k][i].name);
			io_i64(io, &t->t[k][i].score);
			io_enum(io, &end);
			t->t[k][i].end = (uint8_t)end;
		}
	}
}

uint32_t crc32_buf(const void *p, size_t n)
{
	const unsigned char *b = p;
	uint32_t c = 0xffffffffu;
	size_t i;
	int k;

	for (i = 0; i < n; i++) {
		c ^= b[i];
		for (k = 0; k < 8; k++)
			c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1u)));
	}
	return c ^ 0xffffffffu;
}

int atomic_write_file(const char *path, const void *buf, size_t n)
{
	char tmp[1024], dirpath[1024];
	const char *slash;
	const unsigned char *b = buf;
	size_t done = 0;
	int fd;

	if (snprintf(tmp, sizeof tmp, "%s.tmp", path) >= (int)sizeof tmp)
		return -1;
	fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
		return -1;
	while (done < n) {
		ssize_t w = write(fd, b + done, n - done);
		if (w < 0) {
			if (errno == EINTR)
				continue;
			close(fd);
			unlink(tmp);
			return -1;
		}
		done += (size_t)w;
	}
	if (fsync(fd) != 0 || close(fd) != 0) {
		unlink(tmp);
		return -1;
	}
	if (rename(tmp, path) != 0) {
		unlink(tmp);
		return -1;
	}
	slash = strrchr(path, '/');
	if (slash) {
		size_t len = (size_t)(slash - path);
		if (len == 0)
			len = 1;	/* "/file" lives in "/" */
		if (len >= sizeof dirpath)
			return 0;
		memcpy(dirpath, path, len);
		dirpath[len] = 0;
	} else {
		strcpy(dirpath, ".");
	}
	fd = open(dirpath, O_RDONLY);
	if (fd >= 0) {
		fsync(fd);
		close(fd);
	}
	return 0;
}

static void put_header(unsigned char *h, const char *magic, int version, uint32_t len, uint32_t crc)
{
	int i;

	memcpy(h, magic, 4);
	h[4] = (unsigned char)(version & 0xff);
	h[5] = (unsigned char)(version >> 8);
	h[6] = h[7] = 0;
	for (i = 0; i < 4; i++) {
		h[8 + i] = (unsigned char)(len >> (8 * i));
		h[12 + i] = (unsigned char)(crc >> (8 * i));
	}
}

static uint32_t get_u32(const unsigned char *b)
{
	return (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
}

/* Writes magic + payload produced by fn atomically. */
static int write_file(const char *path, const char *magic, int version, void (*fn)(Io *, void *), void *obj)
{
	Io io;
	int rc;

	memset(&io, 0, sizeof io);
	io.cap = MAX_FILE;
	io.buf = malloc(io.cap);
	if (!io.buf)
		return -1;
	io.len = 16;
	fn(&io, obj);
	if (io.bad) {
		free(io.buf);
		return -1;
	}
	put_header(io.buf, magic, version, (uint32_t)(io.len - 16), crc32_buf(io.buf + 16, io.len - 16));
	rc = atomic_write_file(path, io.buf, io.len);
	free(io.buf);
	return rc;
}

/* Reads and checks the header, then decodes the payload with fn into obj.
 * Returns 0, -1 if the file can't be opened, -2 if anything is wrong with it. */
static int read_file(const char *path, const char *magic, int version, void (*fn)(Io *, void *), void *obj)
{
	unsigned char *buf;
	FILE *f = fopen(path, "rb");
	long n;
	Io io;

	if (!f)
		return -1;
	buf = malloc(MAX_FILE + 1);
	if (!buf) {
		fclose(f);
		return -2;
	}
	n = (long)fread(buf, 1, MAX_FILE + 1, f);
	fclose(f);
	if (n < 16 || n > MAX_FILE || memcmp(buf, magic, 4) != 0 ||
	    buf[4] != (version & 0xff) || buf[5] != (version >> 8) || buf[6] || buf[7] ||
	    get_u32(buf + 8) != (uint32_t)(n - 16) || get_u32(buf + 12) != crc32_buf(buf + 16, (size_t)(n - 16))) {
		free(buf);
		return -2;
	}
	memset(&io, 0, sizeof io);
	io.buf = buf + 16;
	io.len = (size_t)(n - 16);
	io.reading = 1;
	fn(&io, obj);
	free(buf);
	return io.bad || io.pos != io.len ? -2 : 0;
}

static void io_game_cb(Io *io, void *obj) { io_game(io, obj); }
static void io_hitable_cb(Io *io, void *obj) { io_hitable(io, obj); }

int save_write(const Game *g, const char *path)
{
	Game copy;

	memcpy(&copy, g, sizeof copy);
	return write_file(path, "PHSV", SAVE_VERSION, io_game_cb, &copy);
}

int save_read(Game *g, const char *path)
{
	static Game tmp;
	int rc;

	memset(&tmp, 0, sizeof tmp);
	rc = read_file(path, "PHSV", SAVE_VERSION, io_game_cb, &tmp);
	if (rc != 0)
		return rc;
	if (!game_sane(&tmp))
		return -2;
	tmp.nmsgs = 0;
	memcpy(g, &tmp, sizeof *g);
	return 0;
}

int hiscores_write(const HiTable *t, const char *path)
{
	HiTable copy;

	memcpy(&copy, t, sizeof copy);
	return write_file(path, "PHHS", HISCORE_VERSION, io_hitable_cb, &copy);
}

int hiscores_read(HiTable *t, const char *path)
{
	HiTable tmp;
	int rc, k, i;

	memset(&tmp, 0, sizeof tmp);
	rc = read_file(path, "PHHS", HISCORE_VERSION, io_hitable_cb, &tmp);
	for (k = 0; rc == 0 && k < 4; k++) {
		if (tmp.n[k] > 10)
			rc = -2;
		for (i = 0; i < 10; i++)
			if (tmp.t[k][i].name[sizeof tmp.t[k][i].name - 1] != 0)
				rc = -2;
	}
	if (rc != 0) {
		memset(t, 0, sizeof *t);
		return rc;
	}
	memcpy(t, &tmp, sizeof *t);
	return 0;
}

int days_index(int days)
{
	switch (days) {
	case 15: return 0;
	case 30: return 1;
	case 60: return 2;
	case 90: return 3;
	default: return -1;
	}
}

int hiscore_insert(HiTable *t, int days, const char *name, int64_t score, EndReason end)
{
	int k = days_index(days), rank, i;
	HiScore *row;

	if (k < 0)
		return -1;
	row = t->t[k];
	for (rank = 0; rank < t->n[k] && row[rank].score >= score; rank++)
		;
	if (rank >= 10)
		return -1;
	for (i = (t->n[k] < 10 ? t->n[k] : 9); i > rank; i--)
		row[i] = row[i - 1];
	memset(&row[rank], 0, sizeof row[rank]);
	if (name)
		strncpy(row[rank].name, name, sizeof row[rank].name - 1);
	row[rank].score = score;
	row[rank].end = (uint8_t)end;
	if (t->n[k] < 10)
		t->n[k]++;
	return rank;
}
