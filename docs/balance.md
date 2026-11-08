# Balance

Last tuned: 2026-11-06 (Plan 1, Task 14); rerun 2026-11-08 for the 18-city map (Plan 4, Task 1). Our numbers and our simulator's results only.

## How it was tuned

`build/host/ph-sim` plays 2,000 games of 30 days with two simple bots (see
`tools/sim/bots.c`) and reports scores, event rates and the prices it saw. The values
below were adjusted until the greedy bot's games felt like the genre should: a median
player gets ahead, a careless one dies now and then, the cops show up a few times a
game, and prices swing enough to trade on. Feel targets came from watching another game
of the genre through its screen; those observations stay outside this repo, and our
numbers were checked to differ from them rather than copy them.

Rerun:

```sh
make sim
build/host/ph-sim --games 2000 --seed 1 --days 30 --bot greedy
build/host/ph-sim --games 2000 --seed 1 --days 30 --bot cautious
```

## Tuned values (src/core/params.c, src/content/*.c)

- Start: $2,600 cash, $4,200 owed to Tornado Terry (Oklahoma City, 9.5% a day), 150 units of space.
- Bank: 4% a day. Free at the two branch neighborhoods in each city; anywhere else the
  out-of-network fee takes 3% of the amount (at least $5). Lenders: 5% (San Jose) to 12% (New York) a day.
- Goons: after 8 overdue days, 12 to 26 damage.
- Market: 20% chance a good is missing, at least 5 on sale; spikes 3.0% per good per stop
  (x2.5 to x4.5), crashes 0.6% (x0.20 to x0.45).
- Cops: 10% per move plus 1.2% per $10,000 of stash (cap 40%); posse grows by one each chase;
  run 55%; each cop hits 30% for 4 to 12.
- Muggings 5% per move (10 to 35% of pocket cash). Rumors 12% per move, 60% of them true.
- Goods (base range before city tendencies and events): Carts 30 to 220, Weed 300 to 1,150,
  Shrooms 450 to 1,500, Ketamine 600 to 2,000, Molly 1,000 to 3,600, Acid 1,800 to 5,000,
  DMT 2,600 to 7,500, Meth 3,500 to 11,000, Heroin 6,500 to 20,000, Cocaine 12,500 to 33,000.

## Simulator results (2,000 games, 30 days, seed 1)

| | Greedy bot | Cautious bot |
|---|---|---|
| Median score | 777841 | 361121 |
| 10th / 90th percentile | -38815 / 3478188 | 29 / 1303659 |
| Died | 6.0% | 0.2% |
| Cop chases per game | 5.57 | 4.35 |
| Spikes / crashes per game | 7.19 / 1.39 | 7.27 / 1.42 |
| Muggings / goon visits per game | 1.13 / 0.78 | 1.25 / 0.32 |

The cautious bot fills half its space, never fights, and repays its lender from any cash
above a $2,000 trading float. It checks that playing it safe rarely gets you killed.

## After 18 cities (Plan 4, Task 1; 2,000 games, 30 days, seed 1)

The map grew from 7 to 18 cities (11 new price tendencies and lenders), and flights across the
border (US to Mexico and back) draw their fare from $350 to $850 instead of $150 to $450.
The bots fly to a uniformly random other city 30% of the time, so most of their flights now
land in the new cities. Before and after, same runs (the "before" column is the code at the
start of this task, rerun today, since the table above was last measured before later
changes):

| | Greedy before | Greedy after | Cautious before | Cautious after |
|---|---|---|---|---|
| Median score | 768332 | 818459 | 356604 | 345552 |
| 10th / 90th percentile | -39006 / 3424984 | -35493 / 3699346 | 29 / 1276852 | 20 / 1410171 |
| Died | 6.0% | 5.2% | 0.2% | 0.2% |
| Cop chases per game | 5.57 | 5.65 | 4.35 | 4.41 |
| Spikes / crashes per game | 7.19 / 1.39 | 7.23 / 1.36 | 7.27 / 1.42 | 7.27 / 1.40 |
| Muggings / goon visits per game | 1.13 / 0.78 | 1.12 / 0.63 | 1.25 / 0.32 | 1.23 / 0.23 |

`ph-sim --strict --targets` still reports all 21 comparisons OK for both bots. Nothing was
retuned: the new cities' biases and the $350 to $850 fare range are exactly the plan's
values. For reference, 60 and 90 day greedy games (500 games) died 23.0% and 34.4% of the
time (not measured before this change), and none crashed.

## Known limits

- The feel targets are provisional: they come from only 20 observed days over two games,
  so the cop and mugging rates in particular are rough (3 and 1 events seen). Seth's
  playtest is the real check; adjust from his notes before trusting these numbers further.
- The out-of-network fee is there because a free bank let a player deposit everything
  before each move and dodge muggings and jail fines. At 3% each way, an overnight
  deposit and withdrawal at ATMs (about 6%) costs more than the 4% interest plus the
  mugging risk it avoids (about 1% a stop), so it only pays when you plan your stops
  around branches. The bots never bank, so the sim numbers above don't move with it.
- 60 and 90 day games are harder than intended for the greedy bot (more deaths as the
  posse keeps growing); only 30 days were tuned, and they all share these numbers.
- The bots are simple. A human who repays the loan early and watches for spikes will beat
  the greedy bot's median by a wide margin; that is intended.
