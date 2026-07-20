//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "hlp_parse_seed.h"

#include <cstdio>
#include <cstring>

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_SEED_FILE = "/home/w/Projects/rts-proto/game_engine_dev/game/game_state_parse/parse_seed.txt";
static const char* G_SAVES = "/home/w/Projects/game-saves";

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

u32 Helper_ParseSeed::m_seed = 0;
u16 Helper_ParseSeed::m_players = 0;
bool Helper_ParseSeed::m_ok = false;

//================================================================================================================================
//=> - Helper_ParseSeed -
//================================================================================================================================

bool Helper_ParseSeed::load () {
    m_ok = false;
    m_seed = 0;
    m_players = 0;
    std::FILE* fp = std::fopen(G_SEED_FILE, "r");
    if (fp == nullptr) {
        return false;
    }
    unsigned long seed = 0;
    unsigned long players = 0;
    if (std::fscanf(fp, "%lu", &seed) != 1 || std::fscanf(fp, "%lu", &players) != 1) {
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    if (seed == 0 || players == 0 || players > 0xfffful) {
        return false;
    }
    m_seed = static_cast<u32>(seed);
    m_players = static_cast<u16>(players);
    m_ok = true;
    return true;
}

u32 Helper_ParseSeed::seed () {
    return m_seed;
}

u16 Helper_ParseSeed::players () {
    return m_players;
}

bool Helper_ParseSeed::fill (char* buf, u32 cap, cstr suffix, u32 turn) {
    if (!m_ok || buf == nullptr || suffix == nullptr || cap == 0) {
        return false;
    }
    return std::snprintf(buf, cap, "%s/game-loop-seed-%u-p%u-%04u%s",
        G_SAVES, m_seed, m_players, turn, suffix) > 0;
}

bool Helper_ParseSeed::map_path (u32 turn, char* buf, u32 cap) {
    return fill(buf, cap, ".bin", turn);
}

bool Helper_ParseSeed::cities_path (u32 turn, char* buf, u32 cap) {
    return fill(buf, cap, "-cities.bin", turn);
}

bool Helper_ParseSeed::units_path (u32 turn, char* buf, u32 cap) {
    return fill(buf, cap, "-units.bin", turn);
}

bool Helper_ParseSeed::players_path (u32 turn, char* buf, u32 cap) {
    return fill(buf, cap, "-players.bin", turn);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
