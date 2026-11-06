#include <stdio.h>
#include "scoring.h"
#include "game_logic.h"

ScoreBreakdown calculate_final_score(Player *p) {
    ScoreBreakdown sb = {0};

    sb.current_score = p->current_score;
    sb.from_buildings = 0;
    for (int i = 0; i < p->buildings_count; i++) {
        sb.from_buildings += p->inventory_buildings[i].victory_points;
    }

    int n_unique_flat = count_unique_score_flat_types(p);
    sb.from_score_flat_squared = n_unique_flat * n_unique_flat;

    int n_per_tool = count_civ_cards_by_type(p, SCORE_PER_TOOL);
    int n_per_hut  = count_civ_cards_by_type(p, SCORE_PER_HUT);
    int n_per_card = count_civ_cards_by_type(p, SCORE_PER_CARD);
    int n_per_farm = count_civ_cards_by_type(p, SCORE_PER_FARM);

    sb.from_per_tool = n_per_tool * sum_permanent_tools(p);
    sb.from_per_hut  = n_per_hut  * p->people_total;
    sb.from_per_card = n_per_card * p->cards_count;
    sb.from_per_farm = n_per_farm * p->food_production;

    sb.from_multiply = 0;
    for (int i = 0; i < p->cards_count; i++) {
        if (p->inventory_cards[i].score_type == SCORE_MULTIPLY) {
            sb.from_multiply += p->inventory_cards[i].bonus_food * p->food_production;
        }
    }

    sb.from_resources = total_resources_owned(p);

    sb.total = sb.current_score
             + sb.from_score_flat_squared
             + sb.from_per_tool
             + sb.from_per_hut
             + sb.from_per_card
             + sb.from_per_farm
             + sb.from_multiply
             + sb.from_resources;
    return sb;
}

int determine_winner(Player players[], int num_players, ScoreBreakdown breakdowns[]) {
    int best = 0;
    for (int i = 1; i < num_players; i++) {
        if (breakdowns[i].total > breakdowns[best].total) best = i;
    }

    int tied_count = 0;
    int tied[8];
    for (int i = 0; i < num_players; i++) {
        if (breakdowns[i].total == breakdowns[best].total) tied[tied_count++] = i;
    }
    if (tied_count == 1) return best;

    int tb_best = tied[0];
    int tb_best_val = players[tied[0]].food_production
                    + sum_permanent_tools(&players[tied[0]])
                    + players[tied[0]].people_total;
    for (int k = 1; k < tied_count; k++) {
        int pid = tied[k];
        int v = players[pid].food_production
              + sum_permanent_tools(&players[pid])
              + players[pid].people_total;
        if (v > tb_best_val) { tb_best_val = v; tb_best = pid; }
    }
    return tb_best;
}

void print_final_scoreboard(Player players[], int num_players) {
    printf("\n");
    printf(COLOR_CYAN  "================================================================\n" COLOR_RESET);
    printf(COLOR_BOLD  "                        FINAL SCORING\n" COLOR_RESET);
    printf(COLOR_CYAN  "================================================================\n" COLOR_RESET);

    ScoreBreakdown sbs[8];
    for (int i = 0; i < num_players; i++) sbs[i] = calculate_final_score(&players[i]);

    for (int i = 0; i < num_players; i++) {
        ScoreBreakdown s = sbs[i];
        printf("\n" COLOR_GREEN "  ===== Player %d =====\n" COLOR_RESET, i + 1);
        printf("    In-game VP (buildings + bonuses)    : " COLOR_YELLOW "%4d\n" COLOR_RESET, s.current_score);
        printf("    Unique SCORE_FLAT cards (n*n)        : " COLOR_YELLOW "%4d\n" COLOR_RESET, s.from_score_flat_squared);
        printf("    PER_TOOL  cards * total tool value   : " COLOR_YELLOW "%4d\n" COLOR_RESET, s.from_per_tool);
        printf("    PER_HUT   cards * total people       : " COLOR_YELLOW "%4d\n" COLOR_RESET, s.from_per_hut);
        printf("    PER_CARD  cards * card count         : " COLOR_YELLOW "%4d\n" COLOR_RESET, s.from_per_card);
        printf("    PER_FARM  cards * food production    : " COLOR_YELLOW "%4d\n" COLOR_RESET, s.from_per_farm);
        printf("    MULTIPLY  cards (bonus * food prod)  : " COLOR_YELLOW "%4d\n" COLOR_RESET, s.from_multiply);
        printf("    Leftover resources (1 VP each)       : " COLOR_YELLOW "%4d\n" COLOR_RESET, s.from_resources);
        printf("    " COLOR_BOLD "TOTAL                                : " COLOR_GREEN "%4d\n" COLOR_RESET, s.total);
        printf("    " COLOR_DIM "(tiebreak stats: tools=%d, people=%d, food_prod=%d)\n" COLOR_RESET,
               sum_permanent_tools(&players[i]),
               players[i].people_total,
               players[i].food_production);
    }

    int winner = determine_winner(players, num_players, sbs);
    printf("\n" COLOR_MAGENTA "================================================================\n" COLOR_RESET);
    printf(COLOR_BOLD COLOR_GREEN "  >>> WINNER: Player %d with %d VP <<<\n" COLOR_RESET,
           winner + 1, sbs[winner].total);
    printf(COLOR_MAGENTA "================================================================\n" COLOR_RESET);
}
