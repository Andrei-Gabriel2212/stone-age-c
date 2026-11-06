#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bot_coleg.h"
#include "game_logic.h"

static int total_resources(const Player *p) {
    return p->player_resources[0] + p->player_resources[1]
         + p->player_resources[2] + p->player_resources[3];
}

static int sum_tools(const Player *p) {
    return p->permanent_tools[0] + p->permanent_tools[1] + p->permanent_tools[2];
}

static int food_deficit_next(const Player *p) {
    int deficit = p->people_total - (p->food_amount + p->food_production);
    return deficit;
}

static int civ_card_value(Player *p, CivCard *c) {
    int v = 0;

    if (c->tool_bonus > 0) v += 7;

    int total_immediate_food = c->bonus_food;
    int total_immediate_res = c->bonus_wood + c->bonus_brick + c->bonus_stone + c->bonus_gold;
    if (total_immediate_food >= 4) v += 5;
    else if (total_immediate_food >= 2) v += 3;
    else if (total_immediate_food >= 1) v += 1;

    if (total_immediate_res > 0) {
        int weighted = c->bonus_wood * 3 + c->bonus_brick * 4
                     + c->bonus_stone * 5 + c->bonus_gold * 6;
        v += 1 + weighted / 4;
    }

    switch (c->score_type) {
        case SCORE_FLAT:
            v += 2;
            for (int i = 0; i < p->cards_count; i++) {
                if (p->inventory_cards[i].score_type == SCORE_FLAT
                    && p->inventory_cards[i].id % CIV_UNIQUE_TYPES != c->id % CIV_UNIQUE_TYPES) {
                    v += 1;
                }
            }
            break;
        case SCORE_PER_TOOL:
            v += 2 + sum_tools(p) / 2;
            break;
        case SCORE_PER_HUT:
            v += 2 + p->people_total / 2;
            break;
        case SCORE_PER_CARD:
            v += 2 + p->cards_count / 2;
            break;
        case SCORE_PER_FARM:
            v += 2 + p->food_production / 2;
            break;
        case SCORE_MULTIPLY:
            v += 3 + p->food_production;
            break;
    }
    return v;
}

static int best_civ_slot(Board *b, Player *p, int *out_val, int require_affordable) {
    int best = -1;
    int best_val = 0;
    for (int i = 0; i < 4; i++) {
        if (b->active_cards[i].id < 0) continue;
        if (b->location_occupants[LOC_CARD_BASE + i] > 0) continue;
        int cost = i;
        if (require_affordable && total_resources(p) < cost) continue;
        int val = civ_card_value(p, &b->active_cards[i]);
        if (val > best_val) { best_val = val; best = i; }
    }
    if (out_val) *out_val = best_val;
    return best;
}

static int can_afford_building(Player *p, BuildingCard *bd) {
    return (p->player_resources[RES_WOOD]  >= bd->cost_wood) &&
           (p->player_resources[RES_BRICK] >= bd->cost_brick) &&
           (p->player_resources[RES_STONE] >= bd->cost_stone) &&
           (p->player_resources[RES_GOLD]  >= bd->cost_gold);
}

static int best_affordable_building(Board *b, Player *p, int *out_points) {
    int best = -1;
    int best_points = -1;
    for (int s = 0; s < NUM_BUILDING_STACKS; s++) {
        if (b->stack_counters[s] >= BUILDINGS_PER_STACK) continue;
        if (b->location_occupants[LOC_BUILD_BASE + s] > 0) continue;
        BuildingCard bd = b->building_stacks[s][b->stack_counters[s]];
        if (can_afford_building(p, &bd) && bd.victory_points > best_points) {
            best_points = bd.victory_points;
            best = s;
        }
    }
    if (out_points) *out_points = best_points;
    return best;
}

static int min_building_remaining(Board *b) {
    int best = BUILDINGS_PER_STACK;
    for (int i = 0; i < NUM_BUILDING_STACKS; i++) {
        int remaining = BUILDINGS_PER_STACK - b->stack_counters[i];
        if (remaining < best) best = remaining;
    }
    return best;
}

static int game_phase(BotColegState *bs, Board *b) {
    int cards_left = CIV_TOTAL_CARDS - b->deck_index;
    int build_left = min_building_remaining(b);

    if (build_left <= 2 || cards_left <= 8 || bs->current_round >= 9) return 2;
    if (build_left <= 4 || cards_left <= 16 || bs->current_round >= 5) return 1;
    return 0;
}

