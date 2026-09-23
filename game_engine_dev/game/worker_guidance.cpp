//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "worker_guidance.h"

#include "assert_log.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "map_overlay_enum.h"
#include "overlay_yields.h"
#include "resource_static_data.h"
#include "resource_static_key.h"
#include "runtime_statics.h"
#include "std_add_helper.h"
#include "tile_attr_tables.h"
#include "tile_imp_helper.h"
#include "tile_work_assessor.h"
#include "worker_imp_select.h"
#include "worker_job_enum.h"
#include "worker_job_imp_static_key.h"
#include "worker_job_static_key.h"
#include "worker_job_target_enum.h"
#include "worker_job_type_enum.h"

#include <cstring>

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

const RuntimeStatics* WorkerGuidance::m_st = nullptr;
GameArraySimple* WorkerGuidance::m_map = nullptr;

static u16 clear_job_for_ov (u16 ov) {
    if (ov == static_cast<u16>(MapOverlay::Forest)) {
        return static_cast<u16>(WorkerJob::Clear_Forest);
    }
    if (ov == static_cast<u16>(MapOverlay::Jungle)) {
        return static_cast<u16>(WorkerJob::Clear_Jungle);
    }
    if (ov == static_cast<u16>(MapOverlay::Swamp)) {
        return static_cast<u16>(WorkerJob::Clear_Swamp);
    }
    return U16_KEY_NULL;
}

static u16 res_job_idx (const RuntimeStatics* st, u16 ri) {
    if (st == nullptr || ri == U16_KEY_NULL) {
        return U16_KEY_NULL;
    }
    const ResourceStaticData& rs = st->resource();
    if (ri >= rs.get_item_count()) {
        return U16_KEY_NULL;
    }
    const u16 wj = rs.get_item(ResourceStaticDataKey::from_raw(ri)).worker_job_idx;
    if (wj == U16_KEY_NULL || wj >= st->worker_job().get_item_count()) {
        return U16_KEY_NULL;
    }
    const u16 typ = st->worker_job().get_item(WorkerJobStaticDataKey::from_raw(wj)).type;
    if (static_cast<WorkerJobType>(typ) != WorkerJobType::Resource) {
        return U16_KEY_NULL;
    }
    return wj;
}

static u16 mother_job_for_ov (const RuntimeStatics* st, u16 ov) {
    if (st == nullptr || ov >= OverlayYields::ov_n()) {
        return U16_KEY_NULL;
    }
    const u16 jn = st->worker_job().get_item_count();
    for (u16 j = 0; j < jn; ++j) {
        const WorkerJobStaticDataStruct& row = st->worker_job().get_item(WorkerJobStaticDataKey::from_raw(j));
        if (row.target_kind != static_cast<u16>(WorkerJobTarget::Overlay) || row.target_idx != ov) {
            continue;
        }
        const WorkerJobType typ = static_cast<WorkerJobType>(row.type);
        if (typ == WorkerJobType::Farm || typ == WorkerJobType::Forest) {
            return j;
        }
    }
    return U16_KEY_NULL;
}

static bool ensure_farm (GameArraySimple& map, u16 x, u16 y) {
    const u16 ov = map.get_overlay(x, y);
    if (ov == static_cast<u16>(MapOverlay::City) || ov == static_cast<u16>(MapOverlay::Mine)
        || ov == static_cast<u16>(MapOverlay::Plantation) || ov == static_cast<u16>(MapOverlay::Fort)) {
        return false;
    }
    if (ov == static_cast<u16>(MapOverlay::Farm)) {
        return true;
    }
    if (!map.set_overlay(x, y, static_cast<u16>(MapOverlay::Farm))) {
        return false;
    }
    return map.set_add_idx(x, y, 0u);
}

static bool job_ok (u16 job_idx, u16 x, u16 y) {
    return TileWorkAssessor::tile_ok(job_idx, x, y);
}

