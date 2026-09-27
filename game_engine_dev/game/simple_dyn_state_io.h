//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SIMPLE_DYN_STATE_IO_H
#define SIMPLE_DYN_STATE_IO_H

#include "game_primitives.h"

class GameState;
class PlayerState;

//================================================================================================================================
//=> - SimpleDynStateIO -
//================================================================================================================================
//
//  Binary (de)serialize of the PlayerState seat array. Used by GameIo::save/load_players.
//  Writes every PlayerState field not owned by another GameState array; heap leaves are inlined.
//  TechAgeMng process statics are not written (setup-once). Blob: magic + ver + counts + seats.
//
//================================================================================================================================

class SimpleDynStateIO {
public:
    static const u32 k_magic = 0x53445950u; // 'SDYP'
    static const u32 k_ver = 1u;

    SimpleDynStateIO () = delete;

    static bool save_players (cstr path, const PlayerState* seats, u16 player_n, u16 small_wonder_n);
    static bool save_players (cstr path, const GameState& state);
    static bool load_players (cstr path, PlayerState*& seats, u16& player_n);
    static bool load_players (cstr path, GameState& state);

private:
    static bool wr_bit_cl (void* fp, const class BitArrayCL* ba);
    static bool rd_bit_cl (void* fp, class BitArrayCL** out);
    static bool wr_seat (void* fp, const PlayerState& ps, u16 small_wonder_n);
    static bool rd_seat (void* fp, PlayerState& ps, u16 small_wonder_n);
    static void clr_seat (PlayerState& ps);
    static void clr_seats (PlayerState* seats, u16 player_n);
};

#endif // SIMPLE_DYN_STATE_IO_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
