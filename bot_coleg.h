#ifndef BOT_COLEG_H
#define BOT_COLEG_H

#include "game_types.h"

typedef enum {
    COLEG_RESOURCE_RUSH   = 0,
    COLEG_CARD_SNIPER     = 1,
    COLEG_ENGINE_BUILDER  = 2
} ColegPersonality;

typedef struct {
    ColegPersonality personality;
    int current_round;
} BotColegState;

void bot_coleg_init(BotColegState *bs, ColegPersonality persona);
void bot_coleg_start_of_round(BotColegState *bs, int round_number);

int  bot_coleg_choose_action(BotColegState *bs, Board *b, Player *bot_player, Player *opponent,
                              int *out_location, int *out_count);

int  bot_coleg_decide_tool_use(BotColegState *bs, Player *bot_player, int base_sum, int location_id);
int  bot_coleg_decide_card_payment(BotColegState *bs, Player *bot_player, int slot_index, int payment[4]);
int  bot_coleg_decide_building_purchase(BotColegState *bs, Player *bot_player, int stack_index, Board *b);
int  bot_coleg_decide_feeding_resource_use(BotColegState *bs, Player *bot_player, int remaining_to_pay);

const char *bot_coleg_personality_name(ColegPersonality p);

#endif