static bool clear_work_food (const RuntimeStatics* st, GameArraySimple& map, u16 x, u16 y, u16* job) {
    (void)st;
    if (map.get_terrain(x, y) != TERR_PLAINS[0]) {
        return false;
    }
    const u16 cj = clear_job_for_ov(map.get_overlay(x, y));
    if (cj == U16_KEY_NULL || !job_ok(cj, x, y)) {
        return false;
    }
    *job = cj;
    return true;
}

static bool fort_next_work (const RuntimeStatics* st, GameArraySimple& map, u16 x, u16 y, u16* job, u16* imp) {
    (void)st;
    if (map.get_ai_ov_intent(x, y) != AI_TILE_OV_INTENT_FORT) {
        return false;
    }
    const u16 fj = static_cast<u16>(WorkerJob::Build_Fort);
    if (map.get_overlay(x, y) == static_cast<u16>(MapOverlay::Fort)) {
        const u16 pick = WorkerImpSelect::pick(x, y, fj);
        if (pick == U16_KEY_NULL) {
            return false;
        }
        *job = fj;
        *imp = pick;
        return true;
    }
    if (!job_ok(fj, x, y)) {
        return false;
    }
    *job = fj;
    *imp = U16_KEY_NULL;
    return true;
}

static bool fort_has_work (const RuntimeStatics* st, GameArraySimple& map, u16 x, u16 y) {
    u16 job = U16_KEY_NULL;
    u16 imp = U16_KEY_NULL;
    return fort_next_work(st, map, x, y, &job, &imp);
}

static bool dirt_needed (GameArraySimple& map, u16 x, u16 y) {
    if (road_is_built(map.get_road_typ(x, y))) {
        return false;
    }
    if (road_is_virtual(map.get_road_typ(x, y))) {
        return true;
    }
    const u8 iv = map.get_ai_ov_intent(x, y);
    if (iv == AI_TILE_OV_INTENT_MTN_PASS) {
        return true;
    }
    if (iv != AI_TILE_OV_INTENT_FORT) {
        return false;
    }
    const u8 terr = map.get_terrain(x, y);
    return terr == TERR_MOUNTAINS[0] || terr == TERR_VOLCANO[0];
}

static bool dirt_next_work (GameArraySimple& map, u16 x, u16 y, u16* job, u16* imp) {
    if (!dirt_needed(map, x, y)) {
        return false;
    }
    const u16 dj = static_cast<u16>(WorkerJob::Build_Dirt_Path);
    if (!job_ok(dj, x, y)) {
        return false;
    }
    *job = dj;
    *imp = U16_KEY_NULL;
    return true;
}

static bool dirt_has_work (GameArraySimple& map, u16 x, u16 y) {
    u16 job = U16_KEY_NULL;
    u16 imp = U16_KEY_NULL;
    return dirt_next_work(map, x, y, &job, &imp);
}

static bool res_has_work (const RuntimeStatics* st, GameArraySimple& map, u16 x, u16 y) {
    const u16 rj = res_job_idx(st, map.get_res(x, y));
    if (rj == U16_KEY_NULL) {
        return false;
    }
    const u16 cj = clear_job_for_ov(map.get_overlay(x, y));
    if (cj != U16_KEY_NULL && job_ok(cj, x, y)) {
        return true;
    }
    const u16 job_ov = st->worker_job().get_item(WorkerJobStaticDataKey::from_raw(rj)).target_idx;
    if (map.get_overlay(x, y) == job_ov) {
        return TileWorkAssessor::has_job_work(x, y, rj);
    }
    return job_ok(rj, x, y);
}

static bool res_next_work (const RuntimeStatics* st, GameArraySimple& map, u16 x, u16 y, u16* job, u16* imp) {
    const u16 rj = res_job_idx(st, map.get_res(x, y));
    if (rj == U16_KEY_NULL) {
        return false;
    }
    const u16 cj = clear_job_for_ov(map.get_overlay(x, y));
    if (cj != U16_KEY_NULL && job_ok(cj, x, y)) {
        *job = cj;
        *imp = U16_KEY_NULL;
        return true;
    }
    const u16 job_ov = st->worker_job().get_item(WorkerJobStaticDataKey::from_raw(rj)).target_idx;
    if (map.get_overlay(x, y) == job_ov) {
        const u16 pick = WorkerImpSelect::pick(x, y, rj);
        if (pick == U16_KEY_NULL) {
            return false;
        }
        *job = rj;
        *imp = pick;
        return true;
    }
    if (!job_ok(rj, x, y)) {
        return false;
    }
    *job = rj;
    *imp = U16_KEY_NULL;
    return true;
}

