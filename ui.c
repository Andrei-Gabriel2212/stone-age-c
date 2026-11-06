#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"

typedef struct {
    const char *immediate_text;
    const char *endgame_text;
} CivCardDesc;

static const CivCardDesc CARD_DESCRIPTIONS[18] = {
    { "+3 food",     "+3 VP per unique culture type (squared)" },
    { "-",           "+3 VP per unique culture type (squared)" },
    { "+1 wood",     "+3 VP per unique culture type (squared)" },
    { "+1 brick",    "+3 VP per unique culture type (squared)" },
    { "+1 stone",    "+3 VP per unique culture type (squared)" },
    { "+1 gold",     "+3 VP per unique culture type (squared)" },
    { "-",           "+1 VP per tool point owned"              },
    { "-",           "+1 VP per tool point owned"              },
    { "-",           "+1 VP per total person"                  },
    { "-",           "+1 VP per total person"                  },
    { "-",           "+1 VP per civilization card"             },
    { "-",           "+1 VP per civilization card"             },
    { "-",           "+1 VP per food production level"         },
    { "+3 food",     "+1 VP per food production level"         },
    { "+4 food",     "+4 * food production VP"                 },
    { "+3 food",     "+3 * food production VP"                 },
    { "+1 new tool", "+2 VP (flat)"                            },
    { "+1 new tool", "+2 VP (flat)"                            },
};

static void building_cost_str(const BuildingCard *bd, char *buf, int buf_len) {
    if (bd->type == BUILD_2KIND_4RES) {
        snprintf(buf, buf_len, "SPECIAL: 4 res from 2 kinds");
        return;
    }
    if (bd->type == BUILD_ANY_1TO7) {
        snprintf(buf, buf_len, "SPECIAL: 1-7 any resources");
        return;
    }
    const int   costs[4] = { bd->cost_wood, bd->cost_brick, bd->cost_stone, bd->cost_gold };
    const char *names[4] = { "wood", "brick", "stone", "gold" };
    buf[0] = '\0';
    int added = 0;
    for (int r = 0; r < 4; r++) {
        if (costs[r] > 0) {
            if (added > 0) strncat(buf, " + ", (size_t)(buf_len - (int)strlen(buf) - 1));
            char part[32];
            snprintf(part, sizeof(part), "%d %s", costs[r], names[r]);
            strncat(buf, part, (size_t)(buf_len - (int)strlen(buf) - 1));
            added++;
        }
    }
    if (added == 0) strncat(buf, "free", (size_t)(buf_len - (int)strlen(buf) - 1));
}

void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    printf("\x1b[2J\x1b[H");
#endif
}

void draw_header(int current_round, int current_turn) {
    printf(COLOR_CYAN "================================================================\n" COLOR_RESET);
    printf(COLOR_BOLD COLOR_YELLOW "                STONE AGE - 2 PLAYER EDITION\n" COLOR_RESET);
    printf(COLOR_CYAN "================================================================\n" COLOR_RESET);
    printf(" " COLOR_BOLD "Round: " COLOR_YELLOW "%-3d" COLOR_RESET, current_round);
    printf("   " COLOR_BOLD "Phase: " COLOR_MAGENTA "Worker Placement" COLOR_RESET);
    printf("   " COLOR_BOLD "Turn: " COLOR_GREEN "PLAYER %d" COLOR_RESET "\n", current_turn + 1);
    printf(COLOR_CYAN "----------------------------------------------------------------\n" COLOR_RESET);
}

