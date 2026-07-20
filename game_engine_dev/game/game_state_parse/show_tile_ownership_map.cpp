//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>

#include "game_primitives.h"
#include "hlp_make_tile_owner_map.h"
#include "hlp_print_tile_owner.h"

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    if (argc != 2) {
        std::printf("usage: %s <turn>\n", argc > 0 ? argv[0] : "show_tile_ownership_map");
        return 1;
    }
    char* end = nullptr;
    const unsigned long turn_ul = std::strtoul(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0' || turn_ul > 0xfffffffful) {
        std::printf("usage: %s <turn>\n", argv[0]);
        return 1;
    }
    const u32 turn = static_cast<u32>(turn_ul);
    if (!Helper_PrintTileOwner::run(turn)) {
        return 1;
    }
    if (!Helper_MakeTileOwnerMap::run(turn)) {
        return 1;
    }
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
