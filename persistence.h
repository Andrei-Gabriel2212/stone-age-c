#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "game_types.h"

void log_action(const char *message);
void init_log_file(void);
void save_game(Board *b, Player players[], int num_players, int current_round, int current_turn);
bool load_game(Board *b, Player players[], int num_players, int *current_round, int *current_turn);

#endif