void draw_board(Board *b) {
    printf(COLOR_BOLD COLOR_CYAN "\n  === VILLAGE & HUNTING ===\n" COLOR_RESET);

    int used_count = 0;
    for (int i = 0; i < 3; i++) if (b->village_used[i]) used_count++;
    int slots_left = 2 - used_count;
    if (slots_left < 0) slots_left = 0;

    printf(COLOR_DIM "    (Village rule: only 2 of {Tool Maker, Field, Hut} per round - %d slot(s) left)\n" COLOR_RESET, slots_left);

    printf("    " COLOR_CYAN  "[0] Hunting:    " COLOR_RESET " %d people " COLOR_DIM "(no limit, once per player)\n" COLOR_RESET, b->location_occupants[0]);

    const char *vstat[3];
    for (int i = 0; i < 3; i++) vstat[i] = b->village_used[i] ? COLOR_RED "USED  " COLOR_RESET : COLOR_GREEN "FREE  " COLOR_RESET;

    printf("    " COLOR_CYAN  "[1] Tool Maker: " COLOR_RESET " %d %s " COLOR_DIM "(1 worker)\n" COLOR_RESET, b->location_occupants[1], vstat[0]);
    printf("    " COLOR_CYAN  "[2] Field:      " COLOR_RESET " %d %s " COLOR_DIM "(1 worker)\n" COLOR_RESET, b->location_occupants[2], vstat[1]);
    printf("    " COLOR_CYAN  "[3] Hut:        " COLOR_RESET " %d %s " COLOR_DIM "(2 workers)\n" COLOR_RESET, b->location_occupants[3], vstat[2]);

    printf(COLOR_BOLD COLOR_YELLOW "\n  === RESOURCE LOCATIONS (locked once a player places) ===\n" COLOR_RESET);
    const char *res_names[]  = {"Forest (Wood) ", "Clay Pit (Brick)", "Quarry (Stone)", "River (Gold)"};
    const char *res_colors[] = {COLOR_GREEN, COLOR_ORANGE, COLOR_GRAY, COLOR_YELLOW};
    for (int r = 0; r < 4; r++) {
        int loc = 4 + r;
        printf("    %s[%d] %-17s" COLOR_RESET, res_colors[r], loc, res_names[r]);
        printf(" %d / %d ", b->location_occupants[loc], RESOURCE_SPACE_LIMIT);
        if (b->location_owner[loc] == -1) {
            printf(COLOR_GREEN "FREE  " COLOR_RESET);
        } else {
            printf(COLOR_RED "LOCKED by P%d  " COLOR_RESET, b->location_owner[loc] + 1);
        }
        printf(COLOR_DIM "| Supply: %d\n" COLOR_RESET, b->global_supply[r]);
    }
}

void draw_cards_and_buildings(Board *b) {
    printf(COLOR_BOLD COLOR_BLUE "\n  === CIVILIZATION CARDS (locations 8-11) ===\n" COLOR_RESET);
    for (int i = 0; i < 4; i++) {
        printf("    " COLOR_BLUE "[%d]" COLOR_RESET " ", 8 + i);
        if (b->active_cards[i].id >= 0) {
            printf(COLOR_PINK "%s" COLOR_RESET " (cost: %d res)", b->active_cards[i].name, i);
            if (b->location_occupants[8 + i] > 0)
                printf(COLOR_YELLOW " <- P%d occupied" COLOR_RESET, b->location_owner[8+i] + 1);
            printf("\n");
            int desc_id = b->active_cards[i].id % 18;
            printf("        " COLOR_GREEN "Immediate: " COLOR_RESET "%s\n",
                   CARD_DESCRIPTIONS[desc_id].immediate_text);
            printf("        " COLOR_CYAN  "End-game:  " COLOR_RESET "%s\n",
                   CARD_DESCRIPTIONS[desc_id].endgame_text);
        } else {
            printf(COLOR_DIM "EMPTY\n" COLOR_RESET);
        }
    }

    printf(COLOR_BOLD COLOR_MAGENTA "\n  === BUILDING STACKS (locations 12-15) ===\n" COLOR_RESET);
    for (int s = 0; s < 4; s++) {
        printf("    " COLOR_MAGENTA "[%d]" COLOR_RESET " ", 12 + s);
        if (b->stack_counters[s] < BUILDINGS_PER_STACK) {
            BuildingCard bd = b->building_stacks[s][b->stack_counters[s]];
            char cost_buf[80];
            building_cost_str(&bd, cost_buf, sizeof(cost_buf));
            if (bd.type == BUILD_FIXED) {
                printf(COLOR_PINK "%-14s" COLOR_RESET " (cost: %s) -> " COLOR_GREEN "%d VP" COLOR_RESET,
                       bd.name, cost_buf, bd.victory_points);
            } else {
                printf(COLOR_PINK "%-14s" COLOR_RESET " (%s) -> " COLOR_GREEN "VP = sum of values" COLOR_RESET,
                       bd.name, cost_buf);
            }
            if (b->location_occupants[12 + s] > 0)
                printf(COLOR_YELLOW " <- P%d occupied" COLOR_RESET, b->location_owner[12+s] + 1);
            printf("\n");
            printf("        " COLOR_DIM "Remaining in stack: %d\n" COLOR_RESET,
                   BUILDINGS_PER_STACK - b->stack_counters[s]);
        } else {
            printf(COLOR_DIM "EMPTY (stack exhausted)\n" COLOR_RESET);
        }
    }
}

