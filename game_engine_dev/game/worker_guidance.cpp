//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "worker_guidance.h"

#include "assert_log.h"
#include "build_adds_array.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "resource_static_data.h"
#include "resource_static_key.h"
#include "runtime_statics.h"
#include "std_add_helper.h"
#include "tile_work_assessor.h"
#include "worker_job_enum.h"
#include "worker_job_static_key.h"
#include "worker_job_type_enum.h"
#include "map_overlay_enum.h"

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

//================================================================================================================================
//=> - WorkerGuidance -
//================================================================================================================================

void WorkerGuidance::bind_statics (const RuntimeStatics* st) {
    m_st = st;
    if (st != nullptr) {
        TileWorkAssessor::setup(*st);
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
    if (intent == TILE_ASSIGN_FOOD) {
        if (m_map->get_terrain(x, y) != TERR_PLAINS[0]) {
            return TILE_USAGE_NONE;
        }
        const u16 cj = clear_job_for_ov(m_map->get_overlay(x, y));
        if (cj != U16_KEY_NULL && job_ok(cj, x, y)) {
            return TILE_USAGE_FOOD;
        }
        if (job_ok(static_cast<u16>(WorkerJob::Cultivate_Farm), x, y)) {
            return TILE_USAGE_FOOD;
        }
        return TILE_USAGE_NONE;
    }
    return TILE_USAGE_PROD;
}

u16 WorkerGuidance::next_job (u16 x, u16 y, TileAssignIntent intent) {
    GAME_EXPECT(m_map != nullptr, "WorkerGuidance map");
    const u16 rj = res_job_idx(m_st, m_map->get_res(x, y));
    if (rj != U16_KEY_NULL) {
        const u16 cj = clear_job_for_ov(m_map->get_overlay(x, y));
        if (cj != U16_KEY_NULL && job_ok(cj, x, y)) {
            return cj;
        }
        if (job_ok(rj, x, y)) {
            return rj;
        }
        return U16_KEY_NULL;
    }
    if (intent == TILE_ASSIGN_FOOD) {
        if (m_map->get_terrain(x, y) == TERR_PLAINS[0]) {
            const u16 cj = clear_job_for_ov(m_map->get_overlay(x, y));
            if (cj != U16_KEY_NULL && job_ok(cj, x, y)) {
                return cj;
            }
            if (job_ok(static_cast<u16>(WorkerJob::Cultivate_Farm), x, y)) {
                return static_cast<u16>(WorkerJob::Cultivate_Farm);
            }
        }
        return U16_KEY_NULL;
    }
    if (job_ok(static_cast<u16>(WorkerJob::Plant_Forest), x, y)) {
        return static_cast<u16>(WorkerJob::Plant_Forest);
    }
    return U16_KEY_NULL;
}

bool WorkerGuidance::apply_job (u16 x, u16 y, u16 job_idx) {
    GAME_EXPECT(m_map != nullptr, "WorkerGuidance map");
    if (job_idx == U16_KEY_NULL || !job_ok(job_idx, x, y)) {
        return false;
    }
    switch (static_cast<WorkerJob>(job_idx)) {
        case WorkerJob::Clear_Forest:
        case WorkerJob::Clear_Jungle:
        case WorkerJob::Clear_Swamp:
            return m_map->set_overlay(x, y, U16_KEY_NULL);
        case WorkerJob::Cultivate_Farm: {
            if (!ensure_farm(*m_map, x, y)) {
                return false;
            }
            StdAddHelper::set_farm(m_map->tile(x, y));
            return true;
        }
        case WorkerJob::Plant_Forest:
            if (!m_map->set_overlay(x, y, static_cast<u16>(MapOverlay::Forest))) {
                return false;
            }
            return m_map->set_add_idx(x, y, 0u);
        case WorkerJob::Build_Mine:
            return m_map->set_tile_add(x, y, 0u, BUILD_ADD_MINE);
        case WorkerJob::Build_Plantation:
            return m_map->set_tile_add(x, y, 0u, BUILD_ADD_PLANTATION);
        case WorkerJob::Build_Fort:
            return m_map->set_tile_add(x, y, 0u, BUILD_ADD_FORT);
        default:
            return false;
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
