//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_DESERT_RIVER_CULL_H
#define P1_GEN_DESERT_RIVER_CULL_H

#include "game_primitives.h"
#include "p1_map_size.h"
#include "p1_tile_flags.h"

//================================================================================================================================
//=> - P1_Gen_DesertRiverCull -
//================================================================================================================================
//
//  Mode 1: tip-walk cull on desert. Mode 2: mouth-walk cull on desert + orphan wipe.
//  Mode 3: mouth-walk cull on hot_desert / inland lake / inland sea + orphan wipe, then restore
//  from mountain-adjacent pre-cull tips until the same stop set.
//
//================================================================================================================================

static const u8 P1_DESERT_CULL_TIP = 1u;
static const u8 P1_DESERT_CULL_MOUTH = 2u;
static const u8 P1_DESERT_CULL_MOUTH_RESTORE = 3u;

class P1_Gen_DesertRiverCull {
public:
    explicit P1_Gen_DesertRiverCull (const P1_RunPrm& prm);

    bool generate (
        u8* riv,
        u16 w,
        u16 h,
        const u8* terrain,
        const u8* climate,
        bool cull_from_upstream);
    bool generate (
        u8* riv,
        u16 w,
        u16 h,
        const u8* terrain,
        const u8* climate,
        const P1_TileFlags* flags,
        u8 mode);
    bool is_valid () const;
    u32 culled_n () const;

private:
    P1_Gen_DesertRiverCull (const P1_Gen_DesertRiverCull& other) = delete;
    P1_Gen_DesertRiverCull (P1_Gen_DesertRiverCull&& other) = delete;

    P1_RunPrm m_prm; // Run seed/size
    bool m_valid_generation; // True after successful generate
    u32 m_culled_n; // River tiles removed net of restores
};

#endif // P1_GEN_DESERT_RIVER_CULL_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