void draw_player_info(Player *p, int is_active) {
    const char *frame = is_active ? COLOR_BOLD COLOR_GREEN : COLOR_DIM;
    char bot_suffix[40] = "";
    if (p->is_bot) {
        if (p->bot_kind == BOT_KIND_EU) {
            snprintf(bot_suffix, sizeof(bot_suffix), " [BotEU]");
        } else if (p->bot_kind == BOT_KIND_COLEG) {
            const char *pnames[] = {"ResourceRush", "CardSniper", "EngineBuilder"};
            int pidx = p->bot_coleg_persona;
            if (pidx < 0 || pidx > 2) pidx = 0;
            snprintf(bot_suffix, sizeof(bot_suffix), " [BotColeg-%s]", pnames[pidx]);
        }
    }
    printf("\n%s  +-------------------------------------------------+\n" COLOR_RESET, frame);
    printf("%s  | PLAYER %d%s %s                         |\n" COLOR_RESET,
           frame, p->player_id + 1, bot_suffix, is_active ? "(ACTIVE)" : "        ");
    printf("%s  +-------------------------------------------------+\n" COLOR_RESET, frame);

    printf("    " COLOR_YELLOW "Score:" COLOR_RESET " %-3d  ", p->current_score);
    printf(COLOR_GREEN  "Food:" COLOR_RESET " %-2d  ", p->food_amount);
    printf(COLOR_BLUE   "FoodProd:" COLOR_RESET " %-2d  ", p->food_production);
    printf(COLOR_CYAN   "Workers:" COLOR_RESET " %d/%d\n", p->people_available, p->people_total);

    printf("    " COLOR_BROWN  "Wood:%d" COLOR_RESET "  ", p->player_resources[0]);
    printf(COLOR_ORANGE "Brick:%d" COLOR_RESET "  ", p->player_resources[1]);
    printf(COLOR_GRAY   "Stone:%d" COLOR_RESET "  ", p->player_resources[2]);
    printf(COLOR_YELLOW "Gold:%d" COLOR_RESET "\n", p->player_resources[3]);

    printf("    " COLOR_MAGENTA "Tools:" COLOR_RESET);
    for (int i = 0; i < 3; i++) {
        if (p->permanent_tools[i] == 0) {
            printf(COLOR_DIM " [--]" COLOR_RESET);
        } else if (p->tools_used[i]) {
            printf(COLOR_RED " [%d-USED]" COLOR_RESET, p->permanent_tools[i]);
        } else {
            printf(COLOR_GREEN " [+%d]" COLOR_RESET, p->permanent_tools[i]);
        }
    }
    printf("   " COLOR_PINK "Cards:" COLOR_RESET " %d   " COLOR_PINK "Buildings:" COLOR_RESET " %d\n",
           p->cards_count, p->buildings_count);
}

void draw_inventory(Player *p) {
    if (p->cards_count == 0 && p->buildings_count == 0) {
        printf(COLOR_DIM "    (no civ cards or buildings owned yet)\n" COLOR_RESET);
        return;
    }
    if (p->cards_count > 0) {
        printf("    " COLOR_PINK "Civ cards:" COLOR_RESET);
        for (int i = 0; i < p->cards_count; i++) printf(" %s,", p->inventory_cards[i].name);
        printf("\n");
    }
    if (p->buildings_count > 0) {
        printf("    " COLOR_PINK "Buildings:" COLOR_RESET);
        for (int i = 0; i < p->buildings_count; i++) printf(" %s(%dVP),", p->inventory_buildings[i].name, p->inventory_buildings[i].victory_points);
        printf("\n");
    }
}

