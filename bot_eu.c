#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bot_eu.h"
#include "game_logic.h"

void bot_init(BotState *bs) {
    memset(bs, 0, sizeof(BotState));
    bs->strategy = STRAT_UNDECIDED;
    bs->mode = MODE_NORMAL;
    bs->initial_guess = STRAT_UNDECIDED;
}

void bot_observe_human(BotState *bs, Player *human) {
    bs->human_total_resources_seen = human->player_resources[0] + human->player_resources[1]
                                   + human->player_resources[2] + human->player_resources[3];
    bs->human_total_tools_seen = human->permanent_tools[0] + human->permanent_tools[1]
                               + human->permanent_tools[2];
    bs->human_total_people_seen = human->people_total;
    bs->human_total_food_prod_seen = human->food_production;
    bs->human_buildings_count_last = human->buildings_count;

    for (int i = 0; i < 6; i++) bs->human_cards_per_type[i] = 0;
    for (int i = 0; i < human->cards_count; i++) {
        int t = (int)human->inventory_cards[i].score_type;
        if (t >= 0 && t < 6) bs->human_cards_per_type[t]++;
    }

    for (int r = 0; r < 4; r++) bs->human_resource_snapshot[r] = human->player_resources[r];
    bs->human_food_snapshot = human->food_amount;
}

static BotStrategy guess_initial_strategy(BotState *bs) {
    int total = bs->human_total_resources_seen;

    if (bs->human_total_people_seen > 5 || bs->human_total_food_prod_seen >= 2) {
        return STRAT_AGGRO_BUILDER;
    }
    if (bs->human_total_tools_seen >= 2 || total >= 5) {
        return STRAT_CARD_RUSHER;
    }
    int card_diversity = 0;
    for (int i = 0; i < 6; i++) if (bs->human_cards_per_type[i] > 0) card_diversity++;
    if (card_diversity >= 2) {
        return STRAT_HOARDER;
    }

    int pick = rand() % 3;
    if (pick == 0) return STRAT_AGGRO_BUILDER;
    if (pick == 1) return STRAT_CARD_RUSHER;
    return STRAT_HOARDER;
}

void bot_start_of_round(BotState *bs, Board *b, Player *human, int round_number) {
    bs->current_round = round_number;
    bs->placements_this_round = 0;
    (void)b;

    if (bs->strategy == STRAT_UNDECIDED && round_number >= 3) {
        bs->strategy = guess_initial_strategy(bs);
        bs->initial_guess = bs->strategy;
    }

    bs->last_action_was_sabotage = 0;

    if (bs->mode == MODE_SABOTAGE && bs->sabotage_rounds_left > 0) {
        bs->sabotage_rounds_left--;
        if (bs->sabotage_rounds_left <= 0) {
            bs->mode = MODE_RECOVERY;
            bs->recovery_rounds_left = 1;
        }
    } else if (bs->mode == MODE_RECOVERY && bs->recovery_rounds_left > 0) {
        bs->recovery_rounds_left--;
        if (bs->recovery_rounds_left <= 0) bs->mode = MODE_NORMAL;
    }

    if (bs->mode == MODE_NORMAL && bs->strategy != STRAT_UNDECIDED && round_number >= 4) {
        int trigger_sabotage = 0;

        int human_can_afford_expensive_building = 0;
        if (human->player_resources[2] + human->player_resources[3] >= 2
            && human->player_resources[0] + human->player_resources[1] >= 2) {
            human_can_afford_expensive_building = 1;
        }

        int human_total_res = human->player_resources[0] + human->player_resources[1]
                            + human->player_resources[2] + human->player_resources[3];
        int human_swing = human_total_res - bs->human_total_resources_seen;

        if (human_can_afford_expensive_building) trigger_sabotage = 1;
        if (human_swing >= 3) trigger_sabotage = 1;
        if (human->food_production >= 4) trigger_sabotage = 1;
        if (human->cards_count >= 4) trigger_sabotage = 1;

        if (trigger_sabotage) {
            bs->mode = MODE_SABOTAGE;
            bs->sabotage_rounds_left = 1 + (rand() % 2);
        }
    }

    if (bs->human_blocked_me_streak >= 2 && bs->strategy != STRAT_UNDECIDED) {
        BotStrategy old = bs->strategy;
        int attempts = 0;
        while (bs->strategy == old && attempts < 5) {
            bs->strategy = (BotStrategy)(1 + (rand() % 3));
            attempts++;
        }
        bs->human_blocked_me_streak = 0;
    }
}

