#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game_logic.h"
#include "persistence.h"
#include "bot_glue.h"
#include "bot_eu.h"
#include "bot_coleg.h"

int roll_dice(int people_count) {
    int total_sum = 0;
    for (int i = 0; i < people_count; i++) {
        total_sum += (rand() % 6) + 1;
    }
    return total_sum;
}

void shuffle_all(Board *b) {
    for (int i = CIV_TOTAL_CARDS - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        CivCard temp = b->civilization_deck[i];
        b->civilization_deck[i] = b->civilization_deck[j];
        b->civilization_deck[j] = temp;
    }
    for (int s = 0; s < NUM_BUILDING_STACKS; s++) {
        for (int i = BUILDINGS_PER_STACK - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            BuildingCard temp = b->building_stacks[s][i];
            b->building_stacks[s][i] = b->building_stacks[s][j];
            b->building_stacks[s][j] = temp;
        }
    }
}

void initialize_game(Board *b, Player players[], int num_players) {
    b->food_supply = 999;
    b->global_supply[RES_WOOD]  = 20;
    b->global_supply[RES_BRICK] = 16;
    b->global_supply[RES_STONE] = 12;
    b->global_supply[RES_GOLD]  = 10;
    b->deck_index = 0;
    b->start_player = 0;

    for (int i = 0; i < NUM_LOCATIONS; i++) {
        b->location_occupants[i] = 0;
        b->location_owner[i] = -1;
        if (i < NUM_BUILDING_STACKS) b->stack_counters[i] = 0;
    }
    for (int i = 0; i < 3; i++) b->village_used[i] = false;

    memcpy(b->civilization_deck, CIV_DECK_TEMPLATE, sizeof(CIV_DECK_TEMPLATE));
    memcpy(b->building_stacks, BUILDING_STACK_TEMPLATE, sizeof(BUILDING_STACK_TEMPLATE));

    shuffle_all(b);

    for (int i = 0; i < 4; i++) {
        b->active_cards[i] = b->civilization_deck[b->deck_index];
        b->deck_index++;
    }

    for (int p = 0; p < num_players; p++) {
        players[p].player_id        = p;
        players[p].people_total     = STARTING_PEOPLE;
        players[p].people_available = STARTING_PEOPLE;
        players[p].food_amount      = STARTING_FOOD;
        players[p].food_production  = 0;
        players[p].current_score    = 0;
        players[p].cards_count      = 0;
        players[p].buildings_count  = 0;
        players[p].temporary_tool   = 0;

        for (int i = 0; i < 4; i++) players[p].player_resources[i] = 0;
        for (int i = 0; i < 3; i++) {
            players[p].permanent_tools[i] = 0;
            players[p].tools_used[i] = 0;
        }
        for (int i = 0; i < NUM_LOCATIONS; i++) players[p].people_on_location[i] = 0;
    }
}

int is_placement_valid(Board *b, Player *p, int location_id, int count) {
    if (location_id < 0 || location_id >= NUM_LOCATIONS) return 0;
    if (count <= 0 || count > p->people_available) return 0;
    if (p->people_on_location[location_id] > 0) return 0;

    if (location_id == LOC_TOOL_MAKER || location_id == LOC_FIELD) {
        if (count != 1) return 0;
        if (b->location_occupants[location_id] > 0) return 0;
    }
    if (location_id == LOC_HUT) {
        if (count != 2) return 0;
        if (b->location_occupants[location_id] > 0) return 0;
    }

    if (location_id == LOC_TOOL_MAKER || location_id == LOC_FIELD || location_id == LOC_HUT) {
        int village_idx;
        if (location_id == LOC_TOOL_MAKER) village_idx = 0;
        else if (location_id == LOC_FIELD) village_idx = 1;
        else village_idx = 2;

        int already_used = 0;
        for (int i = 0; i < 3; i++) if (b->village_used[i]) already_used++;
        if (already_used >= 2 && !b->village_used[village_idx]) return 0;
    }

    if (location_id >= LOC_WOOD && location_id <= LOC_GOLD) {
        if (b->location_owner[location_id] != -1) return 0;
        if (count > RESOURCE_SPACE_LIMIT) return 0;
    }

    if (location_id == LOC_HUNTING) {
        if (count > p->people_available) return 0;
    }

    if (location_id >= LOC_CARD_BASE && location_id <= LOC_CARD_BASE + 3) {
        if (count != 1 || b->location_occupants[location_id] > 0) return 0;
        if (b->active_cards[location_id - LOC_CARD_BASE].id < 0) return 0;
    }
    if (location_id >= LOC_BUILD_BASE && location_id <= LOC_BUILD_BASE + 3) {
        if (count != 1 || b->location_occupants[location_id] > 0) return 0;
        if (b->stack_counters[location_id - LOC_BUILD_BASE] >= BUILDINGS_PER_STACK) return 0;
    }
    return 1;
}

