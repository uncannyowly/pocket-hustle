/* Game text: message templates and money formatting. Placeholders:
 *   {good} {gear} {rc} (a research chemical, chemical kinds only) {hood} {city} {transit} {lender} {cop} {rumor} {name}
 *   {amount} money   {qty} amount as a number   {n} a as a number
 *   {loc} "Hood, City" from b   {day} "day N" from amount   {trend} from c (tips)
 * Where a message's a names a city or hood, {city}/{hood}/{lender} use it;
 * otherwise they mean where the player is. */
#include <stdio.h>
#include <string.h>

#include "content/text.h"

#define L(t) {-1, t}
#define C(city, t) {city, t}
#define X(t) {-2, t}		/* a flight that crosses the border */
#define LINES(name, ...) static const TextLine name[] = { __VA_ARGS__ }

LINES(t_spike,
	L("Cops raided a {good} stash house in {city}. {good} is going for {amount}!"),
	L("A celebrity got caught with {good} on camera. Everybody wants some: {amount}!"),
	L("Supply chain problems hit {good}. Buyers are paying {amount}!"),
	L("{good} went viral overnight. Prices just hit {amount}."));
LINES(t_crash,
	L("A shipment of {good} fell off a truck. It's going for {amount}!"),
	L("Somebody flooded {hood} with {good}. Prices crashed to {amount}."),
	L("The {good} guy is retiring and clearing out his inventory: {amount}."),
	L("Overstock sale! {good} is down to {amount}."));
LINES(t_rode,
	L("You ride {transit} to {hood}."),
	L("You take {transit} over to {hood}."));
LINES(t_flew,
	L("You fly to {city}. The ticket cost {amount}."),
	L("Your flight to {city} cost {amount}, plus nine bucks for a water."),
	X("You cross the border to {city}. The ticket cost {amount}."),
	X("Passport out, {city} here you come. That flight cost {amount}."));
LINES(t_last_day,
	L("Last day, {name}. Sell everything."),
	L("Final day! Anything you're still holding tomorrow is worth nothing."));
LINES(t_game_over,
	L("Time's up, {name}. Let's count the money."));
LINES(t_goons,
	L("{lender}'s boys found you. They take it out on your ribs. (-{qty} health)"),
	L("{lender} sends a reminder about your loan. It hurts. (-{qty} health)"),
	L("Two large guys from {lender} explain compound interest with their fists. (-{qty} health)"));
LINES(t_dead,
	L("You're dead, {name}. The hustle is over."));
LINES(t_cops,
	L("It's {cop}! Backup is right behind: {n}."),
	L("{cop} spotted you and called for backup. Backup count: {n}."),
	L("Sirens! {cop} is chasing you with {n} backup."));
LINES(t_escaped,
	L("You ducked into a laundromat. {cop} ran right past."),
	L("You lost {cop} in a crowd of tourists."),
	L("You got away. {cop} stopped for donuts. Classic."));
LINES(t_dropped,
	L("You dropped {qty} {good} while running. Somebody's having a great day."),
	L("{qty} {good} fell out of your pocket. Gone."),
	L("In the chase you lost {qty} {good}."));
LINES(t_you_hit,
	L("Got one! Cops left: {n}."),
	L("Direct hit. {n} to go."));
LINES(t_you_missed,
	L("You missed. {cop} laughs."),
	L("Swing and a miss."),
	L("You hit a mailbox. The mailbox is fine."));
LINES(t_hit_you,
	L("You've been hit! (-{qty} health)"),
	L("Ouch. That one connected. (-{qty} health)"),
	L("A cop tags you. (-{qty} health)"));
LINES(t_cops_missed,
	L("They shoot and miss."),
	L("A shot goes wide."));
LINES(t_cops_beaten,
	L("You beat {cop}'s whole crew and found {amount} on them."));
LINES(t_jailed,
	L("{cop} hauls you in. You lose your stash and pay a {amount} fine."),
	L("A night in jail. Your stash is evidence now, and the fine was {amount}."),
	L("You give up. The judge takes {amount} and your whole stash."));