static int bot_try_place(BotState *bs, Board *b, Player *bot_player,
                          int loc, int count, int *out_loc, int *out_count) {
    if (is_placement_valid(b, bot_player, loc, count)) {
        *out_loc = loc;
        *out_count = count;
        bs->placements_this_round++;
        return 1;
    }
    return 0;
}

static int pick_sabotage_action(BotState *bs, Board *b, Player *bot_player, Player *human,
                                 int *out_loc, int *out_count) {
    bs->last_action_was_sabotage = 1;

    int human_res_max_idx = 0;
    for (int r = 1; r < 4; r++) {
        if (human->player_resources[r] > human->player_resources[human_res_max_idx])
            human_res_max_idx = r;
    }
    int target_resource_loc = LOC_WOOD + human_res_max_idx;
    if (bot_try_place(bs, b, bot_player, target_resource_loc, 1, out_loc, out_count)) return 1;

    if (human->food_production >= 3 || human->people_total >= 7) {
        if (bot_try_place(bs, b, bot_player, LOC_FIELD, 1, out_loc, out_count)) return 1;
        if (bot_try_place(bs, b, bot_player, LOC_HUT, 2, out_loc, out_count)) return 1;
    }

    int total_remaining = 0;
    int min_stack = -1;
    int min_remaining = BUILDINGS_PER_STACK + 1;
    for (int s = 0; s < NUM_BUILDING_STACKS; s++) {
        int rem = BUILDINGS_PER_STACK - b->stack_counters[s];
        total_remaining += rem;
        if (rem > 0 && rem < min_remaining) { min_remaining = rem; min_stack = s; }
    }
    if (min_stack != -1 && min_remaining <= 2) {
        if (bot_try_place(bs, b, bot_player, LOC_BUILD_BASE + min_stack, 1, out_loc, out_count)) return 1;
    }

    for (int s = 0; s < 4; s++) {
        if (b->active_cards[s].id < 0) continue;
        int t = (int)b->active_cards[s].score_type;
        if (t >= 0 && t < 6 && bs->human_cards_per_type[t] >= 1) {
            int cost = s;
            int player_total = bot_player->player_resources[0] + bot_player->player_resources[1]
                             + bot_player->player_resources[2] + bot_player->player_resources[3];
            if (cost == 0 || player_total >= cost) {
                if (bot_try_place(bs, b, bot_player, LOC_CARD_BASE + s, 1, out_loc, out_count)) return 1;
            }
        }
    }

    bs->last_action_was_sabotage = 0;
    return 0;
}

static int pick_normal_action_aggro_builder(BotState *bs, Board *b, Player *bot_player,
                                             int *out_loc, int *out_count) {
    for (int s = 0; s < NUM_BUILDING_STACKS; s++) {
        if (b->stack_counters[s] >= BUILDINGS_PER_STACK) continue;
        BuildingCard bd = b->building_stacks[s][b->stack_counters[s]];
        if (bot_player->player_resources[RES_WOOD]  >= bd.cost_wood
         && bot_player->player_resources[RES_BRICK] >= bd.cost_brick
         && bot_player->player_resources[RES_STONE] >= bd.cost_stone
         && bot_player->player_resources[RES_GOLD]  >= bd.cost_gold) {
            if (bot_try_place(bs, b, bot_player, LOC_BUILD_BASE + s, 1, out_loc, out_count)) return 1;
        }
    }

    int sum_tools = bot_player->permanent_tools[0] + bot_player->permanent_tools[1]
                  + bot_player->permanent_tools[2];
    if (sum_tools < 4) {
        if (bot_try_place(bs, b, bot_player, LOC_TOOL_MAKER, 1, out_loc, out_count)) return 1;
    }

    int target_locs[] = {LOC_STONE, LOC_WOOD, LOC_BRICK, LOC_GOLD};
    int worker_budget = bot_player->people_available >= 3 ? 3 : bot_player->people_available;
    if (worker_budget < 1) worker_budget = 1;
    for (int i = 0; i < 4; i++) {
        if (bot_try_place(bs, b, bot_player, target_locs[i], worker_budget, out_loc, out_count)) return 1;
        for (int c = worker_budget - 1; c >= 1; c--) {
            if (bot_try_place(bs, b, bot_player, target_locs[i], c, out_loc, out_count)) return 1;
        }
    }

    if (b->active_cards[0].id >= 0 && b->location_occupants[LOC_CARD_BASE + 0] == 0) {
        if (bot_try_place(bs, b, bot_player, LOC_CARD_BASE + 0, 1, out_loc, out_count)) return 1;
    }
    if (bot_try_place(bs, b, bot_player, LOC_HUNTING, bot_player->people_available, out_loc, out_count)) return 1;
    return 0;
}

