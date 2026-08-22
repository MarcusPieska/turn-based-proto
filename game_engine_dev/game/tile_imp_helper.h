//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TILE_IMP_HELPER_H
#define TILE_IMP_HELPER_H

#include "game_primitives.h"

struct GameTileSimple;
class RuntimeStatics;

//================================================================================================================================
//=> - TileImpHelper -
//================================================================================================================================
//
//  Maps worker_job_imp catalog rows to m_add_idx payload bits per overlay. Farm/Forest keep legacy bit
//  positions; Mine, Plantation, and Fort use imp_index slot as (1 << slot) under their overlay.
//
//================================================================================================================================

class TileImpHelper {
public:
    static const u16 m_fr_wm_bit = 8u; // Forest slot-1 Water Mill; distinct from Saw Mill mill_bit

    static u16 imp_slot (const RuntimeStatics& st, u16 imp_idx);
    static u16 payload_bit (const RuntimeStatics& st, u16 imp_idx);
    static bool has_imp (const GameTileSimple* t, const RuntimeStatics& st, u16 imp_idx);
    static bool set_imp (GameTileSimple* t, const RuntimeStatics& st, u16 imp_idx);

private:
    TileImpHelper () = delete;
};

#endif // TILE_IMP_HELPER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
