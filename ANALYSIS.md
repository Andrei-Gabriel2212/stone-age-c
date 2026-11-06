# Milestone 5 - Analysis Sheet (15-round average game)

Each round, a player commits 5-10 workers across the board. Workers placed
cannot be re-used in the same round. The action space is therefore the central
decision in Stone Age, and every action must be compared not just by its
immediate yield but also by its long-term contribution to final score.

## Assumptions

* Game length: ~15 rounds (matches the project specification baseline).
* Each player begins with 5 workers, 12 food, no resources, no tools.
* Dice expected value per worker = 3.5 pips.
* Resource expected yield: workers * 3.5 / divisor, with tools adding +k.

## Expected Yields per Action (no tools, single round)

- HUNTING  (loc 0, /2): 2 workers -> 7/2 = 3 food on average.
- FOREST   (loc 4, /3): 2 workers -> 7/3 = 2 wood on average.
- CLAY PIT (loc 5, /4): 2 workers -> 7/4 = 1 brick on average.
- QUARRY   (loc 6, /5): 2 workers -> 7/5 = 1 stone on average.
- RIVER    (loc 7, /6): 2 workers -> 7/6 = 1 gold on average.

The higher the divisor, the more variance. Gold trips are big-or-bust.

## Value of Special Locations (long-term)

**TOOL MAKER (loc 1):**
One worker per round. Each tool is permanent and reusable per round.
Tool value 1 saves 1 dice pip per round once owned -> over 15 rounds
that is +15 pips of total guaranteed addition, roughly +5 food / +3 wood.
Investing tools EARLY (rounds 1-5) yields 10-12x return.
Investing LATE (rounds 12+) yields almost nothing.

**FIELD (loc 2):**
One worker -> +1 food production permanently. Each level saves 1 worker
from being assigned to hunting every following round. Strongly compounds
with HUT (more people to feed). Optimal around rounds 2-5.

**HUT (loc 3):**
Two workers -> +1 person to total pool. Cost: 2 workers + 1 worker on
Field eventually to feed them. Net benefit: +1 worker for 10+ rounds =
+35 dice pips. Excellent if taken EARLY (round 1-4), marginal after
round 10.

## Value of Buildings (locations 12-15)

Buildings give IMMEDIATE victory points and END THE GAME when a stack runs
out. Their value scales linearly with VP, but their strategic value depends
on the round taken:

- EARLY (round 1-4): low; you lack resources.
- MID   (round 5-10): peak; you can afford 1-2 per round.
- LATE  (round 11+): be careful - finishing a stack triggers game end!

Average building yields 3-7 VP for 2-4 resources, i.e. 1.0-1.8 VP/resource.

## Value of Civilization Cards (locations 8-11)

Cost = slot index (slot 0 = free, slot 3 = 3 resources).
Bonuses are immediate (resources, food, tool), score is at game-end.

- **SCORE_FLAT** cards (Pottery, Weaving, Scratching, Transport, Gemstone, Time,
  Tool Making, Bookkeeping): Score n*n where n = unique types collected. A
  player who collects 5 different SCORE_FLAT cards scores 25 VP just from
  this multiplier. VERY HIGH long-term value - rewards diverse collecting.
- **SCORE_PER_TOOL** (Claywork, Cartography): score 1 VP per tool point owned.
  Good if you also invest in Tool Maker. With ~6 tool pips at endgame,
  one such card = +6 VP. Two cards stack: +12 VP.
- **SCORE_PER_HUT** (Shaman, Tribal Art): score 1 VP per total people. With
  ~8 people, one card = +8 VP. Synergy with Hut location.
- **SCORE_PER_CARD** (Healing, Music): score 1 VP per total civ card owned.
  Self-amplifying: the more cards you buy, the more these score.
- **SCORE_PER_FARM** (Writing, Harvest): score 1 VP per food production level.
  Good with Field location.
- **SCORE_MULTIPLY** (Irrigation, Farmer): scored 3*food_production or
  4*food_production at end of game.

## Expected Score Breakdown (target endgame, 15 rounds, balanced play)

| Component                                    | Value           |
|-----------------------------------------------|----------------|
| Buildings purchased: ~5 buildings              | ~25 VP         |
| SCORE_FLAT unique squared: 4 unique            | 16 VP          |
| SCORE_PER_TOOL * tools: 2 * 5                  | 10 VP          |
| SCORE_PER_HUT * people: 1 * 7                  | 7 VP           |
| SCORE_PER_FARM * food_prod: 1 * 5              | 5 VP           |
| SCORE_MULTIPLY: 1 * 4 * 5                      | 20 VP          |
| Leftover resources                             | ~4 VP          |
| Feeding penalties                              | -10/-20 VP     |
| **TOTAL ESTIMATE**                             | **~75-90 VP**  |

# Heuristic Agent Proposals (used as references for Milestone 6)

## Agent A - "Greedy Resource Grabber" (immediate-gain greedy)

Priority queue per round:

1. If owns < 2 tools, place 1 worker on Tool Maker.
2. Place all remaining workers on the highest-yield resource location
   currently available (prefers Forest, then Quarry).
3. Fall back to Hunting if all resource locations are blocked.

Buys buildings whenever affordable; never civ cards.

- Strength: piles VPs fast through buildings.
- Weakness: ignores cards entirely -> caps around 50-60 VP.

## Agent B - "Greedy Card Collector" (immediate-gain greedy, alt path)

Priority queue per round:

1. ALWAYS try to grab the cheapest free civ card (slot 0).
2. Place workers on Quarry/River for high-cost cards next round.
3. Hunts only if food < people_total.

Buys buildings only with leftover gold/stone.

- Strength: very high SCORE_FLAT multiplier (n*n) at endgame.
- Weakness: starvation risk in mid-game.

## Agent C - "Balanced Long-Term Planner" (long-term, mixed)

Three-phase strategy:

- ROUNDS 1-4 (FOUNDATION): prioritize Hut and Field, take 1 tool.
- ROUNDS 5-10 (EXPANSION): alternate cards and buildings; keep food
  production >= people_total to avoid penalty.
- ROUNDS 11-15 (HARVEST): buy any affordable card with PER_TOOL,
  PER_HUT, PER_FARM bonus; finish a stack only if winning.

- Strength: scales late-game multipliers (~85-100 VP expected).
- Weakness: slow start; vulnerable if opponent rushes buildings.

## Shared Utility Functions (for Milestone 6)

All three agents share these utility functions:

- `expected_resource_yield(workers, divisor, tools)`
- `card_long_term_value(CivCard, current_state)`
- `building_purchase_value(Building, current_round)`
- `feeding_safety_check(Player, after_action)`