static int pick_normal_action_card_rusher(BotState *bs, Board *b, Player *bot_player,
                                           int *out_loc, int *out_count) {
    int total_res = bot_player->player_resources[0] + bot_player->player_resources[1]
                  + bot_player->player_resources[2] + bot_player->player_resources[3];
    for (int s = 0; s < 4; s++) {
        if (b->active_cards[s].id < 0) continue;
        int cost = s;
        if (total_res >= cost) {
            if (bot_try_place(bs, b, bot_player, LOC_CARD_BASE + s, 1, out_loc, out_count)) return 1;
        }
    }

    int target_locs[] = {LOC_WOOD, LOC_BRICK, LOC_STONE, LOC_GOLD};
    int worker_budget = bot_player->people_available >= 2 ? 2 : bot_player->people_available;
    if (worker_budget < 1) worker_budget = 1;
    for (int i = 0; i < 4; i++) {
        if (bot_try_place(bs, b, bot_player, target_locs[i], worker_budget, out_loc, out_count)) return 1;
        if (bot_try_place(bs, b, bot_player, target_locs[i], 1, out_loc, out_count)) return 1;
    }

    int sum_tools = bot_player->permanent_tools[0] + bot_player->permanent_tools[1]
                  + bot_player->permanent_tools[2];
    if (sum_tools < 3) {
        if (bot_try_place(bs, b, bot_player, LOC_TOOL_MAKER, 1, out_loc, out_count)) return 1;
    }

    if (bot_try_place(bs, b, bot_player, LOC_HUNTING, bot_player->people_available, out_loc, out_count)) return 1;
    return 0;
}

static int pick_normal_action_hoarder(BotState *bs, Board *b, Player *bot_player,
                                       int *out_loc, int *out_count) {
    int sum_tools = bot_player->permanent_tools[0] + bot_player->permanent_tools[1]
                  + bot_player->permanent_tools[2];

    if (bs->current_round <= 5) {
        if (bot_player->people_total < 8 && bot_player->people_available >= 2) {
            if (bot_try_place(bs, b, bot_player, LOC_HUT, 2, out_loc, out_count)) return 1;
        }
        if (bot_player->food_production < 4) {
            if (bot_try_place(bs, b, bot_player, LOC_FIELD, 1, out_loc, out_count)) return 1;
        }
        if (sum_tools < 3) {
            if (bot_try_place(bs, b, bot_player, LOC_TOOL_MAKER, 1, out_loc, out_count)) return 1;
        }
    }

    if (bs->current_round >= 6) {
        for (int s = 0; s < 4; s++) {
            if (b->active_cards[s].id < 0) continue;
            CivCard c = b->active_cards[s];
            if (c.score_type == SCORE_MULTIPLY || c.score_type == SCORE_PER_FARM
             || c.score_type == SCORE_PER_HUT  || c.score_type == SCORE_PER_TOOL) {
                int cost = s;
                int total_res = bot_player->player_resources[0] + bot_player->player_resources[1]
                              + bot_player->player_resources[2] + bot_player->player_resources[3];
                if (total_res >= cost) {
                    if (bot_try_place(bs, b, bot_player, LOC_CARD_BASE + s, 1, out_loc, out_count)) return 1;
                }
            }
        }
    }

    int target_locs[] = {LOC_WOOD, LOC_BRICK, LOC_STONE};
    int worker_budget = bot_player->people_available >= 2 ? 2 : bot_player->people_available;
    if (worker_budget < 1) worker_budget = 1;
    for (int i = 0; i < 3; i++) {
        if (bot_try_place(bs, b, bot_player, target_locs[i], worker_budget, out_loc, out_count)) return 1;
        if (bot_try_place(bs, b, bot_player, target_locs[i], 1, out_loc, out_count)) return 1;
    }

    int food_needed = bot_player->people_total - bot_player->food_production;
    if (bot_player->food_amount < food_needed) {
        if (bot_try_place(bs, b, bot_player, LOC_HUNTING, bot_player->people_available, out_loc, out_count)) return 1;
    }

    if (b->active_cards[0].id >= 0 && b->location_occupants[LOC_CARD_BASE + 0] == 0) {
        if (bot_try_place(bs, b, bot_player, LOC_CARD_BASE + 0, 1, out_loc, out_count)) return 1;
    }
    if (bot_try_place(bs, b, bot_player, LOC_HUNTING, bot_player->people_available, out_loc, out_count)) return 1;
    return 0;
}