static int bot_is_leading(Player *bot_player, Player *opponent) {
    return bot_player->current_score >= opponent->current_score;
}

static int try_place(Board *b, Player *bot_player, int loc, int count,
                     int *out_loc, int *out_count) {
    if (count <= 0) return 0;
    if (is_placement_valid(b, bot_player, loc, count)) {
        *out_loc = loc;
        *out_count = count;
        return 1;
    }
    return 0;
}

static int place_resource_loc(Board *b, Player *bot_player, int loc, int cap,
                               int *out_loc, int *out_count) {
    int w = bot_player->people_available;
    int room;
    if (loc == LOC_HUNTING) {
        room = w;
    } else {
        room = RESOURCE_SPACE_LIMIT - b->location_occupants[loc];
    }
    if (w > room) w = room;
    if (cap > 0 && w > cap) w = cap;
    return try_place(b, bot_player, loc, w, out_loc, out_count);
}

static int strategy_resource_rush(BotColegState *bs, Board *b, Player *p,
                                   int *out_loc, int *out_count) {
    (void)bs;
    if (food_deficit_next(p) > 0) {
        if (place_resource_loc(b, p, LOC_HUNTING, 0, out_loc, out_count)) return 1;
    }
    if (p->people_total < 10 && p->people_available >= 2 && food_deficit_next(p) <= 0) {
        if (try_place(b, p, LOC_HUT, 2, out_loc, out_count)) return 1;
    }
    int stack = best_affordable_building(b, p, NULL);
    if (stack >= 0) {
        if (try_place(b, p, LOC_BUILD_BASE + stack, 1, out_loc, out_count)) return 1;
    }
    if (b->active_cards[0].id >= 0 && b->location_occupants[LOC_CARD_BASE + 0] == 0) {
        if (try_place(b, p, LOC_CARD_BASE + 0, 1, out_loc, out_count)) return 1;
    }
    if (place_resource_loc(b, p, LOC_GOLD,  0, out_loc, out_count)) return 1;
    if (place_resource_loc(b, p, LOC_STONE, 0, out_loc, out_count)) return 1;
    if (place_resource_loc(b, p, LOC_BRICK, 0, out_loc, out_count)) return 1;
    if (place_resource_loc(b, p, LOC_WOOD,  0, out_loc, out_count)) return 1;
    if (place_resource_loc(b, p, LOC_HUNTING, 0, out_loc, out_count)) return 1;
    return 0;
}

static int strategy_card_sniper(BotColegState *bs, Board *b, Player *p,
                                  int *out_loc, int *out_count) {
    (void)bs;
    int val;
    int slot = best_civ_slot(b, p, &val, 1);
    if (slot >= 0) {
        if (try_place(b, p, LOC_CARD_BASE + slot, 1, out_loc, out_count)) return 1;
    }
    if (food_deficit_next(p) > 0) {
        if (try_place(b, p, LOC_FIELD, 1, out_loc, out_count)) return 1;
    }
    if (place_resource_loc(b, p, LOC_WOOD,  2, out_loc, out_count)) return 1;
    if (place_resource_loc(b, p, LOC_BRICK, 2, out_loc, out_count)) return 1;
    if (place_resource_loc(b, p, LOC_HUNTING, 0, out_loc, out_count)) return 1;
    return 0;
}