int place_workers(Board *b, Player *p, int location_id, int count) {
    if (is_placement_valid(b, p, location_id, count) == 0) return 0;

    p->people_on_location[location_id] += count;
    p->people_available -= count;
    b->location_occupants[location_id] += count;

    if (location_id == LOC_TOOL_MAKER) {
        int lowest_val = p->permanent_tools[0];
        int target_idx = 0;
        for (int i = 1; i < 3; i++) {
            if (p->permanent_tools[i] < lowest_val) {
                lowest_val = p->permanent_tools[i];
                target_idx = i;
            }
        }
        if (p->permanent_tools[target_idx] < MAX_TOOL_VALUE) {
            p->permanent_tools[target_idx]++;
        }
        char msg[120];
        sprintf(msg, "Player %d placed worker at Tool Maker -> received new tool (slot %d now value %d).",
                p->player_id + 1, target_idx + 1, p->permanent_tools[target_idx]);
        log_action(msg);
    }

    if (location_id == LOC_TOOL_MAKER) b->village_used[0] = true;
    else if (location_id == LOC_FIELD)  b->village_used[1] = true;
    else if (location_id == LOC_HUT)    b->village_used[2] = true;

    if (location_id >= LOC_WOOD && location_id <= LOC_GOLD) {
        b->location_owner[location_id] = p->player_id;
    }

    return 1;
}

int apply_tools_interactive(Player *p, int base_sum, const char *context) {
    int total = base_sum;
    int any_available = 0;
    for (int i = 0; i < 3; i++) {
        if (p->permanent_tools[i] > 0 && p->tools_used[i] == 0) any_available = 1;
    }
    if (!any_available) return total;

    if (p->is_bot) {
        while (1) {
            int slot = 0;
            if (p->bot_kind == BOT_KIND_COLEG) {
                BotColegState *bc = bot_glue_get_active_coleg(p->player_id);
                if (!bc) break;
                slot = bot_coleg_decide_tool_use(bc, p, total, p->people_on_location[0]);
                for (int loc = 0; loc < NUM_LOCATIONS; loc++) {
                    if (p->people_on_location[loc] > 0) {
                        slot = bot_coleg_decide_tool_use(bc, p, total, loc);
                        break;
                    }
                }
            } else {
                BotState *be = bot_glue_get_active_eu(p->player_id);
                if (!be) break;
                int active_loc = LOC_HUNTING;
                for (int loc = 0; loc < NUM_LOCATIONS; loc++) {
                    if (p->people_on_location[loc] > 0) { active_loc = loc; break; }
                }
                slot = bot_decide_tool_use(be, p, total, active_loc);
            }
            if (slot <= 0 || slot > 3) break;
            int idx = slot - 1;
            if (p->permanent_tools[idx] == 0 || p->tools_used[idx]) break;
            total += p->permanent_tools[idx];
            p->tools_used[idx] = 1;
            printf(COLOR_GREEN "  [Bot P%d] Tool slot %d (+%d) used. New total: %d\n" COLOR_RESET,
                   p->player_id + 1, slot, p->permanent_tools[idx], total);
        }
        return total;
    }

    printf("\n" COLOR_CYAN "  >> Player %d <<" COLOR_RESET " at " COLOR_YELLOW "%s" COLOR_RESET ": rolled total = " COLOR_BOLD "%d" COLOR_RESET "\n",
           p->player_id + 1, context, base_sum);
    printf("  Your tools: ");
    for (int i = 0; i < 3; i++) {
        if (p->permanent_tools[i] == 0) {
            printf(COLOR_GRAY "[--] " COLOR_RESET);
        } else if (p->tools_used[i]) {
            printf(COLOR_DIM "[%d used] " COLOR_RESET, p->permanent_tools[i]);
        } else {
            printf(COLOR_GREEN "[Slot %d: +%d] " COLOR_RESET, i + 1, p->permanent_tools[i]);
        }
    }
    printf("\n");

    while (1) {
        int available_count = 0;
        for (int i = 0; i < 3; i++) {
            if (p->permanent_tools[i] > 0 && p->tools_used[i] == 0) available_count++;
        }
        if (available_count == 0) break;

        printf("  Current total: " COLOR_BOLD "%d" COLOR_RESET ". Use a tool? Enter slot (1-3) or 0 to stop: ", total);
        int choice = -1;
        if (scanf("%d", &choice) != 1) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            choice = 0;
        }
        if (choice == 0) break;
        if (choice < 1 || choice > 3) {
            printf(COLOR_RED "  Invalid slot.\n" COLOR_RESET);
            continue;
        }
        int idx = choice - 1;
        if (p->permanent_tools[idx] == 0) {
            printf(COLOR_RED "  Slot %d is empty.\n" COLOR_RESET, choice);
            continue;
        }
        if (p->tools_used[idx]) {
            printf(COLOR_RED "  Slot %d already used this round.\n" COLOR_RESET, choice);
            continue;
        }
        total += p->permanent_tools[idx];
        p->tools_used[idx] = 1;
        printf(COLOR_GREEN "  Tool slot %d (+%d) used. New total: %d\n" COLOR_RESET,
               choice, p->permanent_tools[idx], total);
    }
    return total;
}

