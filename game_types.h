#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <stdbool.h>
#include "cards.h"

typedef enum {
    BOT_KIND_NONE  = 0,
    BOT_KIND_EU    = 1,
    BOT_KIND_COLEG = 2
} BotKind;

#define COLOR_RESET    "\x1b[0m"
#define COLOR_BOLD     "\x1b[1m"
#define COLOR_DIM      "\x1b[2m"
#define COLOR_RED      "\x1b[1;31m"
#define COLOR_GREEN    "\x1b[1;32m"
#define COLOR_YELLOW   "\x1b[1;33m"
#define COLOR_BLUE     "\x1b[1;34m"
#define COLOR_MAGENTA  "\x1b[1;35m"
#define COLOR_CYAN     "\x1b[1;36m"
#define COLOR_WHITE    "\x1b[1;37m"
#define COLOR_ORANGE   "\x1b[38;5;208m"
#define COLOR_BROWN    "\x1b[38;5;130m"
#define COLOR_GRAY     "\x1b[38;5;245m"
#define COLOR_PINK     "\x1b[38;5;213m"
#define BG_DARK        "\x1b[48;5;235m"

#define NUM_PLAYERS         2
#define MAX_PEOPLE          10
#define MAX_TOOL_VALUE      4
#define STARTING_PEOPLE      5
#define STARTING_FOOD       12
#define FEED_PENALTY        10
#define RESOURCE_SPACE_LIMIT 7

#define LOC_HUNTING     0
#define LOC_TOOL_MAKER  1
#define LOC_FIELD       2
#define LOC_HUT         3
#define LOC_WOOD        4
#define LOC_BRICK       5
#define LOC_STONE       6
#define LOC_GOLD        7
#define LOC_CARD_BASE   8
#define LOC_BUILD_BASE 12
#define NUM_LOCATIONS  16

#define RES_WOOD   0
#define RES_BRICK  1
#define RES_STONE  2
#define RES_GOLD   3

typedef struct {
    int player_id;
    int is_bot;
    int bot_kind;
    int bot_coleg_persona;
    int people_total;
    int people_available;
    int food_amount;
    int food_production;
    int current_score;
    int player_resources[4];
    int permanent_tools[3];
    int tools_used[3];
    int temporary_tool;
    int people_on_location[NUM_LOCATIONS];

    CivCard inventory_cards[CIV_TOTAL_CARDS];
    int cards_count;
    BuildingCard inventory_buildings[NUM_BUILDING_STACKS * BUILDINGS_PER_STACK];
    int buildings_count;
} Player;

typedef struct {
    int global_supply[4];
    int food_supply;
    int location_occupants[NUM_LOCATIONS];
    int location_owner[NUM_LOCATIONS];
    bool village_used[3];

    CivCard civilization_deck[CIV_TOTAL_CARDS];
    int deck_index;
    CivCard active_cards[4];
    BuildingCard building_stacks[NUM_BUILDING_STACKS][BUILDINGS_PER_STACK];
    int stack_counters[4];
    int start_player;
} Board;

#endif
