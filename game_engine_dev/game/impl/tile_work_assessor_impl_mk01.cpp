//================================================================================================================================
//=> - Includes (mk01: full scan of worker_job_imp_index) -
//================================================================================================================================

#include "bit_array.h"
#include "build_adds_array.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "general_assessor.h"
#include "resource_static_key.h"
#include "runtime_statics.h"
#include "tile_yields.h"
#include "worker_job_enum.h"
#include "worker_job_imp_index.h"
#include "worker_job_imp_static_data.h"
#include "worker_job_imp_static_key.h"
#include "worker_job_static_data.h"
#include "worker_job_static_key.h"
#include "worker_job_type_enum.h"

//================================================================================================================================
//=> - Local helpers -
//================================================================================================================================

static AssessorCtx make_ctx (const TileWorkCtx* tw) {
    AssessorCtx ctx = {};
    if (tw != nullptr) {
        ctx.m_tech = tw->m_tech;
        ctx.m_resource = tw->m_resource;
    }
    return ctx;
}

static bool job_unlocked (const RuntimeStatics& st, u16 job_idx, const AssessorCtx& ctx) {
    if (job_idx >= st.worker_job().get_item_count()) {
        return false;
    }
    const WorkerJobStaticDataStruct& row = st.worker_job().get_item(WorkerJobStaticDataKey::from_raw(job_idx));
    return GeneralAssessor::chk(row.reqs, ctx);
}

static bool imp_unlocked (const RuntimeStatics& st, u16 imp_idx, const AssessorCtx& ctx) {
    if (imp_idx >= st.worker_job_imp().get_item_count()) {
        return false;
    }
    const WorkerJobImpStaticDataStruct& row = st.worker_job_imp().get_item(WorkerJobImpStaticDataKey::from_raw(imp_idx));
    return GeneralAssessor::chk(row.reqs, ctx);
}

static bool push_cand (TileWorkCand* out, u16 out_cap, u16* n, u16 job_idx, u16 imp_idx) {
    if (*n >= out_cap) {
        return false;
    }
    out[*n].m_job = job_idx;
    out[*n].m_imp = imp_idx;
    ++(*n);
    return true;
}

static bool place_ok_farm (const GameArraySimple& map, u16 job_idx, u16 x, u16 y) {
    const u8 terr = map.get_terrain(x, y);
    const u8 ov = map.get_overlay(x, y);
    return terr == TERR_PLAINS[0] && ov == OV_NONE[0] && TileYields::job_raises_food(x, y, job_idx);
}

static bool place_ok_road (const GameArraySimple& map, u16 x, u16 y) {
    return !overlay_is_water_terr(map.get_terrain(x, y));
}

static bool place_ok_forest (const GameArraySimple& map, u16 x, u16 y) {
    return map.get_overlay(x, y) == OV_FOREST[0];
}

static bool clear_target_ov (u16 job_idx, u8* out_ov) {
    switch (static_cast<WorkerJob>(job_idx)) {
        case WorkerJob::Clear_Forest:
            *out_ov = OV_FOREST[0];
            return true;
        case WorkerJob::Clear_Jungle:
            *out_ov = OV_JUNGLE[0];
            return true;
        case WorkerJob::Clear_Swamp:
            *out_ov = OV_SWAMP[0];
            return true;
        default:
            return false;
    }
}

static bool place_ok_clearing (const GameArraySimple& map, u16 job_idx, u16 x, u16 y) {
    u8 want = 0;
    if (!clear_target_ov(job_idx, &want)) {
        return false;
    }
    return map.get_overlay(x, y) == want;
}

static bool strategic_add_typ (u8 typ) {
    return typ == BUILD_ADD_FORT || typ == BUILD_ADD_OUTPOST || typ == BUILD_ADD_SHIPYARD
        || typ == BUILD_ADD_TRADE_POST || typ == BUILD_ADD_MONASTERY;
}