int apply_tools_auto(Player *p, int base_sum) {
    int total = base_sum;
    for (int i = 0; i < 3; i++) {
        if (p->permanent_tools[i] > 0 && p->tools_used[i] == 0) {
            total += p->permanent_tools[i];
            p->tools_used[i] = 1;
        }
    }
    return total;
}

void evaluate_resources(Player *p, Board *b, int location_id, int extra_dice_bonus) {
    int workers = p->people_on_location[location_id];
    if (workers == 0) return;

    int divisor[8] = {2, 0, 0, 0, 3, 4, 5, 6};

    int base = roll_dice(workers) + extra_dice_bonus;

    const char *loc_name = "Unknown";
    if (location_id == LOC_HUNTING) loc_name = "Hunting (food /2)";
    else if (location_id == LOC_WOOD)  loc_name = "Forest (wood /3)";
    else if (location_id == LOC_BRICK) loc_name = "Clay pit (brick /4)";
    else if (location_id == LOC_STONE) loc_name = "Quarry (stone /5)";
    else if (location_id == LOC_GOLD)  loc_name = "River (gold /6)";

    int total = apply_tools_interactive(p, base, loc_name);

    int yield = total / divisor[location_id];

    if (location_id == LOC_HUNTING) {
        p->food_amount += yield;
        if (b->food_supply >= yield) b->food_supply -= yield;
        else b->food_supply = 0;
        printf(COLOR_GREEN "  -> Gained %d food.\n" COLOR_RESET, yield);
    } else {
        int res = location_id - LOC_WOOD;
        int actually = yield;
        if (actually > b->global_supply[res]) actually = b->global_supply[res];
        p->player_resources[res] += actually;
        b->global_supply[res] -= actually;
        const char *names[] = {"wood", "brick", "stone", "gold"};
        printf(COLOR_GREEN "  -> Gained %d %s.\n" COLOR_RESET, actually, names[res]);
    }

    char log_msg[160];
    sprintf(log_msg, "Player %d evaluated location %d: rolled=%d, total_with_tools=%d, yield=%d.",
            p->player_id + 1, location_id, base, total, yield);
    log_action(log_msg);

    b->location_occupants[location_id] -= workers;
    p->people_available += workers;
    p->people_on_location[location_id] = 0;
}

void evaluate_special_locations(Player *p, Board *b, int location_id) {
    int workers = p->people_on_location[location_id];
    if (workers == 0) return;

    if (location_id == LOC_FIELD) {
        if (p->food_production < MAX_PEOPLE) {
            p->food_production++;
            printf(COLOR_GREEN "  Player %d Field -> food production now %d.\n" COLOR_RESET,
                   p->player_id + 1, p->food_production);
        }
    } else if (location_id == LOC_HUT) {
        if (p->people_total < MAX_PEOPLE) {
            p->people_total++;
            printf(COLOR_GREEN "  Player %d Hut -> total people now %d.\n" COLOR_RESET,
                   p->player_id + 1, p->people_total);
        }
    }

    b->location_occupants[location_id] -= workers;
    p->people_available += workers;
    p->people_on_location[location_id] = 0;
}

