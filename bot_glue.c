#include <stddef.h>
#include "bot_glue.h"

BotState *g_active_bot = NULL;

void      bot_glue_set_active(BotState *bs) { g_active_bot = bs; }
BotState *bot_glue_get_active(void)         { return g_active_bot; }

static BotState      *s_eu_bots[NUM_PLAYERS]    = {NULL};
static BotColegState *s_coleg_bots[NUM_PLAYERS] = {NULL};

void bot_glue_set_active_eu(int player_idx, BotState *bs) {
    if (player_idx >= 0 && player_idx < NUM_PLAYERS) s_eu_bots[player_idx] = bs;
}
BotState *bot_glue_get_active_eu(int player_idx) {
    if (player_idx >= 0 && player_idx < NUM_PLAYERS) return s_eu_bots[player_idx];
    return NULL;
}

void bot_glue_set_active_coleg(int player_idx, BotColegState *bs) {
    if (player_idx >= 0 && player_idx < NUM_PLAYERS) s_coleg_bots[player_idx] = bs;
}
BotColegState *bot_glue_get_active_coleg(int player_idx) {
    if (player_idx >= 0 && player_idx < NUM_PLAYERS) return s_coleg_bots[player_idx];
    return NULL;
}