LINES(t_mugged,
	L("Somebody mugged you in {hood}! You lost {amount}."),
	L("A kid on a scooter snatched {amount} right out of your hand."),
	L("You got pickpocketed in {hood}. {amount} gone."));
LINES(t_rumor_tip,
	L("{rumor} swears {good} is about to {trend} in {loc} on {day}."),
	L("Overheard from {rumor}: {good} will {trend} in {loc} on {day}."));
LINES(t_rumor_joke,
	L("{rumor} says birds aren't real. He has a podcast about it."),
	L("{rumor} asks if you've heard of crypto. You walk away."),
	L("{rumor} is starting a new podcast. Episode one is about you."),
	L("{rumor} claims {cop} is three raccoons in a trench coat."),
	L("{rumor} reminds you to like and subscribe."),
	L("{rumor} says the secret to wealth is buying low and selling high. Thanks, {rumor}."),
	L("{rumor} says he invented the fidget spinner. He did not."),
	L("{rumor} heard the pigeons work for the government."),
	L("{rumor} is selling a course on how to sell courses."),
	L("{rumor} says the next big thing is NFTs again."),
	L("{rumor} swears he saw {cop} buying a tie-dye shirt."),
	L("{rumor} says sleep is a scam invented by mattress companies."),
	L("{rumor} wants you to invest in his energy drink startup."),
	L("{rumor} says the Wi-Fi is spying on you. He is wearing a foil hat."),
	L("{rumor} says an AI wrote this rumor."),
	C(0, "{rumor} says it's so windy in {city} that his podcast mic blew to Tulsa."),
	C(0, "{rumor} swears the tornado sirens test every day just for him."),
	C(0, "{rumor} says they call it Loud City because of his podcast."),
	C(1, "{rumor} says {city} has two seasons: hot and hotter."),
	C(1, "{rumor} got stuck on the 610 Loop for three days."),
	C(1, "{rumor} says space is fake but Mission Control has great snacks."),
	C(2, "{rumor} says his startup is Uber for podcasts. It's just a podcast."),
	C(2, "{rumor} says rent in {city} went up while he was talking."),
	C(2, "{rumor} says every barista in {city} is a former founder."),
	C(3, "{rumor} says the rats on the Subway have a union now."),
	C(3, "{rumor} paid nineteen dollars for a bagel and says it was worth it."),
	C(3, "{rumor} found a rent-stabilized apartment. It's a closet."),
	C(4, "{rumor} says the humidity in {city} is legally a liquid."),
	C(4, "{rumor} saw a flamingo driving a sports car on Brickell."),
	C(4, "{rumor} says {city} runs on cafecito and bad decisions."),
	C(5, "{rumor} says the best gumbo in town is his family's secret. His family doesn't know it."),
	C(5, "{rumor} caught so many beads he can't lift his arms."),
	C(5, "{rumor} says a ghost in the French Quarter owes him five bucks."),
	C(6, "{rumor} says it's been raining in {city} since 1987."),
	C(6, "{rumor} says everyone in {city} has a band and a bike."),
	C(6, "{rumor} is fermenting his own kombucha. It's sentient now."),
	C(7, "{rumor} says everything is bigger in {city}. He means his ego."),
	C(7, "{rumor} waited two hours for brisket in Deep Ellum and calls it a personality."),
	C(8, "{rumor} called it Frisco. Six people corrected him at once."),
	C(8, "{rumor} says the fog in {city} has its own podcast now. It's better than his."),
	C(9, "{rumor} is pitching his podcast as a streaming series. Nobody called back."),
	C(9, "{rumor} says the 405 is not a freeway, it's a lifestyle."),
	C(10, "{rumor} says it only rained once in {city}. It started in October."),
	C(10, "{rumor} is on his fourth espresso and his second podcast."),
	C(11, "{rumor} put chili on spaghetti and called it a three-way. He's not wrong."),
	C(11, "{rumor} says the Nati has more hills than Rome. He has never been to Rome."),
	C(12, "{rumor} says {city} invented flight, so he invented the podcast."),
	C(12, "{rumor} calls {city} the Land of Funk and will not stop doing the dance."),
	C(13, "{rumor} says he caught a 46-pound snapper. It was a flip-flop."),
	C(13, "{rumor} anchored at Crab Island in June. Nobody has seen him leave."),
	C(14, "{rumor} took a picture with a zonkey on La Revu. It's his new profile pic."),
	C(14, "{rumor} says the Caesar salad was invented in {city}, and so was his podcast."),
	C(15, "{rumor} rented a moped on {city} three days ago. Still looking for the rental shop."),
	C(15, "{rumor} says the reef off Punta Sur is quieter than his podcast. Much quieter."),
	C(16, "{rumor} sat through a timeshare pitch for free tacos. He bought two timeshares."),
	C(16, "{rumor} says swim at Lover's Beach, never at Divorce Beach. He learned the hard way."),
	C(17, "{rumor} rode a pulmonia the length of the Malecon twice. The driver wants a cut."),
	C(17, "{rumor} says the aguachile in La Perla made him cry, in a good way."));