int evaluate_civ_card_acquisition(Player *p, Board *b, int slot_index) {
    int loc = LOC_CARD_BASE + slot_index;
    int workers = p->people_on_location[loc];
    if (workers == 0) return 0;
    if (b->active_cards[slot_index].id < 0) {
        b->location_occupants[loc] -= workers;
        p->people_available += workers;
        p->people_on_location[loc] = 0;
        return 0;
    }

    CivCard card = b->active_cards[slot_index];
    int cost = slot_index;

    printf("\n" COLOR_MAGENTA "  Player %d wants Civ Card slot %d: \"%s\" (cost %d resource(s)).\n" COLOR_RESET,
           p->player_id + 1, slot_index + 1, card.name, cost);

    int total_owned = p->player_resources[0] + p->player_resources[1]
                    + p->player_resources[2] + p->player_resources[3];
    bool acquired = false;

    if (cost == 0) {
        acquired = true;
    } else if (total_owned < cost) {
        printf(COLOR_RED "  Not enough resources (have %d, need %d). Worker returned.\n" COLOR_RESET, total_owned, cost);
    } else if (p->is_bot) {
        int payment[4] = {0, 0, 0, 0};
        int ok = 0;
        if (p->bot_kind == BOT_KIND_COLEG) {
            BotColegState *bc = bot_glue_get_active_coleg(p->player_id);
            if (bc) ok = bot_coleg_decide_card_payment(bc, p, slot_index, payment);
        } else {
            BotState *be = bot_glue_get_active_eu(p->player_id);
            if (be) ok = bot_decide_card_payment(be, p, slot_index, payment);
        }
        if (ok) {
            for (int r = 0; r < 4; r++) {
                p->player_resources[r] -= payment[r];
                b->global_supply[r] += payment[r];
            }
            acquired = true;
        }
    } else {
        printf("  Pay %d resource(s). Your resources: W=%d B=%d S=%d G=%d\n",
               cost, p->player_resources[0], p->player_resources[1],
               p->player_resources[2], p->player_resources[3]);
        int paid = 0;
        int payment[4] = {0,0,0,0};
        while (paid < cost) {
            printf("  Pay [%d/%d] - choose resource (1=Wood 2=Brick 3=Stone 4=Gold, 0=cancel): ",
                   paid + 1, cost);
            int ch = 0;
            if (scanf("%d", &ch) != 1) { int c; while ((c=getchar())!='\n'&&c!=EOF){} ch=0; }
            if (ch == 0) break;
            if (ch < 1 || ch > 4) { printf(COLOR_RED "  Invalid.\n" COLOR_RESET); continue; }
            if (p->player_resources[ch-1] - payment[ch-1] <= 0) {
                printf(COLOR_RED "  You don't have that resource available.\n" COLOR_RESET);
                continue;
            }
            payment[ch-1]++;
            paid++;
        }
        if (paid == cost) {
            for (int r = 0; r < 4; r++) {
                p->player_resources[r] -= payment[r];
                b->global_supply[r] += payment[r];
            }
            acquired = true;
        } else {
            printf(COLOR_YELLOW "  Purchase cancelled. Worker returned.\n" COLOR_RESET);
        }
    }

    if (acquired) {
        p->inventory_cards[p->cards_count++] = card;

        p->food_amount     += card.bonus_food;
        p->player_resources[RES_WOOD]  += card.bonus_wood;
        p->player_resources[RES_BRICK] += card.bonus_brick;
        p->player_resources[RES_STONE] += card.bonus_stone;
        p->player_resources[RES_GOLD]  += card.bonus_gold;
        b->global_supply[RES_WOOD]   -= card.bonus_wood;
        b->global_supply[RES_BRICK]  -= card.bonus_brick;
        b->global_supply[RES_STONE]  -= card.bonus_stone;
        b->global_supply[RES_GOLD]   -= card.bonus_gold;
        for (int r = 0; r < 4; r++) if (b->global_supply[r] < 0) b->global_supply[r] = 0;

        if (card.tool_bonus > 0) {
            int lowest_val = p->permanent_tools[0];
            int target_idx = 0;
            for (int i = 1; i < 3; i++) {
                if (p->permanent_tools[i] < lowest_val) {
                    lowest_val = p->permanent_tools[i];
                    target_idx = i;
                }
            }
            if (p->permanent_tools[target_idx] < MAX_TOOL_VALUE) p->permanent_tools[target_idx]++;
        }

        printf(COLOR_GREEN "  Acquired \"%s\". Bonuses applied.\n" COLOR_RESET, card.name);
        char lm[160];
        sprintf(lm, "Player %d acquired civ card \"%s\" (slot %d).",
                p->player_id + 1, card.name, slot_index + 1);
        log_action(lm);

        b->active_cards[slot_index].id = -1;
    }

    b->location_occupants[loc] -= workers;
    p->people_available += workers;
    p->people_on_location[loc] = 0;
    return acquired ? 1 : 0;
}

