#ifndef SCORING_H
#define SCORING_H

#include "game_types.h"

typedef struct {
    int from_buildings;
    int from_score_flat_squared;
    int from_per_tool;
    int from_per_hut;
    int from_per_card;
    int from_per_farm;
    int from_multiply;
    int from_resources;
    int current_score;
    int total;
} ScoreBreakdown;

ScoreBreakdown calculate_final_score(Player *p);
int  determine_winner(Player players[], int num_players, ScoreBreakdown breakdowns[]);
void print_final_scoreboard(Player players[], int num_players);

#endif
