//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WORKER_GUIDANCE_H
#define WORKER_GUIDANCE_H

#include "game_primitives.h"
#include "tile_usage.h"

class GameArraySimple;
class RuntimeStatics;

//================================================================================================================================
//=> - WorkerGuidance -
//================================================================================================================================
//
//  Static intent-to-job mapping for workers on assigned tiles. Placement uses TileWorkAssessor::tile_ok.
//  bind_statics and bind_map also wire the assessor; bind a TileWorkCtx on the assessor when tech-gating.
//
//================================================================================================================================

class WorkerGuidance {
public:
    static void bind_statics (const RuntimeStatics* st);
    static void bind_map (GameArraySimple* map);
    static u8 usage_for_intent (u16 x, u16 y, TileAssignIntent intent);
    static u16 next_job (u16 x, u16 y, TileAssignIntent intent);
    static bool apply_job (u16 x, u16 y, u16 job_idx);

private:
    static const RuntimeStatics* m_st; // Runtime catalog; null until bind_statics
    static GameArraySimple* m_map; // Active tile grid; null until bind_map
};

#endif // WORKER_GUIDANCE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
