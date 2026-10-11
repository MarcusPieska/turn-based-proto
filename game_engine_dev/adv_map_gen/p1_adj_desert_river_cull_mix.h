//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_ADJ_DESERT_RIVER_CULL_MIX_H
#define P1_ADJ_DESERT_RIVER_CULL_MIX_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_map_size.h"
#include "p1_tile_flags.h"

//================================================================================================================================
//=> - P1_Adj_DesertRiverCullMix -
//================================================================================================================================
//
//  Rank river systems by hot_desert river-tile fraction; mouth-cull (mode 2) the top m_harsh_pct
//  percent, tip-cull (mode 1) the rest. m_harsh marks mode-2 systems. Used by P1_MakeMap.
//
//================================================================================================================================

struct P1_Adj_DesertRiverCullMixPrm {
    u8 m_harsh_pct; // Top percent of systems (by hot_desert fraction) that use mouth cull
};

static inline P1_Adj_DesertRiverCullMixPrm p1_adj_desert_river_cull_mix_prm_def () {
    P1_Adj_DesertRiverCullMixPrm p;
    p.m_harsh_pct = 7u;
    return p;
}

struct P1_Adj_DesertRiverCullMixRslt {
    u16 m_w; // Map width
    u16 m_h; // Map height
    u32 m_sys_n; // River systems found
    u32 m_harsh_n; // Systems assigned mouth cull
    u32 m_mild_n; // Systems assigned tip cull
    u32 m_harsh_tiles; // Pre-cull tiles in mouth-cull systems
    u32 m_mild_tiles; // Pre-cull tiles in tip-cull systems
    u32 m_culled_n; // River tiles cleared
    MapArrayOverlay m_harsh; // Pre-cull tiles of mouth-cull systems
    MapArrayOverlay m_mild; // Pre-cull tiles of tip-cull systems
};

class P1_Adj_DesertRiverCullMix {
public:
    explicit P1_Adj_DesertRiverCullMix (
        const P1_RunPrm& prm,
        const P1_Adj_DesertRiverCullMixPrm& sp = p1_adj_desert_river_cull_mix_prm_def ());

    bool adjust (
        u8* riv,
        u16 w,
        u16 h,
        const u8* terrain,
        const u8* climate,
        const P1_TileFlags* flags);
    bool is_valid () const;
    const P1_Adj_DesertRiverCullMixRslt& result () const;

private:
    P1_Adj_DesertRiverCullMix (const P1_Adj_DesertRiverCullMix& other) = delete;
    P1_Adj_DesertRiverCullMix (P1_Adj_DesertRiverCullMix&& other) = delete;

    P1_RunPrm m_prm; // Run seed/size
    P1_Adj_DesertRiverCullMixPrm m_sp; // Harsh percent knob
    bool m_ok; // True after successful adjust
    P1_Adj_DesertRiverCullMixRslt m_rslt; // Harsh mark and counts
};

#endif // P1_ADJ_DESERT_RIVER_CULL_MIX_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