LINES(t_healed,
	L("The doctor patches you up. (+{qty} health)"));
LINES(t_bought_gear,
	L("You bought the {gear}."),
	L("New gear: {gear}. Use it wisely."));
LINES(t_rc_offer,
	L("A chemist in {hood} has a new research chemical: {rc}. Back it before the scene decides?"),
	L("Word in {hood} is that {rc} might be the next big thing. Want a piece?"));
LINES(t_rc_find,
	L("You found a forgotten batch of {rc}. If it catches on, it could be worth {amount}."));
LINES(t_rc_hit,
	L("{rc} caught on in the scene. Your stake pays {amount}!"),
	L("Everybody wants {rc} now. You cash out {amount}."));
LINES(t_rc_bust,
	L("{rc} flopped. Nobody wants it, and your {amount} is gone."),
	L("The scene passed on {rc}. You lose {amount}."));
LINES(t_rc_bust_found,
	L("Your batch of {rc} never caught on. Easy come, easy go."));
/* The crew path is serious: no jokes, and Podcast Pete never goes near it. */
LINES(t_crew_offer,
	L("A shady crew in {hood} wants to cut your stash with fentanyl. They promise a huge payout."),
	L("Some people you would rather not know offer to stretch your stash with fentanyl. Big money, they say."));
LINES(t_crew_paid,
	L("The cut batch moved fast. The crew pays you {amount}. You try not to think about it."));
LINES(t_overdose,
	L("The cut batch hit the streets and people overdosed. {city} wants you gone, for good. Your stash is gone."));
LINES(t_debt_sold,
	L("Your old lender sold your debt to {lender}. You owe {amount} here now."));

typedef struct { const TextLine *lines; int n; const char *allowed; } KindText;

#define K(arr, allowed) {arr, (int)(sizeof arr / sizeof arr[0]), allowed}

