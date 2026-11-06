#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "game_types.h"
#include "game_logic.h"
#include "persistence.h"
#include "ui.h"
#include "tests.h"
#include "scoring.h"
#include "bot_eu.h"
#include "bot_coleg.h"
#include "bot_glue.h"

static void flush_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

static Player *get_opponent(Player players[], int num_players, int my_idx) {
    for (int i = 0; i < num_players; i++) {
        if (i != my_idx && !players[i].is_bot) return &players[i];
    }
    return &players[(my_idx + 1) % num_players];
}

static void resolve_round(Board *b, Player players[], int num_players, int start_player) {
    printf("\n" COLOR_BOLD COLOR_CYAN "================ RESOLUTION PHASE ================\n" COLOR_RESET);
    log_action("Resolution phase started.");

    for (int order = 0; order < num_players; order++) {
        int pid = (start_player + order) % num_players;
        printf("\n" COLOR_BOLD COLOR_GREEN "--- Resolving Player %d's workers ---\n" COLOR_RESET, pid + 1);

        for (int slot = 0; slot < 4; slot++) {
            if (players[pid].people_on_location[LOC_CARD_BASE + slot] > 0) {
                evaluate_civ_card_acquisition(&players[pid], b, slot);
            }
        }
        for (int s = 0; s < NUM_BUILDING_STACKS; s++) {
            if (players[pid].people_on_location[LOC_BUILD_BASE + s] > 0) {
                evaluate_building_acquisition(&players[pid], b, s);
            }
        }
        if (players[pid].people_on_location[LOC_TOOL_MAKER] > 0) {
            evaluate_special_locations(&players[pid], b, LOC_TOOL_MAKER);
        }
        if (players[pid].people_on_location[LOC_FIELD] > 0) {
            evaluate_special_locations(&players[pid], b, LOC_FIELD);
        }
        if (players[pid].people_on_location[LOC_HUT] > 0) {
            evaluate_special_locations(&players[pid], b, LOC_HUT);
        }
        for (int loc = LOC_WOOD; loc <= LOC_GOLD; loc++) {
            if (players[pid].people_on_location[loc] > 0) {
                evaluate_resources(&players[pid], b, loc, 0);
            }
        }
        if (players[pid].people_on_location[LOC_HUNTING] > 0) {
            evaluate_resources(&players[pid], b, LOC_HUNTING, 0);
        }
    }

    printf("\n" COLOR_BOLD COLOR_CYAN "================ FEEDING PHASE ================\n" COLOR_RESET);
    log_action("Feeding phase started.");
    for (int p = 0; p < num_players; p++) handle_round_end(b, &players[p]);

    refill_civilization_cards(b);
    for (int i = 0; i < NUM_LOCATIONS; i++) b->location_owner[i] = -1;
    for (int i = 0; i < 3; i++) b->village_used[i] = false;

    printf("\n" COLOR_DIM "Press ENTER to continue to the next round..." COLOR_RESET);
    flush_stdin();
    getchar();
}

static bool round_complete(Board *b, Player players[], int num_players) {
    for (int p = 0; p < num_players; p++) {
        if (players[p].people_available > 0) {
            int has_legal = 0;
            for (int loc = 0; loc < NUM_LOCATIONS && !has_legal; loc++) {
                int max_try = (loc == LOC_HUT) ? 2 : 1;
                if (players[p].people_available < max_try) continue;
                if (is_placement_valid(b, &players[p], loc, max_try)) has_legal = 1;
                if (!has_legal && loc == LOC_HUNTING && players[p].people_available >= 1) {
                    if (is_placement_valid(b, &players[p], LOC_HUNTING, players[p].people_available)) has_legal = 1;
                }
                if (!has_legal && loc >= LOC_WOOD && loc <= LOC_GOLD) {
                    if (is_placement_valid(b, &players[p], loc, 1)) has_legal = 1;
                }
            }
            if (has_legal) return false;
        }
    }
    return true;
}