static int strategy_engine_builder(BotColegState *bs, Board *b, Player *p, Player *opponent,
                                     int *out_loc, int *out_count) {
    int phase = game_phase(bs, b);

    if (food_deficit_next(p) > 0) {
        if (try_place(b, p, LOC_FIELD, 1, out_loc, out_count)) return 1;
    }

    if (phase == 0) {
        if (p->food_production < p->people_total) {
            if (try_place(b, p, LOC_FIELD, 1, out_loc, out_count)) return 1;
        }
        if (sum_tools(p) < 6) {
            if (try_place(b, p, LOC_TOOL_MAKER, 1, out_loc, out_count)) return 1;
        }
        if (p->people_total < 7 && p->people_available >= 2 && food_deficit_next(p) <= 0) {
            if (try_place(b, p, LOC_HUT, 2, out_loc, out_count)) return 1;
        }
        int val;
        int slot = best_civ_slot(b, p, &val, 1);
        if (slot >= 0 && val >= 7) {
            if (try_place(b, p, LOC_CARD_BASE + slot, 1, out_loc, out_count)) return 1;
        }
        if (place_resource_loc(b, p, LOC_STONE, 2, out_loc, out_count)) return 1;
        if (place_resource_loc(b, p, LOC_BRICK, 2, out_loc, out_count)) return 1;
        if (place_resource_loc(b, p, LOC_HUNTING, 0, out_loc, out_count)) return 1;
        return 0;
    }

    if (phase == 1) {
        int val;
        int slot = best_civ_slot(b, p, &val, 1);
        if (slot >= 0 && val >= 4) {
            if (try_place(b, p, LOC_CARD_BASE + slot, 1, out_loc, out_count)) return 1;
        }
        int stack = best_affordable_building(b, p, &val);
        if (stack >= 0 && val >= 5) {
            if (try_place(b, p, LOC_BUILD_BASE + stack, 1, out_loc, out_count)) return 1;
        }
        if (p->food_production < p->people_total) {
            if (try_place(b, p, LOC_FIELD, 1, out_loc, out_count)) return 1;
        }
        if (sum_tools(p) < 6) {
            if (try_place(b, p, LOC_TOOL_MAKER, 1, out_loc, out_count)) return 1;
        }
        if (place_resource_loc(b, p, LOC_STONE, 3, out_loc, out_count)) return 1;
        if (place_resource_loc(b, p, LOC_GOLD,  3, out_loc, out_count)) return 1;
        if (place_resource_loc(b, p, LOC_HUNTING, 0, out_loc, out_count)) return 1;
        return 0;
    }

    int val;
    int stack = best_affordable_building(b, p, &val);
    if (stack >= 0) {
        if (try_place(b, p, LOC_BUILD_BASE + stack, 1, out_loc, out_count)) return 1;
    }
    int slot = best_civ_slot(b, p, &val, 1);
    if (slot >= 0) {
        if (try_place(b, p, LOC_CARD_BASE + slot, 1, out_loc, out_count)) return 1;
    }
    if (!bot_is_leading(p, opponent)) {
        for (int i = 0; i < NUM_BUILDING_STACKS; i++) {
            int remaining = BUILDINGS_PER_STACK - b->stack_counters[i];
            if (remaining <= 2 && b->location_occupants[LOC_BUILD_BASE + i] == 0) {
                BuildingCard bd = b->building_stacks[i][b->stack_counters[i]];
                if (!can_afford_building(p, &bd)) {
                    if (try_place(b, p, LOC_BUILD_BASE + i, 1, out_loc, out_count)) return 1;
                }
            }
        }
    }
    if (place_resource_loc(b, p, LOC_HUNTING, 0, out_loc, out_count)) return 1;
    return 0;
}

static int fallback_placement(Board *b, Player *p, int *out_loc, int *out_count) {
    if (place_resource_loc(b, p, LOC_HUNTING, 0, out_loc, out_count)) return 1;
    for (int loc = LOC_WOOD; loc <= LOC_GOLD; loc++) {
        if (place_resource_loc(b, p, loc, 0, out_loc, out_count)) return 1;
    }
    if (try_place(b, p, LOC_FIELD, 1, out_loc, out_count)) return 1;
    if (try_place(b, p, LOC_TOOL_MAKER, 1, out_loc, out_count)) return 1;
    if (try_place(b, p, LOC_HUT, 2, out_loc, out_count)) return 1;
    for (int i = 0; i < 4; i++) {
        if (try_place(b, p, LOC_CARD_BASE + i, 1, out_loc, out_count)) return 1;
    }
    for (int i = 0; i < NUM_BUILDING_STACKS; i++) {
        if (try_place(b, p, LOC_BUILD_BASE + i, 1, out_loc, out_count)) return 1;
    }
    return 0;
}

void bot_coleg_init(BotColegState *bs, ColegPersonality persona) {
    memset(bs, 0, sizeof(BotColegState));
    bs->personality = persona;
}

void bot_coleg_start_of_round(BotColegState *bs, int round_number) {
    bs->current_round = round_number;
}

