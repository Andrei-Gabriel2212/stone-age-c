cmake_minimum_required(VERSION 3.20)
project(stoneage C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(stoneage
        main.c
        game_logic.c
        persistence.c
        ui.c
        tests.c
        scoring.c
        bot_eu.c
        bot_glue.c
        bot_coleg.c
        cards.h
        game_types.h
        game_logic.h
        persistence.h
        ui.h
        tests.h
        scoring.h
        bot_eu.h
        bot_glue.h
        bot_coleg.h
)