int evaluate_building_acquisition(Player *p, Board *b, int stack_index) {
    int loc = LOC_BUILD_BASE + stack_index;
    int workers = p->people_on_location[loc];
    if (workers == 0) return 0;
    if (b->stack_counters[stack_index] >= BUILDINGS_PER_STACK) {
        b->location_occupants[loc] -= workers;
        p->people_available += workers;
        p->people_on_location[loc] = 0;
        return 0;
    }

    BuildingCard bd = b->building_stacks[stack_index][b->stack_counters[stack_index]];
    bool acquired = false;

    static const int RES_VP_VALUES[4] = {3, 4, 5, 6};
    static const char *RES_NAMES[4]   = {"wood", "brick", "stone", "gold"};

    if (bd.type == BUILD_FIXED) {
        printf("\n" COLOR_MAGENTA "  Player %d wants Building \"%s\" (cost W=%d B=%d S=%d G=%d, VP=%d).\n" COLOR_RESET,
               p->player_id + 1, bd.name, bd.cost_wood, bd.cost_brick, bd.cost_stone, bd.cost_gold, bd.victory_points);

        bool can_pay = (p->player_resources[RES_WOOD]  >= bd.cost_wood) &&
                       (p->player_resources[RES_BRICK] >= bd.cost_brick) &&
                       (p->player_resources[RES_STONE] >= bd.cost_stone) &&
                       (p->player_resources[RES_GOLD]  >= bd.cost_gold);

        if (!can_pay) {
            printf(COLOR_RED "  Cannot afford. Worker returned.\n" COLOR_RESET);
        } else {
            int conf = 0;
            if (p->is_bot) {
                if (p->bot_kind == BOT_KIND_COLEG) {
                    BotColegState *bc = bot_glue_get_active_coleg(p->player_id);
                    if (bc) conf = bot_coleg_decide_building_purchase(bc, p, stack_index, b);
                } else {
                    BotState *be = bot_glue_get_active_eu(p->player_id);
                    if (be) conf = bot_decide_building_purchase(be, p, stack_index);
                }
            } else {
                printf("  Confirm purchase? (1=yes, 0=no): ");
                if (scanf("%d", &conf) != 1) { int c; while ((c=getchar())!='\n'&&c!=EOF){} conf=0; }
            }
            if (conf == 1) {
                p->player_resources[RES_WOOD]  -= bd.cost_wood;
                p->player_resources[RES_BRICK] -= bd.cost_brick;
                p->player_resources[RES_STONE] -= bd.cost_stone;
                p->player_resources[RES_GOLD]  -= bd.cost_gold;
                b->global_supply[RES_WOOD]   += bd.cost_wood;
                b->global_supply[RES_BRICK]  += bd.cost_brick;
                b->global_supply[RES_STONE]  += bd.cost_stone;
                b->global_supply[RES_GOLD]   += bd.cost_gold;

                p->inventory_buildings[p->buildings_count++] = bd;
                p->current_score += bd.victory_points;
                b->stack_counters[stack_index]++;
                acquired = true;
                printf(COLOR_GREEN "  Built \"%s\" for %d VP.\n" COLOR_RESET, bd.name, bd.victory_points);
                char lm[160];
                sprintf(lm, "Player %d built \"%s\" (stack %d) for %d VP.",
                        p->player_id + 1, bd.name, stack_index + 1, bd.victory_points);
                log_action(lm);
            } else if (!p->is_bot) {
                printf(COLOR_YELLOW "  Purchase cancelled.\n" COLOR_RESET);
            }
        }

    } else if (bd.type == BUILD_2KIND_4RES) {
        printf("\n" COLOR_MAGENTA
               "  Player %d wants \"%s\" (SPECIAL: pay exactly 4 resources from 2 different kinds).\n"
               COLOR_RESET, p->player_id + 1, bd.name);

        int payment[4] = {0, 0, 0, 0};
        int got_payment = 0;

        if (p->is_bot) {
            if (p->bot_kind == BOT_KIND_COLEG) {
                BotColegState *bc = bot_glue_get_active_coleg(p->player_id);
                if (bc) { bot_decide_building_variable_payment(NULL, p, 1, payment); got_payment = 1; }
            } else {
                BotState *be = bot_glue_get_active_eu(p->player_id);
                if (be) { bot_decide_building_variable_payment(be, p, 1, payment); got_payment = 1; }
            }
            int kinds = 0, total_p = 0;
            for (int r = 0; r < 4; r++) { if (payment[r] > 0) kinds++; total_p += payment[r]; }
            if (kinds != 2 || total_p != 4) got_payment = 0;
        } else {
            printf("  VP = sum of resource values (Wood=3, Brick=4, Stone=5, Gold=6)\n");
            printf("  Your resources: W=%d B=%d S=%d G=%d\n",
                   p->player_resources[0], p->player_resources[1],
                   p->player_resources[2], p->player_resources[3]);
            int r1 = -1, qty1 = 0, r2 = -1, qty2 = 0;
            while (r1 < 0) {
                printf("  First resource kind (1=Wood, 2=Brick, 3=Stone, 4=Gold, 0=cancel): ");
                int ch = 0;
                if (scanf("%d", &ch) != 1) { int c; while ((c=getchar())!='\n'&&c!=EOF){} ch=0; }
                if (ch == 0) { goto build_done; }
                if (ch < 1 || ch > 4) { printf(COLOR_RED "  Invalid.\n" COLOR_RESET); continue; }
                r1 = ch - 1;
            }
            while (qty1 <= 0) {
                printf("  How many %s (1-3, have %d): ", RES_NAMES[r1], p->player_resources[r1]);
                if (scanf("%d", &qty1) != 1) { int c; while ((c=getchar())!='\n'&&c!=EOF){} qty1=0; }
                if (qty1 < 1 || qty1 > 3 || p->player_resources[r1] < qty1) { qty1 = 0; }
            }
            qty2 = 4 - qty1;
            while (r2 < 0) {
                printf("  Second kind (need %d, different from %s, 0=cancel): ", qty2, RES_NAMES[r1]);
                int ch = 0;
                if (scanf("%d", &ch) != 1) { int c; while ((c=getchar())!='\n'&&c!=EOF){} ch=0; }
                if (ch == 0) { goto build_done; }
                if (ch < 1 || ch > 4) { continue; }
                r2 = ch - 1;
                if (r2 == r1 || p->player_resources[r2] < qty2) { r2 = -1; }
            }
            payment[r1] = qty1; payment[r2] = qty2;
            got_payment = 1;
        }

        if (got_payment) {
            int vp = 0;
            for (int r = 0; r < 4; r++) vp += payment[r] * RES_VP_VALUES[r];
            for (int r = 0; r < 4; r++) { p->player_resources[r] -= payment[r]; b->global_supply[r] += payment[r]; }
            BuildingCard abd = bd; abd.victory_points = vp;
            p->inventory_buildings[p->buildings_count++] = abd;
            p->current_score += vp; b->stack_counters[stack_index]++; acquired = true;
            printf(COLOR_GREEN "  Built \"%s\" for %d VP.\n" COLOR_RESET, bd.name, vp);
            char lm[160];
            sprintf(lm, "Player %d built \"%s\" (2-kind special) for %d VP.", p->player_id+1, bd.name, vp);
            log_action(lm);
        } else if (!p->is_bot) {
            printf(COLOR_YELLOW "  Purchase cancelled.\n" COLOR_RESET);
        }

    } else if (bd.type == BUILD_ANY_1TO7) {
        printf("\n" COLOR_MAGENTA
               "  Player %d wants \"%s\" (SPECIAL: pay 1 to 7 resources of any kind).\n"
               COLOR_RESET, p->player_id + 1, bd.name);

        int payment[4] = {0, 0, 0, 0};
        int total_paid = 0;
        int got_payment = 0;

        if (p->is_bot) {
            if (p->bot_kind == BOT_KIND_COLEG) {
                BotColegState *bc = bot_glue_get_active_coleg(p->player_id);
                if (bc) bot_decide_building_variable_payment(NULL, p, 2, payment);
            } else {
                BotState *be = bot_glue_get_active_eu(p->player_id);
                if (be) bot_decide_building_variable_payment(be, p, 2, payment);
            }
            for (int r = 0; r < 4; r++) total_paid += payment[r];
            if (total_paid >= 1 && total_paid <= 7) got_payment = 1;
        } else {
            printf("  VP = sum of resource values (Wood=3, Brick=4, Stone=5, Gold=6)\n");
            printf("  Your resources: W=%d B=%d S=%d G=%d\n",
                   p->player_resources[0], p->player_resources[1],
                   p->player_resources[2], p->player_resources[3]);
            bool valid = false;
            while (!valid) {
                printf("  Enter amount for each resource (0 to skip):\n");
                total_paid = 0;
                for (int r = 0; r < 4; r++) {
                    printf("    %s (have %d): ", RES_NAMES[r], p->player_resources[r]);
                    int qty = 0;
                    if (scanf("%d", &qty) != 1) { int c; while ((c=getchar())!='\n'&&c!=EOF){} qty=0; }
                    if (qty < 0) qty = 0;
                    if (qty > p->player_resources[r]) qty = p->player_resources[r];
                    payment[r] = qty; total_paid += qty;
                }
                if (total_paid < 1 || total_paid > 7) {
                    if (total_paid == 0) { goto build_done; }
                } else { valid = true; got_payment = 1; }
            }
        }

        if (got_payment) {
            int vp = 0;
            for (int r = 0; r < 4; r++) vp += payment[r] * RES_VP_VALUES[r];
            for (int r = 0; r < 4; r++) { p->player_resources[r] -= payment[r]; b->global_supply[r] += payment[r]; }
            BuildingCard abd = bd; abd.victory_points = vp;
            p->inventory_buildings[p->buildings_count++] = abd;
            p->current_score += vp; b->stack_counters[stack_index]++; acquired = true;
            printf(COLOR_GREEN "  Built \"%s\" for %d VP.\n" COLOR_RESET, bd.name, vp);
            char lm[160];
            sprintf(lm, "Player %d built \"%s\" (any-res special) for %d VP.", p->player_id+1, bd.name, vp);
            log_action(lm);
        } else if (!p->is_bot) {
            printf(COLOR_YELLOW "  Purchase cancelled.\n" COLOR_RESET);
        }
    }

build_done:
    b->location_occupants[loc] -= workers;
    p->people_available += workers;
    p->people_on_location[loc] = 0;
    return acquired ? 1 : 0;
}

