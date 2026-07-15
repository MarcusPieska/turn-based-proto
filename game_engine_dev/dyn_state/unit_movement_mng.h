//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_MOVEMENT_MNG_H
#define UNIT_MOVEMENT_MNG_H

#include "unit_add_vector_key.h"
#include "game_primitives.h"

class GameState;
class RuntimeStatics;

struct UnitMovementMngMvtTbl {
    u16 m_terr_n;
    u16 m_clim_n;
    u16 m_ov_n;
    u16 m_riv;
    u16 m_road;
    u32 m_bytes;
};

//================================================================================================================================
//=> - UnitMovementMng -
//================================================================================================================================
//
//  Stateless static API: sole mutator for unit placement, tile stacks, and move groups.
//  Callers path one step at a time via can_step / apply_step. Turn MP refill is external.
//
//================================================================================================================================

class UnitMovementMng {
public:
    UnitMovementMng () = delete;

    static bool setup_mvt_costs (const RuntimeStatics& st);
    static bool mvt_ready ();
    static u16 mvt_mapped_count ();
    static u16 mvt_cost_count ();
    static void mvt_tables (UnitMovementMngMvtTbl* out);
    static u32 mvt_heap_bytes ();
    static u16 mvt_cost_terr (u8 id);
    static u16 mvt_cost_clim (u8 id);
    static u16 mvt_cost_ov (u8 id);
    static bool map_mvt_cost_name (cstr name, u8* out_id, u8* out_kind);

    static i16 tile_cost (const GameState& s, u16 from_x, u16 from_y, u16 to_x, u16 to_y);
    static i16 grp_min_mvt (const GameState& s, UnitAddKey key);

    static bool can_step (const GameState& s, UnitAddKey key, u16 dest_x, u16 dest_y, i16* out_cost);
    static bool apply_step (GameState& s, UnitAddKey key, u16 dest_x, u16 dest_y);

    static void bind_state (GameState* state);
    static bool finish_unit_spawn (UnitAddKey key, u16 x, u16 y, u16 player_idx);

    static bool place_on_tile (GameState& s, u16 x, u16 y, u16 player_idx, u16 typ_idx, UnitAddKey* out);
    static bool link_group (GameState& s, UnitAddKey head, UnitAddKey tail);
    static bool unlink_group (GameState& s, UnitAddKey tail);
};

#endif // UNIT_MOVEMENT_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
