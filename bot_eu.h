#ifndef BOT_EU_H
#define BOT_EU_H

#include "game_types.h"

typedef enum {
    STRAT_UNDECIDED      = 0,
    STRAT_AGGRO_BUILDER  = 1,
    STRAT_CARD_RUSHER    = 2,
    STRAT_HOARDER        = 3
} BotStrategy;

typedef enum {
    MODE_NORMAL    = 0,
    MODE_SABOTAGE  = 1,
    MODE_RECOVERY  = 2
} BotMode;

typedef struct {
    BotStrategy strategy;
    BotMode     mode;
    int         current_round;
    int         sabotage_rounds_left;
    int         recovery_rounds_left;
    int         human_blocked_me_streak;

    int human_loc_count[NUM_LOCATIONS];
    int human_total_resources_seen;
    int human_total_tools_seen;
    int human_total_people_seen;
    int human_total_food_prod_seen;
    int human_cards_per_type[6];
    int human_buildings_count_last;

    int human_resource_snapshot[4];
    int human_food_snapshot;

    BotStrategy initial_guess;
    int         last_action_was_sabotage;

    int placements_this_round;
} BotState;

void bot_init(BotState *bs);
void bot_observe_human(BotState *bs, Player *human);
void bot_start_of_round(BotState *bs, Board *b, Player *human, int round_number);
int  bot_choose_action(BotState *bs, Board *b, Player *bot_player, Player *human,
                       int *out_location, int *out_count);
int  bot_decide_tool_use(BotState *bs, Player *bot_player, int base_sum, int location_id);
int  bot_decide_card_payment(BotState *bs, Player *bot_player, int slot_index,
                              int payment[4]);
int  bot_decide_building_purchase(BotState *bs, Player *bot_player, int stack_index);
int  bot_decide_feeding_resource_use(BotState *bs, Player *bot_player, int remaining_to_pay);
void bot_decide_building_variable_payment(BotState *bs, Player *bot_player,
                                          int building_type, int payment[4]);

#endif