static bool rank_has_work (const RuntimeStatics* st, GameArraySimple& map, u16 x, u16 y, TileAssignIntent intent) {
    u16 rn = 0;
    const u16* ranks = OverlayYields::rank(intent, &rn);
    if (ranks == nullptr) {
        return false;
    }
    for (u16 i = 0; i < rn; ++i) {
        const u16 mj = mother_job_for_ov(st, ranks[i]);
        if (mj == U16_KEY_NULL) {
            continue;
        }
        if (map.get_overlay(x, y) == ranks[i]) {
            if (TileWorkAssessor::has_job_work(x, y, mj)) {
                return true;
            }
            continue;
        }
        if (job_ok(mj, x, y)) {
            return true;
        }
    }
    return false;
}

static bool rank_next_work (const RuntimeStatics* st, GameArraySimple& map, u16 x, u16 y, TileAssignIntent intent, u16* job, u16* imp) {
    u16 rn = 0;
    const u16* ranks = OverlayYields::rank(intent, &rn);
    if (ranks == nullptr) {
        return false;
    }
    for (u16 i = 0; i < rn; ++i) {
        const u16 mj = mother_job_for_ov(st, ranks[i]);
        if (mj == U16_KEY_NULL) {
            continue;
        }
        if (map.get_overlay(x, y) == ranks[i]) {
            const u16 pick = WorkerImpSelect::pick(x, y, mj);
            if (pick == U16_KEY_NULL) {
                continue;
            }
            *job = mj;
            *imp = pick;
            return true;
        }
        if (job_ok(mj, x, y)) {
            *job = mj;
            *imp = U16_KEY_NULL;
            return true;
        }
    }
    return false;
}

static bool apply_imp (const RuntimeStatics& st, GameArraySimple& map, u16 x, u16 y, u16 imp_idx) {
    GameTileSimple* t = map.tile(x, y);
    if (t == nullptr) {
        return false;
    }
    return TileImpHelper::set_imp(t, st, imp_idx);
}

static bool apply_overlay_job (GameArraySimple& map, u16 x, u16 y, u16 job_idx) {
    if (job_idx == U16_KEY_NULL || !job_ok(job_idx, x, y)) {
        return false;
    }
    switch (static_cast<WorkerJob>(job_idx)) {
        case WorkerJob::Clear_Forest:
        case WorkerJob::Clear_Jungle:
        case WorkerJob::Clear_Swamp:
            return map.set_overlay(x, y, U16_KEY_NULL);
        case WorkerJob::Cultivate_Farm: {
            if (!ensure_farm(map, x, y)) {
                return false;
            }
            StdAddHelper::set_farm(map.tile(x, y));
            return true;
        }
        case WorkerJob::Plant_Forest:
            if (!map.set_overlay(x, y, static_cast<u16>(MapOverlay::Forest))) {
                return false;
            }
            return map.set_add_idx(x, y, 0u);
        case WorkerJob::Build_Mine:
            if (!map.set_overlay(x, y, static_cast<u16>(MapOverlay::Mine))) {
                return false;
            }
            return map.set_add_idx(x, y, 0u);
        case WorkerJob::Build_Plantation:
            if (!map.set_overlay(x, y, static_cast<u16>(MapOverlay::Plantation))) {
                return false;
            }
            return map.set_add_idx(x, y, 0u);
        case WorkerJob::Build_Fort:
            if (!map.set_overlay(x, y, static_cast<u16>(MapOverlay::Fort))) {
                return false;
            }
            return map.set_add_idx(x, y, 0u);
        case WorkerJob::Build_Dirt_Path:
            if (road_is_built(map.get_road_typ(x, y))) {
                return false;
            }
            return map.set_road_typ(x, y, ROAD_PATH);
        default:
            return false;
    }
}

