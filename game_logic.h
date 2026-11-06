#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "game_types.h"

int  roll_dice(int people_count);
void shuffle_all(Board *b);
void initialize_game(Board *b, Player players[], int num_players);

int  is_placement_valid(Board *b, Player *p, int location_id, int count);
int  place_workers(Board *b, Player *p, int location_id, int count);

int  apply_tools_interactive(Player *p, int base_sum, const char *context);
int  apply_tools_auto(Player *p, int base_sum);

void evaluate_resources(Player *p, Board *b, int location_id, int extra_dice_bonus);
void evaluate_special_locations(Player *p, Board *b, int location_id);
int  evaluate_civ_card_acquisition(Player *p, Board *b, int slot_index);
int  evaluate_building_acquisition(Player *p, Board *b, int stack_index);

void handle_round_end(Board *b, Player *p);
void refill_civilization_cards(Board *b);
void reset_tools_for_new_round(Player *p);

bool is_game_over(Board *b);

int  count_civ_cards_by_type(Player *p, CivScoreType type);
int  sum_permanent_tools(Player *p);
int  count_unique_score_flat_types(Player *p);
int  total_resources_owned(Player *p);

#endif