static const KindText KINDS[NUM_MSG_KINDS] = {
	[M_SPIKE]       = K(t_spike, "good amount hood city name"),
	[M_CRASH]       = K(t_crash, "good amount hood city name"),
	[M_RODE]        = K(t_rode, "hood city transit name"),
	[M_FLEW]        = K(t_flew, "city amount name"),
	[M_LAST_DAY]    = K(t_last_day, "name city hood"),
	[M_GAME_OVER]   = K(t_game_over, "name"),
	[M_GOONS]       = K(t_goons, "lender city qty name"),
	[M_DEAD]        = K(t_dead, "name"),
	[M_COPS]        = K(t_cops, "cop n name hood city"),
	[M_ESCAPED]     = K(t_escaped, "cop name"),
	[M_DROPPED]     = K(t_dropped, "good qty cop"),
	[M_YOU_HIT]     = K(t_you_hit, "n cop"),
	[M_YOU_MISSED]  = K(t_you_missed, "cop"),
	[M_HIT_YOU]     = K(t_hit_you, "qty cop"),
	[M_COPS_MISSED] = K(t_cops_missed, "cop"),
	[M_COPS_BEATEN] = K(t_cops_beaten, "amount cop"),
	[M_JAILED]      = K(t_jailed, "amount cop name city"),
	[M_MUGGED]      = K(t_mugged, "amount hood city"),
	[M_RUMOR_TIP]   = K(t_rumor_tip, "rumor good loc day trend"),
	[M_RUMOR_JOKE]  = K(t_rumor_joke, "rumor city name cop"),
	[M_HEALED]      = K(t_healed, "qty"),
	[M_BOUGHT_GEAR] = K(t_bought_gear, "gear"),
	[M_RC_OFFER]    = K(t_rc_offer, "rc hood city name"),
	[M_RC_FIND]     = K(t_rc_find, "rc amount hood city name"),
	[M_RC_HIT]      = K(t_rc_hit, "rc amount name"),
	[M_RC_BUST]     = K(t_rc_bust, "rc amount name"),
	[M_RC_BUST_FOUND] = K(t_rc_bust_found, "rc name"),
	[M_CREW_OFFER]  = K(t_crew_offer, "hood city name"),
	[M_CREW_PAID]   = K(t_crew_paid, "amount name"),
	[M_OVERDOSE]    = K(t_overdose, "city name"),
	[M_DEBT_SOLD]   = K(t_debt_sold, "lender amount city name"),
};

int text_lines(MsgKind k, const TextLine **lines)
{
	if ((int)k < 0 || k >= NUM_MSG_KINDS) {
		*lines = NULL;
		return 0;
	}
	*lines = KINDS[k].lines;
	return KINDS[k].n;
}

int text_allowed(MsgKind k, const char *ph)
{
	const char *s;
	size_t n = strlen(ph);

	if ((int)k < 0 || k >= NUM_MSG_KINDS || n == 0)
		return 0;
	for (s = KINDS[k].allowed; *s; ) {
		const char *e = strchr(s, ' ');
		size_t len = e ? (size_t)(e - s) : strlen(s);
		if (len == n && strncmp(s, ph, n) == 0)
			return 1;
		s += len;
		while (*s == ' ')
			s++;
	}
	return 0;
}

static int eligible(const TextLine *l, MsgKind k, const Msg *m)
{
	if (l->city == -2)	/* border crossings only */
		return k == M_FLEW && m->c;
	if (l->city == -1)
		return !(k == M_FLEW && m->c);
	return k == M_RUMOR_JOKE && l->city == m->a;
}

const TextLine *text_pick(const Game *g, const Msg *m)
{
	const TextLine *lines;
	int n = text_lines((MsgKind)m->kind, &lines), i, count = 0, want;

	(void)g;
	for (i = 0; i < n; i++)
		if (eligible(&lines[i], (MsgKind)m->kind, m))
			count++;
	if (count == 0)
		return NULL;
	want = m->variant % count;
	for (i = 0; i < n; i++)
		if (eligible(&lines[i], (MsgKind)m->kind, m) && want-- == 0)
			return &lines[i];
	return NULL;
}

void fmt_money(int64_t v, char *buf, size_t n)
{
	char digits[32], out[48];
	uint64_t u = v < 0 ? (uint64_t)0 - (uint64_t)v : (uint64_t)v;
	int nd = 0, i, o = 0;

	do {
		digits[nd++] = (char)('0' + u % 10);
		u /= 10;
	} while (u);
	if (v < 0)
		out[o++] = '-';
	out[o++] = '$';
	for (i = nd - 1; i >= 0; i--) {
		out[o++] = digits[i];
		if (i > 0 && i % 3 == 0)
			out[o++] = ',';
	}
	out[o] = 0;
	snprintf(buf, n, "%s", out);
}

static const char *city_name(int c)
{
	return c >= 0 && c < NUM_CITIES ? CITIES[c].name : "?";
}