int bot_coleg_choose_action(BotColegState *bs, Board *b, Player *bot_player, Player *opponent,
                             int *out_location, int *out_count) {
    if (bot_player->people_available <= 0) return 0;

    if (bs->current_round >= 60) {
        int stack = best_affordable_building(b, bot_player, NULL);
        if (stack >= 0) {
            if (try_place(b, bot_player, LOC_BUILD_BASE + stack, 1, out_location, out_count)) return 1;
        }
        for (int i = 0; i < 4; i++) {
            if (b->active_cards[i].id >= 0 && b->location_occupants[LOC_CARD_BASE + i] == 0) {
                int cost = i;
                int total_res = bot_player->player_resources[0] + bot_player->player_resources[1]
                              + bot_player->player_resources[2] + bot_player->player_resources[3];
                if (total_res >= cost) {
                    if (try_place(b, bot_player, LOC_CARD_BASE + i, 1, out_location, out_count)) return 1;
                }
            }
        }
        for (int i = 0; i < NUM_BUILDING_STACKS; i++) {
            int remaining = BUILDINGS_PER_STACK - b->stack_counters[i];
            if (remaining > 0 && remaining <= 3 && b->location_occupants[LOC_BUILD_BASE + i] == 0) {
                if (try_place(b, bot_player, LOC_BUILD_BASE + i, 1, out_location, out_count)) return 1;
            }
        }
    }

    int placed = 0;
    switch (bs->personality) {
        case COLEG_RESOURCE_RUSH:
            placed = strategy_resource_rush(bs, b, bot_player, out_location, out_count);
            break;
        case COLEG_CARD_SNIPER:
            placed = strategy_card_sniper(bs, b, bot_player, out_location, out_count);
            break;
        case COLEG_ENGINE_BUILDER:
            placed = strategy_engine_builder(bs, b, bot_player, opponent, out_location, out_count);
            break;
    }
    if (!placed) placed = fallback_placement(b, bot_player, out_location, out_count);
    return placed;
}

int bot_coleg_decide_tool_use(BotColegState *bs, Player *bot_player, int base_sum, int location_id) {
    (void)bs;
    int divisor = 2;
    if (location_id == LOC_WOOD) divisor = 3;
    else if (location_id == LOC_BRICK) divisor = 4;
    else if (location_id == LOC_STONE) divisor = 5;
    else if (location_id == LOC_GOLD)  divisor = 6;
    else if (location_id == LOC_HUNTING) divisor = 2;
    else return 0;

    int best = -1;
    int total = base_sum;
    for (int i = 0; i < 3; i++) {
        if (bot_player->permanent_tools[i] == 0 || bot_player->tools_used[i]) continue;
        if ((total + bot_player->permanent_tools[i]) / divisor > total / divisor) {
            if (best < 0 || bot_player->permanent_tools[i] < bot_player->permanent_tools[best]) {
                best = i;
            }
        }
    }
    return (best >= 0) ? (best + 1) : 0;
}

int bot_coleg_decide_card_payment(BotColegState *bs, Player *bot_player, int slot_index, int payment[4]) {
    int cost = slot_index;
    for (int r = 0; r < 4; r++) payment[r] = 0;
    if (cost == 0) return 1;

    int avail[4];
    for (int r = 0; r < 4; r++) avail[r] = bot_player->player_resources[r];

    int prefer_high = (bs->personality == COLEG_CARD_SNIPER) ? 0 : 0;
    (void)prefer_high;

    int paid = 0;
    for (int r = 0; r < 4 && paid < cost; r++) {
        while (avail[r] > 0 && paid < cost) {
            payment[r]++; avail[r]--; paid++;
        }
    }
    return (paid == cost) ? 1 : 0;
}

int bot_coleg_decide_building_purchase(BotColegState *bs, Player *bot_player, int stack_index, Board *b) {
    BuildingCard bd = b->building_stacks[stack_index][b->stack_counters[stack_index]];
    if (!can_afford_building(bot_player, &bd)) return 0;

    int points = bd.victory_points;

    switch (bs->personality) {
        case COLEG_RESOURCE_RUSH:
            return 1;
        case COLEG_CARD_SNIPER:
            return (points >= 7) ? 1 : 0;
        case COLEG_ENGINE_BUILDER: {
            int spent = bd.cost_wood + bd.cost_brick + bd.cost_stone + bd.cost_gold;
            int deficit = food_deficit_next(bot_player);
            if (deficit > 0 && total_resources(bot_player) - spent < deficit) return 0;
            return 1;
        }
    }
    return 1;
}

int bot_coleg_decide_feeding_resource_use(BotColegState *bs, Player *bot_player, int remaining_to_pay) {
    (void)bs;
    int tot = total_resources(bot_player);
    return (tot >= remaining_to_pay) ? 1 : 0;
}

const char *bot_coleg_personality_name(ColegPersonality p) {
    switch (p) {
        case COLEG_RESOURCE_RUSH:  return "Resource Rush";
        case COLEG_CARD_SNIPER:    return "Card Sniper";
        case COLEG_ENGINE_BUILDER: return "Engine Builder";
    }
    return "Unknown";
}
