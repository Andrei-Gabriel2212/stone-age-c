#ifndef CARDS_H
#define CARDS_H

#define CIV_UNIQUE_TYPES      18
#define CIV_COPIES_PER_TYPE    2
#define CIV_TOTAL_CARDS       (CIV_UNIQUE_TYPES * CIV_COPIES_PER_TYPE)

#define NUM_BUILDING_STACKS    4
#define BUILDINGS_PER_STACK    7

#define FOOD_FROM_WOOD   1
#define FOOD_FROM_BRICK  1
#define FOOD_FROM_STONE  2
#define FOOD_FROM_GOLD   3

typedef enum {
    SCORE_FLAT,
    SCORE_PER_TOOL,
    SCORE_PER_HUT,
    SCORE_PER_CARD,
    SCORE_PER_FARM,
    SCORE_MULTIPLY
} CivScoreType;

typedef struct {
    int          id;
    char         name[24];
    CivScoreType score_type;
    int          bonus_score;
    int          bonus_food;
    int          bonus_wood;
    int          bonus_brick;
    int          bonus_stone;
    int          bonus_gold;
    int          tool_bonus;
} CivCard;

typedef enum {
    BUILD_FIXED      = 0,
    BUILD_2KIND_4RES = 1,
    BUILD_ANY_1TO7   = 2
} BuildingType;

typedef struct {
    int          id;
    char         name[20];
    int          cost_wood;
    int          cost_brick;
    int          cost_stone;
    int          cost_gold;
    int          victory_points;
    BuildingType type;
} BuildingCard;

static const CivCard CIV_DECK_TEMPLATE[CIV_TOTAL_CARDS] = {
    {  0, "Pottery",     SCORE_FLAT,      3,  3, 0, 0, 0, 0, 0 },
    {  1, "Weaving",     SCORE_FLAT,      3,  0, 0, 0, 0, 0, 0 },
    {  2, "Scratching",  SCORE_FLAT,      3,  0, 1, 0, 0, 0, 0 },
    {  3, "Transport",   SCORE_FLAT,      3,  0, 0, 1, 0, 0, 0 },
    {  4, "Gemstone",    SCORE_FLAT,      3,  0, 0, 0, 1, 0, 0 },
    {  5, "Time",        SCORE_FLAT,      3,  0, 0, 0, 0, 1, 0 },
    {  6, "Claywork",    SCORE_PER_TOOL,  1,  0, 0, 0, 0, 0, 0 },
    {  7, "Cartography", SCORE_PER_TOOL,  1,  0, 0, 0, 0, 0, 0 },
    {  8, "Shaman",      SCORE_PER_HUT,   1,  0, 0, 0, 0, 0, 0 },
    {  9, "Tribal Art",  SCORE_PER_HUT,   1,  0, 0, 0, 0, 0, 0 },
    { 10, "Healing",     SCORE_PER_CARD,  1,  0, 0, 0, 0, 0, 0 },
    { 11, "Music",       SCORE_PER_CARD,  1,  0, 0, 0, 0, 0, 0 },
    { 12, "Writing",     SCORE_PER_FARM,  1,  0, 0, 0, 0, 0, 0 },
    { 13, "Harvest",     SCORE_PER_FARM,  1,  3, 0, 0, 0, 0, 0 },
    { 14, "Irrigation",  SCORE_MULTIPLY,  1,  4, 0, 0, 0, 0, 0 },
    { 15, "Farmer",      SCORE_MULTIPLY,  1,  3, 0, 0, 0, 0, 0 },
    { 16, "Tool Making", SCORE_FLAT,      2,  0, 0, 0, 0, 0, 1 },
    { 17, "Bookkeeping", SCORE_FLAT,      2,  0, 0, 0, 0, 0, 1 },
    { 18, "Pottery",     SCORE_FLAT,      3,  3, 0, 0, 0, 0, 0 },
    { 19, "Weaving",     SCORE_FLAT,      3,  0, 0, 0, 0, 0, 0 },
    { 20, "Scratching",  SCORE_FLAT,      3,  0, 1, 0, 0, 0, 0 },
    { 21, "Transport",   SCORE_FLAT,      3,  0, 0, 1, 0, 0, 0 },
    { 22, "Gemstone",    SCORE_FLAT,      3,  0, 0, 0, 1, 0, 0 },
    { 23, "Time",        SCORE_FLAT,      3,  0, 0, 0, 0, 1, 0 },
    { 24, "Claywork",    SCORE_PER_TOOL,  1,  0, 0, 0, 0, 0, 0 },
    { 25, "Cartography", SCORE_PER_TOOL,  1,  0, 0, 0, 0, 0, 0 },
    { 26, "Shaman",      SCORE_PER_HUT,   1,  0, 0, 0, 0, 0, 0 },
    { 27, "Tribal Art",  SCORE_PER_HUT,   1,  0, 0, 0, 0, 0, 0 },
    { 28, "Healing",     SCORE_PER_CARD,  1,  0, 0, 0, 0, 0, 0 },
    { 29, "Music",       SCORE_PER_CARD,  1,  0, 0, 0, 0, 0, 0 },
    { 30, "Writing",     SCORE_PER_FARM,  1,  0, 0, 0, 0, 0, 0 },
    { 31, "Harvest",     SCORE_PER_FARM,  1,  3, 0, 0, 0, 0, 0 },
    { 32, "Irrigation",  SCORE_MULTIPLY,  1,  4, 0, 0, 0, 0, 0 },
    { 33, "Farmer",      SCORE_MULTIPLY,  1,  3, 0, 0, 0, 0, 0 },
    { 34, "Tool Making", SCORE_FLAT,      2,  0, 0, 0, 0, 0, 1 },
    { 35, "Bookkeeping", SCORE_FLAT,      2,  0, 0, 0, 0, 0, 1 },
};