static int pick_early_round_action(BotState *bs, Board *b, Player *bot_player,
                                    int *out_loc, int *out_count) {
    int r = rand() % 5;
    int locs[5] = {LOC_TOOL_MAKER, LOC_FIELD, LOC_WOOD, LOC_BRICK, LOC_STONE};
    int chosen = locs[r];
    int count = (chosen == LOC_HUT) ? 2 : (chosen == LOC_TOOL_MAKER || chosen == LOC_FIELD) ? 1 : 2;
    if (count > bot_player->people_available) count = bot_player->people_available;
    if (bot_try_place(bs, b, bot_player, chosen, count, out_loc, out_count)) return 1;

    for (int loc = 0; loc < NUM_LOCATIONS; loc++) {
        for (int c = bot_player->people_available; c >= 1; c--) {
            if (bot_try_place(bs, b, bot_player, loc, c, out_loc, out_count)) return 1;
        }
    }
    return 0;
}

int bot_choose_action(BotState *bs, Board *b, Player *bot_player, Player *human,
                       int *out_location, int *out_count) {
    if (bot_player->people_available <= 0) return 0;

    if (bs->current_round >= 60) {
        int stack = -1;
        for (int s = 0; s < NUM_BUILDING_STACKS; s++) {
            if (b->stack_counters[s] >= BUILDINGS_PER_STACK) continue;
            BuildingCard bd = b->building_stacks[s][b->stack_counters[s]];
            if (bot_player->player_resources[RES_WOOD]  >= bd.cost_wood
             && bot_player->player_resources[RES_BRICK] >= bd.cost_brick
             && bot_player->player_resources[RES_STONE] >= bd.cost_stone
             && bot_player->player_resources[RES_GOLD]  >= bd.cost_gold) {
                stack = s; break;
            }
        }
        if (stack >= 0) {
            if (bot_try_place(bs, b, bot_player, LOC_BUILD_BASE + stack, 1, out_location, out_count)) return 1;
        }
        for (int i = 0; i < 4; i++) {
            if (b->active_cards[i].id >= 0 && b->location_occupants[LOC_CARD_BASE + i] == 0) {
                int cost = i;
                int total_res = bot_player->player_resources[0] + bot_player->player_resources[1]
                              + bot_player->player_resources[2] + bot_player->player_resources[3];
                if (total_res >= cost) {
                    if (bot_try_place(bs, b, bot_player, LOC_CARD_BASE + i, 1, out_location, out_count)) return 1;
                }
            }
        }
        for (int i = 0; i < NUM_BUILDING_STACKS; i++) {
            int remaining = BUILDINGS_PER_STACK - b->stack_counters[i];
            if (remaining > 0 && remaining <= 3 && b->location_occupants[LOC_BUILD_BASE + i] == 0) {
                if (bot_try_place(bs, b, bot_player, LOC_BUILD_BASE + i, 1, out_location, out_count)) return 1;
            }
        }
    }

    int locked_count = 0;
    for (int loc = LOC_WOOD; loc <= LOC_GOLD; loc++) {
        if (b->location_owner[loc] == human->player_id) locked_count++;
    }
    if (locked_count >= 2) bs->human_blocked_me_streak++;

    if (bs->mode == MODE_SABOTAGE && bs->strategy != STRAT_UNDECIDED) {
        if (pick_sabotage_action(bs, b, bot_player, human, out_location, out_count)) return 1;
    }

    if (bs->strategy == STRAT_UNDECIDED) {
        if (pick_early_round_action(bs, b, bot_player, out_location, out_count)) return 1;
        return 0;
    }

    switch (bs->strategy) {
        case STRAT_AGGRO_BUILDER:
            if (pick_normal_action_aggro_builder(bs, b, bot_player, out_location, out_count)) return 1;
            break;
        case STRAT_CARD_RUSHER:
            if (pick_normal_action_card_rusher(bs, b, bot_player, out_location, out_count)) return 1;
            break;
        case STRAT_HOARDER:
            if (pick_normal_action_hoarder(bs, b, bot_player, out_location, out_count)) return 1;
            break;
        default:
            break;
    }

    for (int loc = 0; loc < NUM_LOCATIONS; loc++) {
        for (int c = bot_player->people_available; c >= 1; c--) {
            if (bot_try_place(bs, b, bot_player, loc, c, out_location, out_count)) return 1;
        }
    }
    return 0;
}