void draw_command_bar(const char *system_msg) {
    printf("\n" COLOR_CYAN "----------------------------------------------------------------\n" COLOR_RESET);
    int is_err = (strstr(system_msg, "Error") != NULL || strstr(system_msg, "Invalid") != NULL);
    printf(" " COLOR_BOLD "System: " COLOR_RESET);
    printf("%s%s" COLOR_RESET "\n", is_err ? COLOR_RED : COLOR_GREEN, system_msg);
    printf(COLOR_CYAN "----------------------------------------------------------------\n" COLOR_RESET);

    printf(" " COLOR_BOLD "Commands:" COLOR_RESET);
    printf(" " COLOR_GREEN "[p]" COLOR_RESET " Place workers");
    printf(" | " COLOR_YELLOW "[i]" COLOR_RESET " Inventory");
    printf(" | " COLOR_CYAN  "[h]" COLOR_RESET " Help");
    printf(" | " COLOR_CYAN  "[pass]" COLOR_RESET " Skip");
    printf(" | " COLOR_BLUE  "[s]" COLOR_RESET " Save");
    printf(" | " COLOR_BLUE  "[l]" COLOR_RESET " Load");
    printf(" | " COLOR_RED   "[q]" COLOR_RESET " Quit\n");
    printf(" " COLOR_BOLD "Your choice: " COLOR_RESET);
}

