//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef OVERLAY_YIELDS_H
#define OVERLAY_YIELDS_H

#include "game_primitives.h"
#include "tile_usage.h"

class RuntimeStatics;

//================================================================================================================================
//=> - OvYldTot -
//================================================================================================================================
//
//  Fully developed yield package for one map_overlay on a fixed baseline (plains terr + plains climate, no river).
//
//================================================================================================================================

struct OvYldTot {
    i16 m_food; // Summed food at full development
    i16 m_prod; // Summed production at full development
    i16 m_comm; // Summed commerce at full development
    u16 m_imp_n; // Catalog imps under this overlay (WorkerJobImpIndex)
};

//================================================================================================================================
//=> - OverlayYields -
//================================================================================================================================
//
//  Static totals and intent rankings for generic overlays (Farm / Forest mother jobs). Resource overlays are not
//  ranked here; at tile time they preempt via a fast get_res short-circuit. setup after TileAttrTables::setup.
//
//================================================================================================================================

class OverlayYields {
public:
    OverlayYields () = delete;

    static bool setup (const RuntimeStatics& st);
    static void clear ();

    static u16 ov_n ();
    static OvYldTot tot (u16 ov);
    static bool is_res (u16 ov);
    static const u16* rank (TileAssignIntent intent, u16* out_n);

private:
    static void fill_tots (const RuntimeStatics& st);
    static void fill_ranks (const RuntimeStatics& st);
    static bool base_ok (u8 kind, u8 id, u16 ov);
    static bool rank_cand (const RuntimeStatics& st, u16 ov);
    static int cmp_food (u16 a, u16 b);
    static int cmp_prod (u16 a, u16 b);
    static void sort_rank (u16* ids, u16 n, int (*cmp)(u16, u16));

    static const RuntimeStatics* m_st; // Bound catalog; null until setup
    static OvYldTot* m_tot; // Per map_overlay fully developed totals
    static u16* m_food_rk; // Overlays ordered best food first
    static u16* m_prod_rk; // Overlays ordered best production first
    static u8* m_res; // 1 if overlay has a Resource mother job
    static u16 m_ov_n; // map_overlay catalog size
    static u16 m_food_n; // Length of m_food_rk
    static u16 m_prod_n; // Length of m_prod_rk
};

#endif // OVERLAY_YIELDS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