static bool adj_same_strategic (const GameArraySimple& map, u16 job_idx, u16 x, u16 y) {
    static const i8 k_dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    static const i8 k_dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    const int w = static_cast<int>(map.width());
    const int h = static_cast<int>(map.height());
    for (u32 i = 0; i < 8u; ++i) {
        const int nx = static_cast<int>(x) + k_dx[i];
        const int ny = static_cast<int>(y) + k_dy[i];
        if (nx < 0 || ny < 0 || nx >= w || ny >= h) {
            continue;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        if (!strategic_add_typ(map.get_add_typ(ux, uy))) {
            continue;
        }
        if (map.get_add_idx(ux, uy) == job_idx) {
            return true;
        }
    }
    return false;
}

static bool place_ok_strategic (const GameArraySimple& map, u16 job_idx, u16 x, u16 y) {
    const u8 terr = map.get_terrain(x, y);
    if (overlay_is_water_terr(terr)) {
        return false;
    }
    if (terr == TERR_MOUNTAINS[0] || terr == TERR_VOLCANO[0]) {
        if (map.get_road_typ(x, y) == ROAD_NONE) {
            return false;
        }
    }
    if (strategic_add_typ(map.get_add_typ(x, y)) && map.get_add_idx(x, y) == job_idx) {
        return false;
    }
    if (adj_same_strategic(map, job_idx, x, y)) {
        return false;
    }
    return true;
}

static bool place_ok_resource (const RuntimeStatics& st, const GameArraySimple& map, u16 job_idx, u16 x, u16 y) {
    if (map.get_overlay(x, y) != OV_NONE[0]) {
        return false;
    }
    const u16 ri = map.get_res(x, y);
    if (ri == U16_KEY_NULL) {
        return false;
    }
    const ResourceStaticData& rs = st.resource();
    if (ri >= rs.get_item_count()) {
        return false;
    }
    if (rs.get_item(ResourceStaticDataKey::from_raw(ri)).worker_job_idx != job_idx) {
        return false;
    }
    const u8 terr = map.get_terrain(x, y);
    if (terr == TERR_MOUNTAINS[0] || terr == TERR_VOLCANO[0]) {
        return map.get_road_typ(x, y) != ROAD_NONE;
    }
    return true;
}

//================================================================================================================================
//=> - TileWorkAssessor (mk01) -
//================================================================================================================================

bool TileWorkAssessor::setup (const RuntimeStatics& st) {
    m_st = &st;
    return st.worker_job().get_item_count() > 0;
}

void TileWorkAssessor::bind_map (const GameArraySimple* map) {
    m_map = map;
}

void TileWorkAssessor::bind_ctx (const TileWorkCtx* ctx) {
    m_ctx = ctx;
}

bool TileWorkAssessor::in_bounds (u16 x, u16 y) {
    if (m_map == nullptr) {
        return false;
    }
    return x < m_map->width() && y < m_map->height();
}

bool TileWorkAssessor::tile_ok (u16 job_idx, u16 x, u16 y) {
    if (!in_bounds(x, y) || m_st == nullptr) {
        return false;
    }
    if (job_idx >= m_st->worker_job().get_item_count()) {
        return false;
    }
    const u16 typ = m_st->worker_job().get_item(WorkerJobStaticDataKey::from_raw(job_idx)).type;
    switch (static_cast<WorkerJobType>(typ)) {
        case WorkerJobType::Farm:
            return place_ok_farm(*m_map, job_idx, x, y);
        case WorkerJobType::Resource:
            return place_ok_resource(*m_st, *m_map, job_idx, x, y);
        case WorkerJobType::Road:
            return place_ok_road(*m_map, x, y);
        case WorkerJobType::Forest:
            return place_ok_forest(*m_map, x, y);
        case WorkerJobType::Clearing:
            return place_ok_clearing(*m_map, job_idx, x, y);
        case WorkerJobType::Strategic:
            return place_ok_strategic(*m_map, job_idx, x, y);
        default:
            return false;
    }
}

u16 TileWorkAssessor::assess_job (u16 x, u16 y, u16 job_idx, TileWorkCand* out, u16 out_cap) {
    if (out == nullptr || out_cap == 0 || m_st == nullptr) {
        return 0;
    }
    if (!tile_ok(job_idx, x, y)) {
        return 0;
    }
    const AssessorCtx ctx = make_ctx(m_ctx);
    if (!job_unlocked(*m_st, job_idx, ctx)) {
        return 0;
    }
    u16 n = 0;
    const WorkerJobImpIndex& ix = m_st->worker_job_imp_index();
    const u16 imp_n = ix.imp_n(job_idx);
    if (imp_n == 0) {
        push_cand(out, out_cap, &n, job_idx, U16_KEY_NULL);
        return n;
    }
    const u16* imps = ix.imps(job_idx);
    for (u16 i = 0; i < imp_n; ++i) {
        const u16 imp_idx = imps[i];
        if (!imp_unlocked(*m_st, imp_idx, ctx)) {
            continue;
        }
        if (!push_cand(out, out_cap, &n, job_idx, imp_idx)) {
            break;
        }
    }
    return n;
}

u16 TileWorkAssessor::assess (u16 x, u16 y, TileWorkCand* out, u16 out_cap) {
    if (out == nullptr || out_cap == 0 || m_st == nullptr) {
        return 0;
    }
    u16 n = 0;
    const u16 job_n = m_st->worker_job().get_item_count();
    TileWorkCand buf[32];
    for (u16 job_idx = 0; job_idx < job_n; ++job_idx) {
        const u16 got = assess_job(x, y, job_idx, buf, 32);
        for (u16 i = 0; i < got; ++i) {
            if (!push_cand(out, out_cap, &n, buf[i].m_job, buf[i].m_imp)) {
                return n;
            }
        }
    }
    return n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
