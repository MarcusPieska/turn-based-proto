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
//  Maps worker_job_imp catalog rows to m_add_idx payload bits via WorkerJobImpIndex slot (1 << slot). Validation
//  masks are derived from imp_n per map_overlay row in game data. bind_statics before GameArraySimple checks.
//
//================================================================================================================================

class TileImpHelper {
public:
    static void bind_statics (const RuntimeStatics* st);

    static u16 imp_slot (const RuntimeStatics& st, u16 imp_idx);
    static u16 payload_bit (const RuntimeStatics& st, u16 imp_idx);
    static u16 payload_mask_for_ov (const RuntimeStatics& st, u16 ov);
    static bool add_idx_ok (const RuntimeStatics& st, u16 ov, u16 add_idx);
    static bool add_idx_ok (u16 ov, u16 add_idx);
    static bool has_imp (const GameTileSimple* t, const RuntimeStatics& st, u16 imp_idx);
    static bool set_imp (GameTileSimple* t, const RuntimeStatics& st, u16 imp_idx);

private:
    static const RuntimeStatics* m_st; // Runtime catalog; null until bind_statics

    TileImpHelper () = delete;
};

#endif // TILE_IMP_HELPER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
