//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef HLP_PARSE_SEED_H
#define HLP_PARSE_SEED_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Helper_ParseSeed -
//================================================================================================================================
//
//  Reads parse_seed.txt (seed line, players line) beside the parse tools. Builds game-saves dump paths for a turn.
//
//================================================================================================================================

class Helper_ParseSeed {
public:
    Helper_ParseSeed () = delete;

    static bool load ();
    static u32 seed ();
    static u16 players ();

    static bool map_path (u32 turn, char* buf, u32 cap);
    static bool cities_path (u32 turn, char* buf, u32 cap);
    static bool units_path (u32 turn, char* buf, u32 cap);
    static bool players_path (u32 turn, char* buf, u32 cap);

private:
    static bool fill (char* buf, u32 cap, cstr suffix, u32 turn);

    static u32 m_seed;
    static u16 m_players;
    static bool m_ok;
};

#endif // HLP_PARSE_SEED_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
