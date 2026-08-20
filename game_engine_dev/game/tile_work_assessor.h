//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TILE_WORK_ASSESSOR_H
#define TILE_WORK_ASSESSOR_H

#include "game_primitives.h"

class BitArrayCL;
class GameArraySimple;
class RuntimeStatics;

//================================================================================================================================
//=> - TileWorkCtx -
//================================================================================================================================
//
//  Player visibility for work eligibility (techs gate worker_job / worker_job_imp reqs).
//
//================================================================================================================================

struct TileWorkCtx {
    const BitArrayCL* m_tech; // Researched tech bits; null fails tech reqs
    const BitArrayCL* m_resource; // Owned resources; null fails resource reqs
};

//================================================================================================================================
//=> - TileWorkCand -
//================================================================================================================================
//
//  One eligible work action on a tile. m_imp is U16_KEY_NULL when the mother job has no catalog imps.
//
//================================================================================================================================

struct TileWorkCand {
    u16 m_job; // worker_job catalog index
    u16 m_imp; // worker_job_imp catalog index, or U16_KEY_NULL
};

//================================================================================================================================
//=> - TileWorkAssessor -
//================================================================================================================================
//
//  What worker jobs/imps can run on a tile given player tech (and resources). Placement geography is
//  resolved per WorkerJobType in tile_ok (single source of truth). mk01 full-scans the
//  worker_job_imp_index; later mks may replace the scan with a tile_key map behind the same API.
//  Body is cpp-included from impl/tile_work_assessor_impl_mkNN.cpp via TILE_WORK_ASSESSOR_IMPL.
//
//================================================================================================================================

class TileWorkAssessor {
public:
    static bool setup (const RuntimeStatics& st);
    static void bind_map (const GameArraySimple* map);
    static void bind_ctx (const TileWorkCtx* ctx);
    static bool in_bounds (u16 x, u16 y);
    static u16 assess (u16 x, u16 y, TileWorkCand* out, u16 out_cap);
    static u16 assess_job (u16 x, u16 y, u16 job_idx, TileWorkCand* out, u16 out_cap);
    static bool tile_ok (u16 job_idx, u16 x, u16 y);

private:
    static const GameArraySimple* m_map;
    static const RuntimeStatics* m_st;
    static const TileWorkCtx* m_ctx;
};

#endif // TILE_WORK_ASSESSOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