static void setup_bot_states(Player players[], int num_players,
                              BotState bot_eu_states[],
                              BotColegState bot_coleg_states[]) {
    for (int i = 0; i < num_players; i++) {
        if (!players[i].is_bot) continue;
        if (players[i].bot_kind == BOT_KIND_EU) {
            bot_init(&bot_eu_states[i]);
            bot_glue_set_active_eu(i, &bot_eu_states[i]);
        } else if (players[i].bot_kind == BOT_KIND_COLEG) {
            bot_coleg_init(&bot_coleg_states[i], (ColegPersonality)players[i].bot_coleg_persona);
            bot_glue_set_active_coleg(i, &bot_coleg_states[i]);
        }
    }
}

static void bots_start_of_round(Player players[], int num_players, Board *b, int round_number) {
    for (int i = 0; i < num_players; i++) {
        if (!players[i].is_bot) continue;
        Player *opp = get_opponent(players, num_players, i);
        if (players[i].bot_kind == BOT_KIND_EU) {
            BotState *be = bot_glue_get_active_eu(i);
            if (be) {
                bot_observe_human(be, opp);
                bot_start_of_round(be, b, opp, round_number);
            }
        } else if (players[i].bot_kind == BOT_KIND_COLEG) {
            BotColegState *bc = bot_glue_get_active_coleg(i);
            if (bc) bot_coleg_start_of_round(bc, round_number);
        }
    }
}

static int ask_coleg_persona(const char *who) {
    printf(COLOR_BOLD "\n  BotColeg personality for %s:\n" COLOR_RESET, who);
    printf(COLOR_GREEN "  [1]" COLOR_RESET " Resource Rush\n");
    printf(COLOR_GREEN "  [2]" COLOR_RESET " Card Sniper\n");
    printf(COLOR_GREEN "  [3]" COLOR_RESET " Engine Builder\n");
    printf(COLOR_BOLD "  Choose (1-3): " COLOR_RESET);
    int p = 1;
    if (scanf("%d", &p) != 1) { flush_stdin(); p = 1; }
    flush_stdin();
    if (p < 1 || p > 3) p = 1;
    return p - 1;
}

static int pick_game_mode(int *p0_kind, int *p1_kind,
                           int *p0_persona, int *p1_persona) {
    printf(COLOR_BOLD COLOR_YELLOW
           "\n================================================================\n"
           "                 STONE AGE - SELECT GAME MODE\n"
           "================================================================\n" COLOR_RESET);
    printf(COLOR_GREEN " [1]" COLOR_RESET " Human vs BotEU\n");
    printf(COLOR_GREEN " [2]" COLOR_RESET " Human vs BotColeg\n");
    printf(COLOR_GREEN " [3]" COLOR_RESET " BotEU vs BotColeg\n");
    printf(COLOR_GREEN " [4]" COLOR_RESET " BotColeg vs BotColeg\n");
    printf(COLOR_GREEN " [5]" COLOR_RESET " Human vs Human (classic)\n");
    printf(COLOR_BOLD " Choose mode (1-5): " COLOR_RESET);

    int mode = 5;
    if (scanf("%d", &mode) != 1) { flush_stdin(); mode = 5; }
    flush_stdin();
    if (mode < 1 || mode > 5) mode = 5;

    *p0_persona = COLEG_RESOURCE_RUSH;
    *p1_persona = COLEG_RESOURCE_RUSH;

    switch (mode) {
        case 1:
            *p0_kind = BOT_KIND_NONE;
            *p1_kind = BOT_KIND_EU;
            break;
        case 2:
            *p0_kind = BOT_KIND_NONE;
            *p1_kind = BOT_KIND_COLEG;
            *p1_persona = ask_coleg_persona("Player 2");
            break;
        case 3:
            *p0_kind = BOT_KIND_EU;
            *p1_kind = BOT_KIND_COLEG;
            *p1_persona = ask_coleg_persona("Player 2 (BotColeg)");
            break;
        case 4:
            *p0_kind = BOT_KIND_COLEG;
            *p1_kind = BOT_KIND_COLEG;
            *p0_persona = ask_coleg_persona("Player 1");
            *p1_persona = ask_coleg_persona("Player 2");
            break;
        default:
            *p0_kind = BOT_KIND_NONE;
            *p1_kind = BOT_KIND_NONE;
            break;
    }
    return mode;
}

