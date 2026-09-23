//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GAME_IO_H
#define GAME_IO_H

#include "game_primitives.h"

class GameArraySimple;
class UnitAddVector;
class CityArray;
class GameState;
class PlayerState;

//================================================================================================================================
//=> - GameIo -
//================================================================================================================================
//
//  Binary dumps of live match arrays for end-of-run inspection. Map tiles alone are not enough to interpret
//  m_unit_hd / m_add_idx / city worker keys; pair save_map_tiles with save_units and save_cities.
//  save_cities also writes CityArray's three GeneralBitBanks (flags, resources, buildings).
//  save_players writes per-seat commerce/research and the researched-tech bitset.
//  load_* restores the same blobs into hot-path structures (subset for cities/players as written).
//
//================================================================================================================================

class GameIo {
public:
    GameIo () = delete;

    static bool save_map_tiles (cstr path, const GameArraySimple& map);
    static bool save_units (cstr path, const UnitAddVector& units);
    static bool save_cities (cstr path, const CityArray& cities);
    static bool save_players (cstr path, const PlayerState* seats, u16 player_n);
    static bool save_players (cstr path, const GameState& state);

    static bool load_map_tiles (cstr path, GameArraySimple& map);
    static bool load_units (cstr path, UnitAddVector& units);
    static bool load_cities (cstr path, CityArray& cities);
    static bool load_players (cstr path, PlayerState*& seats, u16& player_n);
    static bool load_players (cstr path, GameState& state);

private:
    static bool wr_bit_bank (void* fp, const class GeneralBitBank* bank);
    static bool wr_bit_cl (void* fp, const class BitArrayCL* ba);
    static bool rd_bit_bank (void* fp, class GeneralBitBank** out);
    static bool rd_bit_cl (void* fp, class BitArrayCL** out);
    static void clr_units (UnitAddVector& units);
    static void clr_cities (CityArray& cities);
};

#endif // GAME_IO_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
