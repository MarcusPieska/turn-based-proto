//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TILE_YIELDS_H
#define TILE_YIELDS_H

#include "game_primitives.h"

class BitArrayCL;
class GameArraySimple;
class RuntimeStatics;
class TileYieldsImpDump;

//================================================================================================================================
//=> - TileYield -
//================================================================================================================================
//
//  Per-tile production yields from land attrs (terr+clim+ov+riv), enabled map resources, and improvement boosts.
//
//================================================================================================================================

struct TileYield {
    u8 m_food; // Food yield on this tile
    u8 m_production; // Production yield on this tile
    u8 m_commerce; // Commerce yield on this tile
};

//================================================================================================================================
//=> - TileYieldCtx -
//================================================================================================================================
//
//  Runtime visibility context for yield lookups (researched techs gate resource enablement).
//
//================================================================================================================================

struct TileYieldCtx {
    const BitArrayCL* m_tech; // Researched tech bits; null disables resource yield adds
};

//================================================================================================================================
//=> - TileYields -
//================================================================================================================================
//
//  Static yield lookup on the bound GameArraySimple map. setup unpacks tile attrs and improvement_yield rows into
//  O(1) tables (job gate, then terr/clim/ov/riv ids). get sums land attrs, enabled resources, and improvement boosts.
//  Body is cpp-included from tile_yields_impl/tile_yields_impl_mkNN.cpp via TILE_YIELDS_IMPL.
//
//================================================================================================================================

class TileYields {
    friend class TileYieldsImpDump;

public:
    static bool setup (const RuntimeStatics& st);
    static void bind_map (const GameArraySimple* map);
    static void bind_ctx (const TileYieldCtx* ctx);
    static TileYield get (u16 x, u16 y);
    static bool in_bounds (u16 x, u16 y);

private:
    struct ImpYldSlot {
        i16 m_food;
        i16 m_prod;
        i16 m_comm;
    };

    struct ImpYldJob;

    static const u16 k_terr_n = 16u; // Matches TileAttrTables terr id span
    static const u16 k_clim_n = 5u; // Matches TileAttrTables clim id span
    static const u16 k_ov_n = 16u; // Matches TileAttrTables ov id span

    static void clear_jobs ();
    static bool setup_imp (const RuntimeStatics& st);
    static bool add_amt (ImpYldSlot* s, u16 yld_typ, i16 amt);
    static ImpYldSlot* slot_for (ImpYldJob* job, u8 kind, u8 id);
    static u16 job_on_tile (const GameArraySimple& map, u16 x, u16 y);
    static void add_imp (i32* food, i32* prod, i32* comm, const GameArraySimple& map, u16 x, u16 y);
    static void add_res (i32* food, i32* prod, i32* comm, const GameArraySimple& map, u16 x, u16 y);

    static const GameArraySimple* m_map; // Active match tile grid; null until bind_map
    static const RuntimeStatics* m_st; // Statics from setup; used for resource rows
    static const TileYieldCtx* m_ctx; // Visibility context; null until bind_ctx
    static ImpYldJob* m_jobs; // Per worker_job_idx land boost tables; null if none
    static u16 m_job_n; // Length of m_jobs
};

#endif // TILE_YIELDS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
