#include <stdio.h>
#include <string.h>
#include "tests.h"
#include "game_types.h"
#include "game_logic.h"
#include "scoring.h"

static int g_tests_passed = 0;
static int g_tests_total  = 0;

static void check(const char *name, int condition) {
    g_tests_total++;
    if (condition) {
        g_tests_passed++;
        printf("  " COLOR_GREEN "[PASS]" COLOR_RESET " %s\n", name);
    } else {
        printf("  " COLOR_RED   "[FAIL]" COLOR_RESET " %s\n", name);
    }
}

static void test_dice_roll(void) {
    int r = roll_dice(3);
    check("Dice roll within [3, 18] for 3 dice", r >= 3 && r <= 18);
    r = roll_dice(7);
    check("Dice roll within [7, 42] for 7 dice", r >= 7 && r <= 42);
}

static void test_init_and_shuffle(void) {
    Board b = {0};
    Player ps[2];
    initialize_game(&b, ps, 2);
    check("Initial active cards filled (slot 0)", b.active_cards[0].id >= 0);
    check("Deck index advanced by 4 after init",  b.deck_index == 4);
    check("Player 1 starts with 5 workers",        ps[0].people_available == 5);
    check("Player 1 starts with 12 food",          ps[0].food_amount == 12);
    check("Global wood supply is 20",              b.global_supply[0] == 20);
    check("No location is owned at start",         b.location_owner[4] == -1);
    check("Village slots not used at start",       !b.village_used[0] && !b.village_used[1] && !b.village_used[2]);
}

static void test_placement_basic(void) {
    Board b = {0};
    Player p = {0};
    initialize_game(&b, &p, 1);

    int ok = place_workers(&b, &p, LOC_WOOD, 2);
    check("Place 2 workers on Forest succeeds", ok == 1);
    check("After placement, 3 workers remain available", p.people_available == 3);
    check("Forest is locked after first placement", b.location_owner[LOC_WOOD] == 0);

    Board b2 = {0};
    Player ps[2];
    initialize_game(&b2, ps, 2);
    place_workers(&b2, &ps[0], LOC_WOOD, 1);
    int ok2 = place_workers(&b2, &ps[1], LOC_WOOD, 1);
    check("Forest blocked for opponent after lock", ok2 == 0);
}

static void test_village_2_of_3_rule(void) {
    Board b = {0};
    Player ps[2];
    initialize_game(&b, ps, 2);

    int a = place_workers(&b, &ps[0], LOC_TOOL_MAKER, 1);
    int bb = place_workers(&b, &ps[1], LOC_FIELD, 1);
    int cc = place_workers(&b, &ps[0], LOC_HUT, 2);
    check("Tool Maker placement succeeds", a == 1);
    check("Field placement succeeds (2nd village slot)", bb == 1);
    check("Hut placement BLOCKED (3rd village slot, rule violation)", cc == 0);
}

static void test_tool_maker_immediate(void) {
    Board b = {0};
    Player p = {0};
    initialize_game(&b, &p, 1);
    int before = p.permanent_tools[0] + p.permanent_tools[1] + p.permanent_tools[2];
    place_workers(&b, &p, LOC_TOOL_MAKER, 1);
    int after = p.permanent_tools[0] + p.permanent_tools[1] + p.permanent_tools[2];
    check("Tool received IMMEDIATELY on placement (not at round end)", after == before + 1);
}

static void test_evaluation_returns_workers(void) {
    Board b = {0};
    Player p = {0};
    initialize_game(&b, &p, 1);
    place_workers(&b, &p, LOC_WOOD, 2);
    int before_avail = p.people_available;
    int loc_occ_before = b.location_occupants[LOC_WOOD];
    p.tools_used[0] = 1; p.tools_used[1] = 1; p.tools_used[2] = 1;
    evaluate_resources(&p, &b, LOC_WOOD, 0);
    check("After eval, workers returned to player", p.people_available == before_avail + 2);
    check("After eval, location cleared",            b.location_occupants[LOC_WOOD] == loc_occ_before - 2);
    check("After eval, player worker count on loc 0", p.people_on_location[LOC_WOOD] == 0);
}

static void test_scoring(void) {
    Player p = {0};
    p.player_id = 0;
    p.people_total = 7;
    p.food_production = 5;
    p.permanent_tools[0] = 2;
    p.permanent_tools[1] = 1;
    p.player_resources[0] = 3;
    p.current_score = 10;

    p.inventory_cards[p.cards_count++] = CIV_DECK_TEMPLATE[0];
    p.inventory_cards[p.cards_count++] = CIV_DECK_TEMPLATE[1];
    p.inventory_cards[p.cards_count++] = CIV_DECK_TEMPLATE[6];
    p.inventory_cards[p.cards_count++] = CIV_DECK_TEMPLATE[8];

    BuildingCard bd = {0, "Test", 0, 0, 0, 0, 8};
    p.inventory_buildings[p.buildings_count++] = bd;

    ScoreBreakdown s = calculate_final_score(&p);
    check("Score: 2 unique SCORE_FLAT (Pottery + Weaving) = 4", s.from_score_flat_squared == 4);
    check("Score: 1 PER_TOOL card * 3 total tools = 3",         s.from_per_tool == 3);
    check("Score: 1 PER_HUT card * 7 people = 7",               s.from_per_hut == 7);
    check("Score: 3 leftover resources = 3 VP",                 s.from_resources == 3);
    check("Score: current_score (incl. building VP) = 10",      s.current_score == 10);
    int expected_total = 10 + 4 + 3 + 7 + 0 + 0 + 0 + 3;
    check("Score: total computed correctly",                    s.total == expected_total);
}

static void test_game_over_detection(void) {
    Board b = {0};
    Player p = {0};
    initialize_game(&b, &p, 1);
    check("Game not over at start", !is_game_over(&b));
    b.stack_counters[0] = BUILDINGS_PER_STACK;
    check("Game OVER when stack exhausted", is_game_over(&b));

    Board b2 = {0};
    initialize_game(&b2, &p, 1);
    b2.deck_index = CIV_TOTAL_CARDS - 3;
    check("Game OVER when < 4 cards remain in deck", is_game_over(&b2));
}

void run_all_tests(void) {
    printf("\n" COLOR_BOLD COLOR_CYAN "================ RUNNING UNIT TESTS ================\n" COLOR_RESET);
    test_dice_roll();
    test_init_and_shuffle();
    test_placement_basic();
    test_village_2_of_3_rule();
    test_tool_maker_immediate();
    test_evaluation_returns_workers();
    test_scoring();
    test_game_over_detection();
    printf(COLOR_BOLD COLOR_CYAN  "==================== SUMMARY ====================\n" COLOR_RESET);
    printf("  " COLOR_BOLD "Passed:" COLOR_RESET " " COLOR_GREEN "%d" COLOR_RESET " / %d\n", g_tests_passed, g_tests_total);
    printf(COLOR_CYAN "==================================================\n" COLOR_RESET);
}