static const char *hood_name(int c, int h)
{
	return c >= 0 && c < NUM_CITIES && h >= 0 && h < HOODS_PER_CITY ? CITIES[c].hoods[h] : "?";
}

static int a_is_city(MsgKind k)
{
	return k == M_FLEW || k == M_GOONS || k == M_RUMOR_JOKE || k == M_OVERDOSE || k == M_DEBT_SOLD;
}

/* Writes the value of placeholder ph into v. Returns 0 for an unknown placeholder. */
static int fill(const Game *g, const Msg *m, const char *ph, char *v, size_t vn)
{
	MsgKind k = (MsgKind)m->kind;
	int city = a_is_city(k) ? m->a : g->loc.city;

	if (!strcmp(ph, "good"))
		snprintf(v, vn, "%s", m->a >= 0 && m->a < NUM_GOODS ? GOODS[m->a].name : "?");
	else if (!strcmp(ph, "rc"))
		snprintf(v, vn, "%s", m->a >= 0 && m->a < NUM_CHEMS ? CHEMS[m->a].name : "?");
	else if (!strcmp(ph, "gear"))
		snprintf(v, vn, "%s", m->a >= 0 && m->a < NUM_GEAR ? GEAR[m->a].name : "?");
	else if (!strcmp(ph, "hood"))
		snprintf(v, vn, "%s", hood_name(g->loc.city, k == M_RODE ? m->a : g->loc.hood));
	else if (!strcmp(ph, "city"))
		snprintf(v, vn, "%s", city_name(city));
	else if (!strcmp(ph, "transit"))
		snprintf(v, vn, "%s", CITIES[g->loc.city].transit);
	else if (!strcmp(ph, "lender"))
		snprintf(v, vn, "%s", city >= 0 && city < NUM_CITIES ? CITIES[city].lender : "?");
	else if (!strcmp(ph, "cop"))
		snprintf(v, vn, "%s", COP_NAME);
	else if (!strcmp(ph, "rumor"))
		snprintf(v, vn, "%s", RUMOR_NAME);
	else if (!strcmp(ph, "name"))
		snprintf(v, vn, "%s", g->name[0] ? g->name : "friend");
	else if (!strcmp(ph, "amount"))
		fmt_money(m->amount, v, vn);
	else if (!strcmp(ph, "qty"))
		snprintf(v, vn, "%lld", (long long)m->amount);
	else if (!strcmp(ph, "n"))
		snprintf(v, vn, "%d", (int)m->a);
	else if (!strcmp(ph, "loc"))
		snprintf(v, vn, "%s, %s", hood_name(m->b / HOODS_PER_CITY, m->b % HOODS_PER_CITY),
			 city_name(m->b / HOODS_PER_CITY));
	else if (!strcmp(ph, "day"))
		snprintf(v, vn, "day %lld", (long long)m->amount);
	else if (!strcmp(ph, "trend"))
		snprintf(v, vn, "%s", m->c ? "skyrocket" : "crash");
	else
		return 0;
	return 1;
}

size_t text_msg(const Game *g, const Msg *m, char *buf, size_t n)
{
	const TextLine *l = text_pick(g, m);
	const char *s;
	size_t o = 0;

	if (n == 0)
		return 0;
	buf[0] = 0;
	if (!l)
		return 0;
	for (s = l->t; *s && o + 1 < n; ) {
		const char *e = *s == '{' ? strchr(s, '}') : NULL;
		char ph[32], val[96];
		size_t len;

		if (e && (size_t)(e - s - 1) < sizeof ph) {
			memcpy(ph, s + 1, (size_t)(e - s - 1));
			ph[e - s - 1] = 0;
			if (fill(g, m, ph, val, sizeof val)) {
				len = strlen(val);
				if (len > n - 1 - o)
					len = n - 1 - o;
				memcpy(buf + o, val, len);
				o += len;
				s = e + 1;
				continue;
			}
		}
		buf[o++] = *s++;
	}
	buf[o] = 0;
	return o;
}