//================================================================================================================================
//=> - WorkerGuidance -
//================================================================================================================================

void WorkerGuidance::bind_statics (const RuntimeStatics* st) {
    m_st = st;
    if (st == nullptr) {
        OverlayYields::clear();
        return;
    }
    TileWorkAssessor::setup(*st);
    if (TileAttrTables::ready()) {
        OverlayYields::setup(*st);
    }
}

void WorkerGuidance::bind_map (GameArraySimple* map) {
    m_map = map;
    TileWorkAssessor::bind_map(map);
}

u8 WorkerGuidance::usage_for_intent (u16 x, u16 y, TileAssignIntent intent) {
    GAME_EXPECT(m_map != nullptr, "WorkerGuidance map");
    if (res_job_idx(m_st, m_map->get_res(x, y)) != U16_KEY_NULL) {
        return TILE_USAGE_RESOURCE;
    }
    u16 job = U16_KEY_NULL;
    if (intent == TILE_ASSIGN_FOOD && clear_work_food(m_st, *m_map, x, y, &job)) {
        return TILE_USAGE_FOOD;
    }
    if (rank_has_work(m_st, *m_map, x, y, intent)) {
        return intent == TILE_ASSIGN_FOOD ? TILE_USAGE_FOOD : TILE_USAGE_PROD;
    }
    return TILE_USAGE_NONE;
}

bool WorkerGuidance::has_pending_work (u16 x, u16 y, TileAssignIntent intent) {
    GAME_EXPECT(m_map != nullptr, "WorkerGuidance map");
    if (dirt_has_work(*m_map, x, y)) {
        return true;
    }
    if (res_has_work(m_st, *m_map, x, y)) {
        return true;
    }
    if (fort_has_work(m_st, *m_map, x, y)) {
        return true;
    }
    u16 job = U16_KEY_NULL;
    if (intent == TILE_ASSIGN_FOOD && clear_work_food(m_st, *m_map, x, y, &job)) {
        return true;
    }
    return rank_has_work(m_st, *m_map, x, y, intent);
}

bool WorkerGuidance::next_work (u16 x, u16 y, TileAssignIntent intent, u16* job, u16* imp) {
    GAME_EXPECT(m_map != nullptr, "WorkerGuidance map");
    GAME_EXPECT(job != nullptr && imp != nullptr, "WorkerGuidance next_work out");
    *job = U16_KEY_NULL;
    *imp = U16_KEY_NULL;
    if (dirt_next_work(*m_map, x, y, job, imp)) {
        return true;
    }
    if (res_next_work(m_st, *m_map, x, y, job, imp)) {
        return true;
    }
    if (fort_next_work(m_st, *m_map, x, y, job, imp)) {
        return true;
    }
    if (intent == TILE_ASSIGN_FOOD && clear_work_food(m_st, *m_map, x, y, job)) {
        *imp = U16_KEY_NULL;
        return true;
    }
    return rank_next_work(m_st, *m_map, x, y, intent, job, imp);
}

u16 WorkerGuidance::next_job (u16 x, u16 y, TileAssignIntent intent) {
    u16 job = U16_KEY_NULL;
    u16 imp = U16_KEY_NULL;
    if (!next_work(x, y, intent, &job, &imp)) {
        return U16_KEY_NULL;
    }
    return job;
}

bool WorkerGuidance::apply_work (u16 x, u16 y, u16 job_idx, u16 imp_idx) {
    GAME_EXPECT(m_map != nullptr, "WorkerGuidance map");
    GAME_EXPECT(m_st != nullptr, "WorkerGuidance statics");
    if (imp_idx != U16_KEY_NULL) {
        return apply_imp(*m_st, *m_map, x, y, imp_idx);
    }
    return apply_overlay_job(*m_map, x, y, job_idx);
}

bool WorkerGuidance::apply_job (u16 x, u16 y, u16 job_idx) {
    return apply_work(x, y, job_idx, U16_KEY_NULL);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
