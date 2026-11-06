#include <stdio.h>
#include <time.h>
#include "persistence.h"

void init_log_file(void) {
    FILE *log_file = fopen("game_log.txt", "w");
    if (log_file != NULL) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        fprintf(log_file,
                "============================================================\n"
                "  STONE AGE - NEW GAME SESSION\n"
                "  Started: %04d-%02d-%02d %02d:%02d:%02d\n"
                "============================================================\n",
                t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec);
        fclose(log_file);
    }
}

void log_action(const char *message) {
    FILE *log_file = fopen("game_log.txt", "a");
    if (log_file != NULL) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        fprintf(log_file, "[%02d:%02d:%02d] %s\n",
                t->tm_hour, t->tm_min, t->tm_sec, message);
        fclose(log_file);
    }
}

void save_game(Board *b, Player players[], int num_players,
               int current_round, int current_turn) {
    FILE *f = fopen("savegame.dat", "wb");
    if (f != NULL) {
        fwrite(b, sizeof(Board), 1, f);
        fwrite(players, sizeof(Player), num_players, f);
        fwrite(&current_round, sizeof(int), 1, f);
        fwrite(&current_turn, sizeof(int), 1, f);
        fclose(f);
        log_action("Game state saved to savegame.dat");
    }
}

bool load_game(Board *b, Player players[], int num_players,
               int *current_round, int *current_turn) {
    FILE *f = fopen("savegame.dat", "rb");
    if (f != NULL) {
        size_t r1 = fread(b, sizeof(Board), 1, f);
        size_t r2 = fread(players, sizeof(Player), num_players, f);
        size_t r3 = fread(current_round, sizeof(int), 1, f);
        size_t r4 = fread(current_turn, sizeof(int), 1, f);
        fclose(f);
        if (r1 == 1 && r2 == (size_t)num_players && r3 == 1 && r4 == 1) {
            log_action("Game state loaded from savegame.dat");
            return true;
        }
    }
    return false;
}