static const BuildingCard BUILDING_STACK_TEMPLATE[NUM_BUILDING_STACKS][BUILDINGS_PER_STACK] = {
    {
        {  0, "Blue Tile 1",  1, 0, 0, 0,  2, BUILD_FIXED },
        {  1, "Blue Tile 2",  2, 0, 0, 0,  3, BUILD_FIXED },
        {  2, "Blue Tile 3",  0, 1, 0, 0,  3, BUILD_FIXED },
        {  3, "Blue Tile 4",  1, 1, 0, 0,  4, BUILD_FIXED },
        {  4, "Blue Tile 5",  2, 1, 0, 0,  5, BUILD_FIXED },
        {  5, "Blue Tile 6",  1, 2, 0, 0,  6, BUILD_FIXED },
        {  6, "Blue Tile 7",  2, 2, 0, 0,  7, BUILD_FIXED },
    },
    {
        {  7, "Green Tile 1", 1, 1, 0, 0,  3, BUILD_FIXED },
        {  8, "Green Tile 2", 0, 1, 1, 0,  4, BUILD_FIXED },
        {  9, "Green Tile 3", 1, 0, 1, 0,  4, BUILD_FIXED },
        { 10, "Green Tile 4", 2, 1, 0, 0,  5, BUILD_FIXED },
        { 11, "Green Tile 5", 1, 1, 1, 0,  6, BUILD_FIXED },
        { 12, "Green Tile 6", 2, 0, 1, 0,  6, BUILD_FIXED },
        { 13, "Green Tile 7", 2, 1, 1, 0,  8, BUILD_FIXED },
    },
    {
        { 14, "Yell. Tile 1", 0, 0, 2, 0,  4, BUILD_FIXED },
        { 15, "Yell. Tile 2", 1, 0, 2, 0,  5, BUILD_FIXED },
        { 16, "Yell. Tile 3", 0, 1, 2, 0,  6, BUILD_FIXED },
        { 17, "Yell. Tile 4", 1, 1, 2, 0,  7, BUILD_FIXED },
        { 18, "Yell. Tile 5", 0, 0, 3, 0,  7, BUILD_FIXED },
        { 19, "Yell. Tile 6", 1, 0, 3, 0,  8, BUILD_FIXED },
        { 20, "Yell. Tile 7", 0, 0, 0, 0,  0, BUILD_ANY_1TO7 },
    },
    {
        { 21, "Red Tile 1",   0, 0, 0, 1,  3, BUILD_FIXED },
        { 22, "Red Tile 2",   1, 0, 0, 1,  5, BUILD_FIXED },
        { 23, "Red Tile 3",   0, 1, 0, 1,  5, BUILD_FIXED },
        { 24, "Red Tile 4",   0, 0, 1, 1,  6, BUILD_FIXED },
        { 25, "Red Tile 5",   1, 1, 0, 1,  7, BUILD_FIXED },
        { 26, "Red Tile 6",   1, 0, 1, 1,  8, BUILD_FIXED },
        { 27, "Red Tile 7",   0, 0, 0, 0,  0, BUILD_2KIND_4RES },
    },
};

#endif