void refill_civilization_cards(Board *b) {
    int target = 3;
    for (int i = 3; i >= 0; i--) {
        if (b->active_cards[i].id != -1) {
            CivCard temp = b->active_cards[i];
            b->active_cards[i].id = -1;
            b->active_cards[target] = temp;
            target--;
        }
    }
    for (int i = 0; i < 4; i++) {
        if (b->active_cards[i].id == -1 && b->deck_index < CIV_TOTAL_CARDS) {
            b->active_cards[i] = b->civilization_deck[b->deck_index];
            b->deck_index++;
        }
    }
}

void reset_tools_for_new_round(Player *p) {
    for (int i = 0; i < 3; i++) p->tools_used[i] = 0;
}

void handle_round_end(Board *b, Player *p) {
    int food_needed = p->people_total - p->food_production;
    if (food_needed < 0) food_needed = 0;

    if (p->food_amount >= food_needed) {
        p->food_amount -= food_needed;
        printf(COLOR_GREEN "  Player %d fed people: -%d food (had enough).\n" COLOR_RESET,
               p->player_id + 1, food_needed);
    } else {
        int remaining = food_needed - p->food_amount;
        p->food_amount = 0;

        int conf = 0;
        if (p->is_bot) {
            if (p->bot_kind == BOT_KIND_COLEG) {
                BotColegState *bc = bot_glue_get_active_coleg(p->player_id);
                if (bc) conf = bot_coleg_decide_feeding_resource_use(bc, p, remaining);
            } else {
                BotState *be = bot_glue_get_active_eu(p->player_id);
                if (be) conf = bot_decide_feeding_resource_use(be, p, remaining);
            }
        } else {
            printf(COLOR_YELLOW "  Player %d short of food by %d. Use resources? (1=yes, 0=no): " COLOR_RESET,
                   p->player_id + 1, remaining);
            if (scanf("%d", &conf) != 1) { int c; while ((c=getchar())!='\n'&&c!=EOF){} conf=0; }
        }

        if (conf == 1) {
            if (p->is_bot) {
                for (int r = 0; r < 4 && remaining > 0; r++) {
                    while (p->player_resources[r] > 0 && remaining > 0) {
                        p->player_resources[r]--; b->global_supply[r]++; remaining--;
                    }
                }
            } else {
                for (int r = 0; r < 4 && remaining > 0; r++) {
                    while (p->player_resources[r] > 0 && remaining > 0) {
                        printf("    Use 1 %s as food? (1=yes, 0=no): ",
                               (r==0)?"wood":(r==1)?"brick":(r==2)?"stone":"gold");
                        int u = 0;
                        if (scanf("%d", &u) != 1) { int c; while ((c=getchar())!='\n'&&c!=EOF){} u=0; }
                        if (u == 1) { p->player_resources[r]--; b->global_supply[r]++; remaining--; }
                        else break;
                    }
                }
            }
            if (remaining > 0) {
                p->current_score -= FEED_PENALTY;
                if (p->current_score < 0) p->current_score = 0;
                printf(COLOR_RED "  Still short. -%d VP penalty.\n" COLOR_RESET, FEED_PENALTY);
                char lm[140];
                sprintf(lm, "Player %d could not fully feed - lost %d VP.", p->player_id + 1, FEED_PENALTY);
                log_action(lm);
            } else {
                printf(COLOR_GREEN "  Player %d fed people with resources.\n" COLOR_RESET, p->player_id + 1);
            }
        } else {
            p->current_score -= FEED_PENALTY;
            if (p->current_score < 0) p->current_score = 0;
            printf(COLOR_RED "  Player %d refused to feed: -%d VP.\n" COLOR_RESET,
                   p->player_id + 1, FEED_PENALTY);
            char lm[140];
            sprintf(lm, "Player %d refused feeding - lost %d VP.", p->player_id + 1, FEED_PENALTY);
            log_action(lm);
        }
    }

    reset_tools_for_new_round(p);
}