int main(void) {
    srand((unsigned int)time(NULL));
    init_log_file();

    run_all_tests();

    printf("\n" COLOR_BOLD COLOR_YELLOW "Press ENTER to continue to mode selection..." COLOR_RESET);
    getchar();

    int p0_kind = BOT_KIND_NONE, p1_kind = BOT_KIND_NONE;
    int p0_persona = 0, p1_persona = 0;
    int game_mode = pick_game_mode(&p0_kind, &p1_kind, &p0_persona, &p1_persona);
    (void)game_mode;

    Board b = {0};
    Player players[NUM_PLAYERS];
    initialize_game(&b, players, NUM_PLAYERS);

    int bot_kinds[NUM_PLAYERS]    = {p0_kind,    p1_kind};
    int bot_personas[NUM_PLAYERS] = {p0_persona, p1_persona};
    for (int i = 0; i < NUM_PLAYERS; i++) {
        players[i].bot_kind         = bot_kinds[i];
        players[i].bot_coleg_persona = bot_personas[i];
        players[i].is_bot           = (bot_kinds[i] != BOT_KIND_NONE) ? 1 : 0;
    }

    BotState      bot_eu_states[NUM_PLAYERS];
    BotColegState bot_coleg_states[NUM_PLAYERS];
    memset(bot_eu_states,    0, sizeof(bot_eu_states));
    memset(bot_coleg_states, 0, sizeof(bot_coleg_states));
    setup_bot_states(players, NUM_PLAYERS, bot_eu_states, bot_coleg_states);

    bool running = true;
    char system_msg[256] = "Welcome! Place workers using [p].";
    int current_round = 1;
    int current_turn  = 0;
    char input[16];

    bots_start_of_round(players, NUM_PLAYERS, &b, current_round);

    while (running) {
        if (is_game_over(&b)) {
            clear_screen();
            printf(COLOR_BOLD COLOR_RED "\n!!! GAME OVER !!!\n" COLOR_RESET);
            log_action("Game ended naturally.");
            print_final_scoreboard(players, NUM_PLAYERS);
            log_action("Final scores printed.");
            /* === NOU: pauza inainte de inchidere, ca scoreboard-ul sa ramana vizibil === */
            printf("\n" COLOR_BOLD COLOR_YELLOW "Press ENTER to exit..." COLOR_RESET);
            flush_stdin();
            getchar();
            break;
        }

        if (players[current_turn].is_bot) {
            int out_loc = -1, out_count = 0;
            int placed = 0;
            Player *opp = get_opponent(players, NUM_PLAYERS, current_turn);

            if (players[current_turn].bot_kind == BOT_KIND_EU) {
                BotState *be = bot_glue_get_active_eu(current_turn);
                if (be) placed = bot_choose_action(be, &b, &players[current_turn], opp, &out_loc, &out_count);
            } else if (players[current_turn].bot_kind == BOT_KIND_COLEG) {
                BotColegState *bc = bot_glue_get_active_coleg(current_turn);
                if (bc) placed = bot_coleg_choose_action(bc, &b, &players[current_turn], opp, &out_loc, &out_count);
            }

            if (placed && place_workers(&b, &players[current_turn], out_loc, out_count)) {
                printf(COLOR_DIM "  [Bot P%d] placed %d worker(s) at location %d.\n" COLOR_RESET,
                       current_turn + 1, out_count, out_loc);
                char lm[160];
                sprintf(lm, "Bot P%d placed %d worker(s) at location %d.", current_turn + 1, out_count, out_loc);
                log_action(lm);
                sprintf(system_msg, "Bot P%d placed %d worker(s) at loc %d.", current_turn + 1, out_count, out_loc);
            } else {
                sprintf(system_msg, "Bot P%d passed (no valid action).", current_turn + 1);
            }

            int next = (current_turn + 1) % NUM_PLAYERS;
            if (players[next].people_available > 0) current_turn = next;

            if (round_complete(&b, players, NUM_PLAYERS)) {
                resolve_round(&b, players, NUM_PLAYERS, b.start_player);
                b.start_player = (b.start_player + 1) % NUM_PLAYERS;
                current_turn = b.start_player;
                current_round++;
                bots_start_of_round(players, NUM_PLAYERS, &b, current_round);
                sprintf(system_msg, "Round %d resolved. New round begins!", current_round - 1);
            }

            draw_full_screen(&b, players, NUM_PLAYERS, current_round, current_turn, system_msg);
            continue;
        }

        draw_full_screen(&b, players, NUM_PLAYERS, current_round, current_turn, system_msg);

        if (scanf("%15s", input) != 1) { flush_stdin(); continue; }

        if (strcmp(input, "q") == 0 || strcmp(input, "Q") == 0) {
            printf(COLOR_YELLOW "Quit without final scoring? (1=yes, 0=no): " COLOR_RESET);
            int c = 0; if (scanf("%d", &c) != 1) { flush_stdin(); c = 0; }
            if (c == 1) {
                log_action("Player quit the game.");
                /* === NOU: pauza inainte de inchidere si la iesire prin quit confirmat === */
                printf("\n" COLOR_BOLD COLOR_YELLOW "Press ENTER to exit..." COLOR_RESET);
                flush_stdin();
                getchar();
                running = false;
            } else {
                strcpy(system_msg, "Quit cancelled.");
            }
        }
        else if (strcmp(input, "s") == 0 || strcmp(input, "S") == 0) {
            save_game(&b, players, NUM_PLAYERS, current_round, current_turn);
            strcpy(system_msg, "Success: Game saved.");
        }
        else if (strcmp(input, "l") == 0 || strcmp(input, "L") == 0) {
            if (load_game(&b, players, NUM_PLAYERS, &current_round, &current_turn)) {
                setup_bot_states(players, NUM_PLAYERS, bot_eu_states, bot_coleg_states);
                strcpy(system_msg, "Success: Game loaded.");
            } else {
                strcpy(system_msg, "Error: No valid save file.");
            }
        }
        else if (strcmp(input, "h") == 0 || strcmp(input, "H") == 0) {
            draw_help_screen();
            strcpy(system_msg, "Help viewed.");
        }
        else if (strcmp(input, "i") == 0 || strcmp(input, "I") == 0) {
            clear_screen();
            printf(COLOR_BOLD COLOR_PINK "================ INVENTORY ================\n" COLOR_RESET);
            for (int p = 0; p < NUM_PLAYERS; p++) {
                printf("\n" COLOR_BOLD COLOR_GREEN "  PLAYER %d INVENTORY\n" COLOR_RESET, p + 1);
                draw_inventory(&players[p]);
            }
            printf("\n" COLOR_DIM "Press ENTER to return..." COLOR_RESET);
            flush_stdin(); getchar();
            strcpy(system_msg, "Inventory viewed.");
        }
        else if (strcmp(input, "pass") == 0) {
            log_action("Player passed turn.");
            int next = (current_turn + 1) % NUM_PLAYERS;
            if (players[next].people_available > 0) current_turn = next;
            strcpy(system_msg, "You passed your turn.");
        }
        else if (strcmp(input, "p") == 0 || strcmp(input, "P") == 0) {
            int loc = -1, count = 0;
            printf("  Location ID (0-15): ");
            if (scanf("%d", &loc) != 1) { flush_stdin(); strcpy(system_msg, "Error: Invalid input."); continue; }
            printf("  Number of workers: ");
            if (scanf("%d", &count) != 1) { flush_stdin(); strcpy(system_msg, "Error: Invalid input."); continue; }

            if (place_workers(&b, &players[current_turn], loc, count)) {
                sprintf(system_msg, "Success: %d worker(s) placed at location %d.", count, loc);
                char lm[160];
                sprintf(lm, "Player %d placed %d worker(s) at location %d.",
                        current_turn + 1, count, loc);
                log_action(lm);

                int next = (current_turn + 1) % NUM_PLAYERS;
                if (players[next].people_available > 0) current_turn = next;

                if (round_complete(&b, players, NUM_PLAYERS)) {
                    resolve_round(&b, players, NUM_PLAYERS, b.start_player);
                    b.start_player = (b.start_player + 1) % NUM_PLAYERS;
                    current_turn = b.start_player;
                    current_round++;
                    bots_start_of_round(players, NUM_PLAYERS, &b, current_round);
                    sprintf(system_msg, "Round %d resolved. New round begins!", current_round - 1);
                }
            } else {
                strcpy(system_msg, "Error: Invalid placement - check rules.");
            }
        }
        else {
            strcpy(system_msg, "Error: Unknown command.");
        }
    }

    printf("\n" COLOR_DIM "Thank you for playing Stone Age!\n" COLOR_RESET);
    return 0;
}
