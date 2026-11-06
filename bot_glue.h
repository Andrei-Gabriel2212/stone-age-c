#ifndef BOT_GLUE_H
#define BOT_GLUE_H

#include "bot_eu.h"
#include "bot_coleg.h"
#include "game_types.h"

extern BotState *g_active_bot;
void      bot_glue_set_active(BotState *bs);
BotState *bot_glue_get_active(void);

void            bot_glue_set_active_eu(int player_idx, BotState *bs);
BotState       *bot_glue_get_active_eu(int player_idx);
void            bot_glue_set_active_coleg(int player_idx, BotColegState *bs);
BotColegState  *bot_glue_get_active_coleg(int player_idx);

#endif