bool is_game_over(Board *b) {
    for (int i = 0; i < NUM_BUILDING_STACKS; i++) {
        if (b->stack_counters[i] >= BUILDINGS_PER_STACK) return true;
    }
    if (CIV_TOTAL_CARDS - b->deck_index < 4) return true;
    return false;
}

int count_civ_cards_by_type(Player *p, CivScoreType type) {
    int n = 0;
    for (int i = 0; i < p->cards_count; i++) {
        if (p->inventory_cards[i].score_type == type) n++;
    }
    return n;
}

int sum_permanent_tools(Player *p) {
    return p->permanent_tools[0] + p->permanent_tools[1] + p->permanent_tools[2];
}

int count_unique_score_flat_types(Player *p) {
    int seen_ids[CIV_UNIQUE_TYPES];
    int n_seen = 0;
    for (int i = 0; i < p->cards_count; i++) {
        if (p->inventory_cards[i].score_type != SCORE_FLAT) continue;
        int unique_id = p->inventory_cards[i].id % CIV_UNIQUE_TYPES;
        bool already = false;
        for (int j = 0; j < n_seen; j++) {
            if (seen_ids[j] == unique_id) { already = true; break; }
        }
        if (!already) seen_ids[n_seen++] = unique_id;
    }
    return n_seen;
}

int total_resources_owned(Player *p) {
    return p->player_resources[0] + p->player_resources[1]
         + p->player_resources[2] + p->player_resources[3];
}