int bot_decide_tool_use(BotState *bs, Player *bot_player, int base_sum, int location_id) {
    (void)bs;
    int divisor = 2;
    if (location_id == LOC_WOOD) divisor = 3;
    else if (location_id == LOC_BRICK) divisor = 4;
    else if (location_id == LOC_STONE) divisor = 5;
    else if (location_id == LOC_GOLD)  divisor = 6;
    else if (location_id == LOC_HUNTING) divisor = 2;
    else return 0;

    int current_yield = base_sum / divisor;
    int next_threshold = (current_yield + 1) * divisor;
    int deficit = next_threshold - base_sum;

    if (deficit <= 0) return 0;

    int best_slot = -1;
    int best_excess = 999;
    for (int i = 0; i < 3; i++) {
        if (bot_player->permanent_tools[i] == 0 || bot_player->tools_used[i]) continue;
        if (bot_player->permanent_tools[i] >= deficit) {
            int excess = bot_player->permanent_tools[i] - deficit;
            if (excess < best_excess) { best_excess = excess; best_slot = i; }
        }
    }
    if (best_slot >= 0) return best_slot + 1;
    return 0;
}

int bot_decide_card_payment(BotState *bs, Player *bot_player, int slot_index,
                              int payment[4]) {
    (void)bs;
    int cost = slot_index;
    if (cost == 0) { for (int r = 0; r < 4; r++) payment[r] = 0; return 1; }

    int avail[4];
    for (int r = 0; r < 4; r++) { avail[r] = bot_player->player_resources[r]; payment[r] = 0; }

    int paid = 0;
    for (int r = 0; r < 4 && paid < cost; r++) {
        while (avail[r] > 0 && paid < cost) {
            payment[r]++; avail[r]--; paid++;
        }
    }
    return (paid == cost) ? 1 : 0;
}

int bot_decide_building_purchase(BotState *bs, Player *bot_player, int stack_index) {
    (void)bot_player;
    (void)stack_index;
    if (bs->strategy == STRAT_AGGRO_BUILDER) return 1;
    if (bs->mode == MODE_SABOTAGE) return 1;
    return 1;
}

int bot_decide_feeding_resource_use(BotState *bs, Player *bot_player, int remaining_to_pay) {
    (void)bs;
    int total_res = bot_player->player_resources[0] + bot_player->player_resources[1]
                  + bot_player->player_resources[2] + bot_player->player_resources[3];
    if (total_res >= remaining_to_pay) return 1;
    return 0;
}

void bot_decide_building_variable_payment(BotState *bs, Player *bot_player,
                                          int building_type, int payment[4]) {
    (void)bs;
    for (int r = 0; r < 4; r++) payment[r] = 0;

    int values[4] = {3, 4, 5, 6};

    if (building_type == 1) {
        int idx[4] = {3, 2, 1, 0};
        int chosen[2] = {-1, -1};
        int chosen_count = 0;
        for (int i = 0; i < 4 && chosen_count < 2; i++) {
            if (bot_player->player_resources[idx[i]] >= 1) {
                chosen[chosen_count++] = idx[i];
            }
        }
        if (chosen_count < 2) return;

        int paid = 0;
        for (int turn = 0; turn < 8 && paid < 4; turn++) {
            int k = turn % 2;
            int r = chosen[k];
            if (payment[r] + 1 <= bot_player->player_resources[r]) {
                payment[r]++; paid++;
            } else {
                int other = chosen[1 - k];
                if (payment[other] + 1 <= bot_player->player_resources[other]) {
                    payment[other]++; paid++;
                }
            }
        }
        if (paid < 4) {
            for (int r = 0; r < 4; r++) payment[r] = 0;
        }
        (void)values;
        return;
    }

    if (building_type == 2) {
        int paid = 0;
        int order[4] = {3, 2, 1, 0};
        for (int i = 0; i < 4 && paid < 7; i++) {
            int r = order[i];
            while (payment[r] < bot_player->player_resources[r] && paid < 7) {
                payment[r]++; paid++;
            }
        }
        if (paid == 0) {
            for (int r = 0; r < 4; r++) {
                if (bot_player->player_resources[r] > 0) { payment[r] = 1; return; }
            }
        }
    }
}
