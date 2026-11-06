#ifndef UI_H
#define UI_H

#include "game_types.h"

void clear_screen(void);
void draw_header(int current_round, int current_turn);
void draw_board(Board *b);
void draw_player_info(Player *p, int is_active);
void draw_cards_and_buildings(Board *b);
void draw_inventory(Player *p);
void draw_command_bar(const char *system_msg);
void draw_help_screen(void);
void draw_full_screen(Board *b, Player players[], int num_players,
                      int current_round, int current_turn, const char *system_msg);

#endif
