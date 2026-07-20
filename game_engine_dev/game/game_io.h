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

//================================================================================================================================
//=> - GameIo -
//================================================================================================================================
//
//  Binary dumps of live match arrays for end-of-run inspection. Map tiles alone are not enough to interpret
//  m_unit_hd / m_add_idx / city worker keys; pair save_map_tiles with save_units and save_cities.
//  save_cities also writes CityArray's three GeneralBitBanks (flags, resources, buildings).
//  save_players writes per-seat commerce/research and the researched-tech bitset.
//
//================================================================================================================================

class GameIo {
public:
    GameIo () = delete;

    static bool save_map_tiles (cstr path, const GameArraySimple& map);
    static bool save_units (cstr path, const UnitAddVector& units);
    static bool save_cities (cstr path, const CityArray& cities);
    static bool save_players (cstr path, const GameState& state);

private:
    static bool wr_bit_bank (void* fp, const class GeneralBitBank* bank);
    static bool wr_bit_cl (void* fp, const class BitArrayCL* ba);
};

#endif // GAME_IO_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
