//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "game_primitives.h"
#include "hlp_print_units.h"

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    if (argc != 2 && argc != 3) {
        std::printf("usage: %s <turn> [list]\n", argc > 0 ? argv[0] : "show_units");
        return 1;
    }
    char* end = nullptr;
    const unsigned long turn_ul = std::strtoul(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0' || turn_ul > 0xfffffffful) {
        std::printf("usage: %s <turn> [list]\n", argv[0]);
        return 1;
    }
    bool list = false;
    if (argc == 3) {
        if (std::strcmp(argv[2], "list") != 0) {
            std::printf("usage: %s <turn> [list]\n", argv[0]);
            return 1;
        }
        list = true;
    }
    const u32 turn = static_cast<u32>(turn_ul);
    if (!Helper_PrintUnits::run(turn, list)) {
        return 1;
    }
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
