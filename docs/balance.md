# Balance

Last tuned: 2026-11-06 (Plan 1, Task 14); rerun 2026-11-08 for the 18-city map (Plan 4, Task 1) and 2026-11-09 for 20 cities (0.3.0). Our numbers and our simulator's results only.

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
| Median score | 768332 | 752202 | 356604 | 325714 |
| 10th / 90th percentile | -39006 / 3424984 | -36828 / 3607767 | 29 / 1276852 | 20 / 1415723 |
| Died | 6.0% | 5.4% | 0.2% | 0.2% |
| Cop chases per game | 5.57 | 5.63 | 4.35 | 4.42 |
| Spikes / crashes per game | 7.19 / 1.39 | 7.26 / 1.36 | 7.27 / 1.42 | 7.29 / 1.40 |
| Muggings / goon visits per game | 1.13 / 0.78 | 1.13 / 0.63 | 1.25 / 0.32 | 1.23 / 0.22 |

`ph-sim --strict --targets` still reports all 21 comparisons OK for both bots. Nothing was
retuned: the new cities' biases and the $350 to $850 fare range are exactly the plan's
values. For reference, 60 and 90 day greedy games (500 games) died 24.4% and 35.0% of the
time (not measured before this change), and none crashed.

## After the play-test task (Plan 4, Task 6; seed 1)

Rerun over every game length with the wild cards, finds and crew in the code (the balance bots take
no offers, and those draw from their own random stream): the 30 day numbers above are unchanged to
the digit (greedy 752202 median, 5.4% died; cautious 325714, 0.2%), `--strict --targets` reports
21 OK for both bots, and 500 game runs at 15, 60 and 90 days complete without a crash:

| Days | Greedy median | Greedy died | Cautious median | Cautious died |
|---|---|---|---|---|
| 15 | 13598 | 0.4% | 9726 | 0.0% |
| 60 | 6547621 | 24.4% | 2765189 | 2.0% |
| 90 | 18177076 | 35.0% | 9414307 | 3.6% |

## Meth as a regional bargain (2026-11-08, Seth's call; 0.2.1; seed 1)

Meth used to sell at 80% of its base price or less in 9 of the 18 cities. Seth chose to make it a
regional bargain: it is now cheap only in Texas and northern Mexico (Houston 80, Dallas 80, Tijuana
70, Sinaloa 60) and normal or dear elsewhere. Five cities lost cheap meth, and each got one other
bargain so every city still has two cheap and two dear goods: Cincinnati (meth 105, Ketamine 80),
Dayton (meth 110, Weed 80), Destin (meth 125, Carts 80), Cozumel (meth 115, Shrooms 80) and Los Cabos
(meth 120, DMT 80). (The Task 1 note above says the new biases are exactly the plan's values; the
Task 1 review had already nudged five of them for the two-cheap, two-dear rule.)

| 30 days, 2,000 games | Greedy before | Greedy after | Cautious before | Cautious after |
|---|---|---|---|---|
| Median score | 752202 | 733377 | 325714 | 316075 |
| 10th / 90th percentile | -36828 / 3607767 | -36559 / 3361654 | 20 / 1415723 | 21 / 1325078 |
| Died | 5.4% | 5.5% | 0.2% | 0.2% |
| Cop chases per game | 5.63 | 5.56 | 4.42 | 4.32 |
| Spikes / crashes per game | 7.26 / 1.36 | 7.25 / 1.36 | 7.29 / 1.40 | 7.27 / 1.39 |
| Muggings / goon visits per game | 1.13 / 0.63 | 1.15 / 0.63 | 1.23 / 0.22 | 1.24 / 0.22 |

`--strict --targets` reports all 21 comparisons OK for both bots. 500 game runs at other lengths
(greedy median and died, cautious median and died): 15 days 14767 / 0.4%, 10227 / 0.0%; 60 days
6547875 / 22.4%, 2499892 / 2.0%; 90 days 16768175 / 36.2%, 8498793 / 3.6%. None crashed.

## Chicago and Detroit (2026-11-09, Seth's request; 0.3.0; seed 1)

Seth asked for Chicago and Detroit, for an even twenty cities. They are appended after Sinaloa
(indices 18 and 19), so every earlier city keeps its index. Their tendencies follow 2026 local
prices where there is a story to tell:

- Chicago: weed is dear (125; Illinois taxes make its legal weed some of the priciest in the
  country), DMT 125; heroin 75 and cocaine 80 are cheap. Meth 110. Lender Deep Dish Dom, 9.25%
  a day, up to $35,000.
- Detroit: weed is cheap (60, as cheap as Oklahoma City and San Francisco; Michigan's legal weed
  is famously the cheapest in the country), carts 75; Molly 125 and cocaine 130 are dear. Meth 120. Lender Coney Connie,
  11.5% a day, up to $18,000.

Both keep the two-cheap, two-dear rule, and meth stays a regional bargain (cheap only in Houston,
Dallas, Tijuana and Sinaloa). Nothing else changed; the bots now draw their random flights from
19 other cities instead of 17.

| 30 days, 2,000 games | Greedy 0.2.1 | Greedy 0.3.0 | Cautious 0.2.1 | Cautious 0.3.0 |
|---|---|---|---|---|
| Median score | 733377 | 691712 | 316075 | 324871 |
| 10th / 90th percentile | -36559 / 3361654 | -39388 / 3452721 | 21 / 1325078 | -2452 / 1384945 |
| Died | 5.5% | 5.7% | 0.2% | 0.2% |
| Cop chases per game | 5.56 | 5.50 | 4.32 | 4.33 |
| Spikes / crashes per game | 7.25 / 1.36 | 7.19 / 1.35 | 7.27 / 1.39 | 7.28 / 1.40 |
| Muggings / goon visits per game | 1.15 / 0.63 | 1.14 / 0.62 | 1.24 / 0.22 | 1.25 / 0.20 |

`--strict --targets` reports all 21 comparisons OK for both bots. 500 game runs at other lengths
(greedy median and died, cautious median and died): 15 days 15963 / 1.2%, 10130 / 0.0%; 60 days
5739625 / 25.0%, 2431559 / 1.6%; 90 days 13524460 / 40.0%, 8670495 / 4.0%. None crashed. The
moves against 0.2.1 (for example 90 day greedy deaths 36.2% to 40.0%, 6 deaths in 500 at 15
days instead of 2) are about the size of the noise in 500 games (about 2 points at 36%); the
2,000 game 30 day runs barely moved.

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