void draw_help_screen(void) {
    clear_screen();

    printf(COLOR_CYAN "================================================================\n" COLOR_RESET);
    printf(COLOR_BOLD COLOR_YELLOW "                    STONE AGE - HELP & RULES\n" COLOR_RESET);
    printf(COLOR_CYAN "================================================================\n\n" COLOR_RESET);

    printf(COLOR_BOLD COLOR_CYAN "== WORKER PLACEMENT ==\n\n" COLOR_RESET);
    printf("Each round, players alternate placing workers on board locations.\n");
    printf("You CANNOT place workers on a location where you already have workers\n");
    printf("this round. Some locations have restrictions:\n\n");

    printf(COLOR_GREEN "[0] HUNTING GROUND\n" COLOR_RESET);
    printf("    Place ANY number of workers (once per player per round).\n");
    printf("    Roll dice, divide sum by 2 -> food earned.\n\n");

    printf(COLOR_GREEN "[1] TOOL MAKER" COLOR_RESET " (village slot)\n");
    printf("    Place exactly 1 worker.\n");
    printf("    Gain 1 tool IMMEDIATELY (added to lowest-value tool slot, max 4).\n");
    printf("    Tools can be used to boost dice rolls (one use per round per tool).\n\n");

    printf(COLOR_GREEN "[2] FIELD" COLOR_RESET " (village slot)\n");
    printf("    Place exactly 1 worker.\n");
    printf("    Gain +1 permanent food production (reduces feeding cost).\n\n");

    printf(COLOR_GREEN "[3] HUT" COLOR_RESET " (village slot)\n");
    printf("    Place exactly 2 workers.\n");
    printf("    Gain +1 permanent person (more workers next round).\n\n");

    printf(COLOR_BOLD COLOR_MAGENTA "VILLAGE RULE:" COLOR_RESET
           " Only 2 of the 3 village slots [1][2][3] can be used\n");
    printf("per round. The third must remain empty.\n\n");

    printf(COLOR_GREEN "[4] FOREST     " COLOR_RESET "- up to 7 workers, /3 = wood\n");
    printf(COLOR_GREEN "[5] CLAY PIT   " COLOR_RESET "- up to 7 workers, /4 = brick\n");
    printf(COLOR_GREEN "[6] QUARRY     " COLOR_RESET "- up to 7 workers, /5 = stone\n");
    printf(COLOR_GREEN "[7] RIVER      " COLOR_RESET "- up to 7 workers, /6 = gold\n\n");

    printf(COLOR_BOLD COLOR_MAGENTA "RESOURCE RULE:" COLOR_RESET
           " Once a player places ANY workers on a resource\n");
    printf("location, that location is LOCKED to all players for the rest\n");
    printf("of the round.\n\n");

    printf(COLOR_GREEN "[8-11] CIVILIZATION CARDS\n" COLOR_RESET);
    printf("    Place 1 worker per card.\n");
    printf("    Cost to acquire: slot index (0, 1, 2, or 3 resources of your choice).\n");
    printf("    You receive an immediate bonus + endgame scoring (see below).\n\n");

    printf(COLOR_GREEN "[12-15] BUILDING STACKS\n" COLOR_RESET);
    printf("    Place 1 worker per stack.\n");
    printf("    Pay listed resources to acquire the top building.\n");
    printf("    Receive Victory Points IMMEDIATELY.\n\n");

    printf(COLOR_CYAN "================================================================\n\n" COLOR_RESET);
    printf(COLOR_BOLD COLOR_CYAN "== CIVILIZATION CARD SCORING ==\n\n" COLOR_RESET);
    printf("There are 6 scoring types. At end of game, you score based on what\n");
    printf("you collected:\n\n");

    printf(COLOR_GREEN "[SCORE_FLAT]" COLOR_RESET " \"Culture cards\"\n");
    printf("   Cards: Pottery, Weaving, Scratching, Transport, Gemstone, Time,\n");
    printf("          Tool Making, Bookkeeping\n");
    printf("   Formula: count UNIQUE types you own, score (n * n) * bonus_score\n");
    printf("   Example: 4 unique cards x 3 bonus = 36 VP\n");
    printf("   Strategy: Collect DIVERSE cards, not duplicates.\n\n");

    printf(COLOR_GREEN "[SCORE_PER_TOOL]" COLOR_RESET " \"Tool cards\"\n");
    printf("   Cards: Claywork, Cartography\n");
    printf("   Formula: 1 VP per tool point you own at end of game\n");
    printf("   Example: 2 cards, 6 tool points = 12 VP\n");
    printf("   Strategy: Pair with Tool Maker investment.\n\n");

    printf(COLOR_GREEN "[SCORE_PER_HUT]" COLOR_RESET " \"People cards\"\n");
    printf("   Cards: Shaman, Tribal Art\n");
    printf("   Formula: 1 VP per total person at end of game\n");
    printf("   Example: 1 card, 8 people = 8 VP\n");
    printf("   Strategy: Pair with Hut investment.\n\n");

    printf(COLOR_GREEN "[SCORE_PER_CARD]" COLOR_RESET " \"Card cards\"\n");
    printf("   Cards: Healing, Music\n");
    printf("   Formula: 1 VP per civilization card you own\n");
    printf("   Example: 1 card, 10 cards total = 10 VP\n");
    printf("   Strategy: Self-amplifying - buy many cards.\n\n");

    printf(COLOR_GREEN "[SCORE_PER_FARM]" COLOR_RESET " \"Farm cards\"\n");
    printf("   Cards: Writing, Harvest\n");
    printf("   Formula: 1 VP per food production level\n");
    printf("   Example: 1 card, 5 food production = 5 VP\n");
    printf("   Strategy: Pair with Field investment.\n\n");

    printf(COLOR_GREEN "[SCORE_MULTIPLY]" COLOR_RESET " \"Multiplier cards\"\n");
    printf("   Cards: Irrigation (x4), Farmer (x3)\n");
    printf("   Formula: bonus * food production VP\n");
    printf("   Example: Irrigation card, 5 food production = 20 VP\n");
    printf("   Strategy: Excellent late-game with high food production.\n\n");

    printf(COLOR_CYAN "================================================================\n\n" COLOR_RESET);
    printf(COLOR_BOLD COLOR_CYAN "== BUILDING TYPES ==\n\n" COLOR_RESET);

    printf(COLOR_GREEN "[BUILD_FIXED]" COLOR_RESET " Standard buildings\n");
    printf("   Pay the exact resources shown. Receive listed VP.\n");
    printf("   Most buildings are this type.\n\n");

    printf(COLOR_GREEN "[BUILD_2KIND_4RES]" COLOR_RESET " Special: 4 resources from 2 kinds\n");
    printf("   You must pay EXACTLY 4 resources from EXACTLY 2 different kinds.\n");
    printf("   VP = sum of resource values paid.\n");
    printf("   Example: 3 stone + 1 wood = 3*5 + 1*3 = 18 VP\n\n");

    printf(COLOR_GREEN "[BUILD_ANY_1TO7]" COLOR_RESET " Special: 1-7 any resources\n");
    printf("   You can pay between 1 and 7 resources, any kind, any combination.\n");
    printf("   VP = sum of resource values paid.\n\n");

    printf("RESOURCE VALUES (for VP calculation):\n");
    printf("   Wood  = 3 VP\n");
    printf("   Brick = 4 VP\n");
    printf("   Stone = 5 VP\n");
    printf("   Gold  = 6 VP\n\n");

    printf(COLOR_CYAN "================================================================\n\n" COLOR_RESET);
    printf(COLOR_BOLD COLOR_CYAN "== FEEDING PHASE (end of each round) ==\n\n" COLOR_RESET);
    printf("Each person needs 1 food. Food production reduces this need:\n");
    printf("   food needed = (total people) - (food production)\n\n");
    printf("If you don't have enough food:\n");
    printf("   - Optionally pay with resources (1 resource = 1 food)\n");
    printf("   - Otherwise: -10 VP penalty\n\n");

    printf(COLOR_CYAN "================================================================\n\n" COLOR_RESET);
    printf(COLOR_BOLD COLOR_CYAN "== FINAL SCORING ==\n\n" COLOR_RESET);
    printf("Game ends when:\n");
    printf("   - A building stack is exhausted (current round finishes), OR\n");
    printf("   - Less than 4 cards remain in the civilization deck\n\n");
    printf("Final score = sum of:\n");
    printf("   + Current in-game score (buildings + game effects)\n");
    printf("   + SCORE_FLAT: (unique types)^2 * bonus per card\n");
    printf("   + SCORE_PER_TOOL: count * total tool value\n");
    printf("   + SCORE_PER_HUT:  count * total people\n");
    printf("   + SCORE_PER_CARD: count * civ card count\n");
    printf("   + SCORE_PER_FARM: count * food production\n");
    printf("   + SCORE_MULTIPLY: bonus * food production\n");
    printf("   + 1 VP per leftover resource\n\n");
    printf(COLOR_BOLD COLOR_MAGENTA "TIEBREAKER:" COLOR_RESET
           " If tied on total score, the player with highest\n");
    printf("   (food production + total tools + total people) wins.\n\n");

    printf(COLOR_CYAN "================================================================\n\n" COLOR_RESET);
    printf(COLOR_BOLD COLOR_CYAN "== COMMANDS ==\n\n" COLOR_RESET);
    printf(COLOR_GREEN "[p]" COLOR_RESET "    Place workers (asks for location ID and count)\n");
    printf(COLOR_GREEN "[i]" COLOR_RESET "    Show inventory (cards + buildings of all players)\n");
    printf(COLOR_GREEN "[h]" COLOR_RESET "    Show this help screen\n");
    printf(COLOR_GREEN "[pass]" COLOR_RESET " Skip turn\n");
    printf(COLOR_GREEN "[s]" COLOR_RESET "    Save game to savegame.dat\n");
    printf(COLOR_GREEN "[l]" COLOR_RESET "    Load game from savegame.dat\n");
    printf(COLOR_GREEN "[q]" COLOR_RESET "    Quit (asks for confirmation)\n\n");

    printf(COLOR_CYAN "================================================================\n" COLOR_RESET);
    printf(COLOR_DIM "        Press ENTER to return to the game...\n" COLOR_RESET);
    printf(COLOR_CYAN "================================================================\n" COLOR_RESET);

    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
    getchar();
}

void draw_full_screen(Board *b, Player players[], int num_players,
                      int current_round, int current_turn, const char *system_msg) {
    clear_screen();
    draw_header(current_round, current_turn);
    draw_board(b);
    draw_cards_and_buildings(b);
    for (int i = 0; i < num_players; i++) {
        draw_player_info(&players[i], i == current_turn);
    }
    draw_command_bar(system_msg);
}
