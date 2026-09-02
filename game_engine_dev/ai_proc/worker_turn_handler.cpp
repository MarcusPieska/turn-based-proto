//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "worker_turn_handler.h"
#include "assert_log.h"
#include "city.h"
#include "city_border.h"
#include "city_connector.h"
#include "city_tile_manager.h"
#include "circular_tile_areas.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "map_overlay_enum.h"
#include "overlay_yields.h"
#include "resource_static_data.h"
#include "resource_static_key.h"
#include "runtime_statics.h"
#include "tile_usage.h"
#include "tile_working.h"
#include "tile_yields.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"
#include "worker_build_progress.h"
#include "worker_guidance.h"
#include "worker_helper.h"
#include "worker_pathing.h"
#include "worker_job_enum.h"
#include "worker_job_static_key.h"
#include "worker_job_target_enum.h"
#include "worker_job_type_enum.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

WorkerTurnHandler::JobNoteFn WorkerTurnHandler::m_job_note = nullptr;

static bool wdest_none (const UnitAddStruct* unit) {
    return unit->m_delta_x_dest == static_cast<i8>(UNIT_DELTA_DEST_NONE)
        && unit->m_delta_y_dest == static_cast<i8>(UNIT_DELTA_DEST_NONE);
}

static bool wdest_arrived (const UnitAddStruct* unit) {
    return unit->m_delta_x_dest == static_cast<i8>(UNIT_DELTA_DEST_ARRIVED)
        && unit->m_delta_y_dest == static_cast<i8>(UNIT_DELTA_DEST_ARRIVED);
}

static void wdest_clr (UnitAddStruct* unit) {
    unit_add_clr_work_dest(unit);
}

static bool wdest_tile (const UnitAddStruct* unit, u16* x, u16* y) {
    if (wdest_none(unit)) {
        return false;
    }
    const i16 tx = static_cast<i16>(unit->m_x) + static_cast<i16>(unit->m_delta_x_dest);
    const i16 ty = static_cast<i16>(unit->m_y) + static_cast<i16>(unit->m_delta_y_dest);
    if (tx < 0 || ty < 0) {
        return false;
    }
    *x = static_cast<u16>(tx);
    *y = static_cast<u16>(ty);
    return true;
}

static bool wdest_set (UnitAddStruct* unit, u16 x, u16 y) {
    const i16 dx = static_cast<i16>(x) - static_cast<i16>(unit->m_x);
    const i16 dy = static_cast<i16>(y) - static_cast<i16>(unit->m_y);
    if (dx < -127 || dx > 127 || dy < -127 || dy > 127) {
        wdest_clr(unit);
        return false;
    }
    unit->m_delta_x_dest = static_cast<i8>(dx);
    unit->m_delta_y_dest = static_cast<i8>(dy);
    return true;
}

static void wdest_apply_step (UnitAddStruct* unit, i16 sx, i16 sy) {
    if (wdest_none(unit) || (sx == 0 && sy == 0)) {
        return;
    }
    unit->m_delta_x_dest = static_cast<i8>(static_cast<i16>(unit->m_delta_x_dest) - sx);
    unit->m_delta_y_dest = static_cast<i8>(static_cast<i16>(unit->m_delta_y_dest) - sy);
}

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_worker_typ (const GameState& state, u16 typ_idx) {
    const UnitStaticDataKey uk = UnitStaticDataKey::from_raw(typ_idx);
    return state.m_statics->unit().get_item(uk).type == state.m_land_worker_type_idx;
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

static bool home_city (const GameState& state, const UnitAddStruct* unit, u16* city_idx, u16* cx, u16* cy) {
    const u16 idx = WorkerHelper::get_data(unit);
    const City* c = state.m_cities.get_city(idx);
    if (c != nullptr && c->get_owner() == unit->m_player_idx) {
        *city_idx = idx;
        *cx = c->get_x();
        *cy = c->get_y();
        return true;
    }
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* city = state.m_cities.get_city(i);
        if (city == nullptr || city->get_owner() != unit->m_player_idx) {
            continue;
        }
        if (city->get_x() == unit->m_x && city->get_y() == unit->m_y) {
            *city_idx = i;
            *cx = city->get_x();
            *cy = city->get_y();
            return true;
        }
    }
    return false;
}

static bool tile_cand (
    GameState& state,
    u16 city_idx,
    u16 ux,
    u16 uy,
    TileAssignIntent* ointent,
    u16* ojob,
    u16* oimp)
{
    if (TileWorking::get_worker(ux, uy) != city_idx) {
        return false;
    }
    if (state.m_map.get_planned_city(ux, uy) != 0u) {
        return false;
    }
    const TileAssignIntent intent = static_cast<TileAssignIntent>(state.m_map.get_tile_usage(ux, uy));
    if (!WorkerGuidance::next_work(ux, uy, intent, ojob, oimp)) {
        return false;
    }
    *ointent = intent;
    return true;
}

static bool pick_active (
    GameState& state,
    u16 unit_idx,
    u16 city_idx,
    u16* ox,
    u16* oy,
    TileAssignIntent* ointent,
    u16* ojob,
    u16* oimp)
{
    UnitAddStruct* unit = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    if (unit == nullptr) {
        return false;
    }
    u16 x = 0;
    u16 y = 0;
    if (!wdest_tile(unit, &x, &y)) {
        return false;
    }
    if (state.m_map.get_planned_city(x, y) != 0u) {
        wdest_clr(unit);
        return false;
    }
    if (tile_cand(state, city_idx, x, y, ointent, ojob, oimp)) {
        *ox = x;
        *oy = y;
        return true;
    }
    const TileAssignIntent intent = static_cast<TileAssignIntent>(state.m_map.get_tile_usage(x, y));
    u16 job = U16_KEY_NULL;
    u16 imp = U16_KEY_NULL;
    if (!WorkerGuidance::next_work(x, y, intent, &job, &imp)) {
        wdest_clr(unit);
        return false;
    }
    *ox = x;
    *oy = y;
    *ointent = intent;
    *ojob = job;
    *oimp = imp;
    return true;
}

static bool tile_has_work (GameState& state, u16 city_idx, u16 ux, u16 uy) {
    u16 job = U16_KEY_NULL;
    u16 imp = U16_KEY_NULL;
    TileAssignIntent intent = TILE_ASSIGN_FOOD;
    if (state.m_map.get_planned_city(ux, uy) != 0u) {
        return false;
    }
    if (tile_cand(state, city_idx, ux, uy, &intent, &job, &imp)) {
        return true;
    }
    intent = static_cast<TileAssignIntent>(state.m_map.get_tile_usage(ux, uy));
    return WorkerGuidance::next_work(ux, uy, intent, &job, &imp);
}

static bool disk_tile_imp_pending (GameState& state, u16 ux, u16 uy) {
    if (state.m_map.get_planned_city(ux, uy) != 0u) {
        return false;
    }
    const TileAssignIntent intent = static_cast<TileAssignIntent>(state.m_map.get_tile_usage(ux, uy));
    return WorkerGuidance::has_pending_work(ux, uy, intent);
}

static bool disk_tile_pending (GameState& state, u16 ux, u16 uy) {
    if (CityConnector::has_virtual_at(state, ux, uy)) {
        return true;
    }
    return disk_tile_imp_pending(state, ux, uy);
}

static bool local_imp_fully_built (GameState& state, u16 city_idx, u16 cx, u16 cy) {
    const CircArea area = CityTileManager::work_area();
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (ux >= state.m_map.width() || uy >= state.m_map.height()) {
            continue;
        }
        if (TileWorking::get_worker(ux, uy) != city_idx) {
            continue;
        }
        if (disk_tile_imp_pending(state, ux, uy)) {
            return false;
        }
    }
    return true;
}

static bool local_is_fully_built (GameState& state, u16 city_idx, u16 cx, u16 cy) {
    const CircArea area = CityTileManager::work_area();
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (ux >= state.m_map.width() || uy >= state.m_map.height()) {
            continue;
        }
        if (TileWorking::get_worker(ux, uy) != city_idx) {
            continue;
        }
        if (state.m_map.get_planned_city(ux, uy) != 0u) {
            continue;
        }
        if (disk_tile_pending(state, ux, uy)) {
            return false;
        }
    }
    return true;
}

static bool res_needs_overlay (const GameState& state, u16 x, u16 y, u16 rj) {
    if (state.m_statics == nullptr || rj == U16_KEY_NULL) {
        return false;
    }
    const u16 job_ov = state.m_statics->worker_job().get_item(WorkerJobStaticDataKey::from_raw(rj)).target_idx;
    return state.m_map.get_overlay(x, y) != job_ov;
}

static bool fort_territory_ok (const GameState& state, u16 city_idx, u16 ux, u16 uy) {
    const City* city = state.m_cities.get_city(city_idx);
    if (city == nullptr) {
        return false;
    }
    const u16 player = city->get_owner();
    if (player == U16_KEY_NULL) {
        return false;
    }
    return state.m_map.get_civ_owner(ux, uy) == static_cast<u8>(player);
}

static void claim_fort_disc (GameState& state, u16 city_idx, u16 x, u16 y) {
    const City* city = state.m_cities.get_city(city_idx);
    if (city == nullptr || city->get_owner() == U16_KEY_NULL) {
        return;
    }
    CityBorder::claim_disc(x, y, 3u, static_cast<u8>(city->get_owner()));
}

static bool fort_tile_cand (
    GameState& state,
    u16 city_idx,
    u16 ux,
    u16 uy,
    TileAssignIntent* ointent,
    u16* ojob,
    u16* oimp)
{
    if (!fort_territory_ok(state, city_idx, ux, uy)) {
        return false;
    }
    if (state.m_map.get_planned_city(ux, uy) != 0u) {
        return false;
    }
    const TileAssignIntent intent = static_cast<TileAssignIntent>(state.m_map.get_tile_usage(ux, uy));
    if (!WorkerGuidance::next_work(ux, uy, intent, ojob, oimp)) {
        return false;
    }
    if (*ojob != static_cast<u16>(WorkerJob::Build_Fort)) {
        return false;
    }
    *ointent = intent;
    return true;
}

static bool pick_fort (
    GameState& state,
    u16 city_idx,
    u16 cx,
    u16 cy,
    u16* ox,
    u16* oy,
    TileAssignIntent* ointent,
    u16* ojob,
    u16* oimp)
{
    const CircArea area = CityTileManager::work_area();
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (ux >= state.m_map.width() || uy >= state.m_map.height()) {
            continue;
        }
        if (state.m_map.get_ai_ov_intent(ux, uy) != AI_TILE_OV_INTENT_FORT) {
            continue;
        }
        TileAssignIntent intent = TILE_ASSIGN_FOOD;
        if (!fort_tile_cand(state, city_idx, ux, uy, &intent, ojob, oimp)) {
            continue;
        }
        *ox = ux;
        *oy = uy;
        *ointent = intent;
        return true;
    }
    return false;
}

static bool mtn_pass_needs_road (const GameState& state, u16 city_idx, u16 x, u16 y) {
    if (!fort_territory_ok(state, city_idx, x, y)) {
        return false;
    }
    const u8 iv = state.m_map.get_ai_ov_intent(x, y);
    if (iv == AI_TILE_OV_INTENT_MTN_PASS) {
        return !road_is_built(state.m_map.get_road_typ(x, y));
    }
    if (iv != AI_TILE_OV_INTENT_FORT) {
        return false;
    }
    if (road_is_built(state.m_map.get_road_typ(x, y))) {
        return false;
    }
    const u8 terr = state.m_map.get_terrain(x, y);
    return terr == TERR_MOUNTAINS[0] || terr == TERR_VOLCANO[0];
}

static bool try_fort_after_dirt (GameState& state, u16 city_idx, u16 x, u16 y) {
    if (state.m_map.get_ai_ov_intent(x, y) != AI_TILE_OV_INTENT_FORT) {
        return false;
    }
    if (!fort_territory_ok(state, city_idx, x, y)) {
        return false;
    }
    if (!WorkerGuidance::apply_work(x, y, static_cast<u16>(WorkerJob::Build_Fort), U16_KEY_NULL)) {
        return false;
    }
    claim_fort_disc(state, city_idx, x, y);
    return true;
}

static bool pick_city_jobs (
    GameState& state,
    u16 city_idx,
    u16 cx,
    u16 cy,
    u16* ox,
    u16* oy,
    TileAssignIntent* ointent,
    u16* ojob,
    u16* oimp)
{
    if (!state.m_ai_help.ok()) {
        return false;
    }
    const WorkerCityJobs& jobs = state.m_ai_help.jobs();
    if (!jobs.ok()) {
        return false;
    }
    u32 six = 0;
    if (!jobs.find_site(cx, cy, &six)) {
        return false;
    }
    const u16 n = jobs.job_n(six);
    for (u16 i = 0; i < n; ++i) {
        u16 ux = 0;
        u16 uy = 0;
        if (!jobs.job_at(six, i, &ux, &uy)) {
            continue;
        }
        if (mtn_pass_needs_road(state, city_idx, ux, uy)) {
            *ox = ux;
            *oy = uy;
            *ojob = static_cast<u16>(WorkerJob::Build_Dirt_Path);
            *oimp = U16_KEY_NULL;
            *ointent = TILE_ASSIGN_FOOD;
            return true;
        }
        TileAssignIntent intent = TILE_ASSIGN_FOOD;
        u16 job = U16_KEY_NULL;
        u16 imp = U16_KEY_NULL;
        if (fort_tile_cand(state, city_idx, ux, uy, &intent, &job, &imp)) {
            *ox = ux;
            *oy = uy;
            *ointent = intent;
            *ojob = job;
            *oimp = imp;
            return true;
        }
    }
    return false;
}

static bool pick_resource (
    GameState& state,
    u16 city_idx,
    u16 cx,
    u16 cy,
    u16* ox,
    u16* oy,
    TileAssignIntent* ointent,
    u16* ojob,
    u16* oimp)
{
    const CircArea area = CityTileManager::work_area();
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (ux >= state.m_map.width() || uy >= state.m_map.height()) {
            continue;
        }
        const u16 rj = res_job_idx(state.m_statics, state.m_map.get_res(ux, uy));
        if (rj == U16_KEY_NULL || !res_needs_overlay(state, ux, uy, rj)) {
            continue;
        }
        u16 job = U16_KEY_NULL;
        u16 imp = U16_KEY_NULL;
        TileAssignIntent intent = TILE_ASSIGN_FOOD;
        if (!tile_cand(state, city_idx, ux, uy, &intent, &job, &imp)) {
            continue;
        }
        *ox = ux;
        *oy = uy;
        *ointent = intent;
        *ojob = job;
        *oimp = imp;
        return true;
    }
    return false;
}

static bool pick_virtual_disk (
    GameState& state,
    u16 city_idx,
    u16 cx,
    u16 cy,
    u16* ox,
    u16* oy)
{
    const CircArea area = CityTileManager::work_area();
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (ux >= state.m_map.width() || uy >= state.m_map.height()) {
            continue;
        }
        if (TileWorking::get_worker(ux, uy) != city_idx) {
            continue;
        }
        if (!CityConnector::has_virtual_at(state, ux, uy)) {
            continue;
        }
        if (disk_tile_imp_pending(state, ux, uy)) {
            continue;
        }
        *ox = ux;
        *oy = uy;
        return true;
    }
    return false;
}

static bool pick_first (
    GameState& state,
    u16 city_idx,
    u16 cx,
    u16 cy,
    u16* ox,
    u16* oy,
    TileAssignIntent* ointent,
    u16* ojob,
    u16* oimp)
{
    const CircArea area = CityTileManager::work_area();
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (ux >= state.m_map.width() || uy >= state.m_map.height()) {
            continue;
        }
        u16 job = U16_KEY_NULL;
        u16 imp = U16_KEY_NULL;
        TileAssignIntent intent = TILE_ASSIGN_FOOD;
        if (!tile_cand(state, city_idx, ux, uy, &intent, &job, &imp)) {
            continue;
        }
        *ox = ux;
        *oy = uy;
        *ointent = intent;
        *ojob = job;
        *oimp = imp;
        return true;
    }
    return false;
}

static OvYldTot pick_tot_for_job (const RuntimeStatics* st, u16 job, TileAssignIntent intent) {
    OvYldTot z = {};
    if (st == nullptr || job >= st->worker_job().get_item_count()) {
        return z;
    }
    const WorkerJobStaticDataStruct& row = st->worker_job().get_item(WorkerJobStaticDataKey::from_raw(job));
    u16 ov = U16_KEY_NULL;
    if (row.target_kind == static_cast<u16>(WorkerJobTarget::Overlay)) {
        ov = row.target_idx;
    } else if (intent == TILE_ASSIGN_FOOD) {
        ov = static_cast<u16>(MapOverlay::Farm);
    } else {
        return z;
    }
    return OverlayYields::tot(ov);
}

static bool pick_best (
    GameState& state,
    u16 city_idx,
    u16 cx,
    u16 cy,
    u16* ox,
    u16* oy,
    TileAssignIntent* ointent,
    u16* ojob,
    u16* oimp)
{
    const CircArea area = CityTileManager::work_area();
    u8 have_food = 0;
    u8 have_prod = 0;
    u16 best_food = 0;
    u16 best_prod = 0;
    u16 fx = 0;
    u16 fy = 0;
    u16 fjob = U16_KEY_NULL;
    u16 fimp = U16_KEY_NULL;
    u16 px = 0;
    u16 py = 0;
    u16 pjob = U16_KEY_NULL;
    u16 pimp = U16_KEY_NULL;
    TileAssignIntent fintent = TILE_ASSIGN_FOOD;
    TileAssignIntent pintent = TILE_ASSIGN_PROD;
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (ux >= state.m_map.width() || uy >= state.m_map.height()) {
            continue;
        }
        TileAssignIntent intent = TILE_ASSIGN_FOOD;
        u16 job = U16_KEY_NULL;
        u16 imp = U16_KEY_NULL;
        if (!tile_cand(state, city_idx, ux, uy, &intent, &job, &imp)) {
            continue;
        }
        if (intent == TILE_ASSIGN_FOOD) {
            const OvYldTot yt = pick_tot_for_job(state.m_statics, job, intent);
            const u16 score = static_cast<u16>(yt.m_food > 0 ? yt.m_food : 0);
            if (have_food == 0 || score > best_food) {
                best_food = score;
                fx = ux;
                fy = uy;
                fjob = job;
                fimp = imp;
                fintent = intent;
                have_food = 1;
            }
        } else {
            const OvYldTot yt = pick_tot_for_job(state.m_statics, job, intent);
            const u16 score = static_cast<u16>(yt.m_prod > 0 ? yt.m_prod : 0);
            if (have_prod == 0 || score > best_prod) {
                best_prod = score;
                px = ux;
                py = uy;
                pjob = job;
                pimp = imp;
                pintent = intent;
                have_prod = 1;
            }
        }
    }
    if (have_food != 0) {
        *ox = fx;
        *oy = fy;
        *ointent = fintent;
        *ojob = fjob;
        *oimp = fimp;
        return true;
    }
    if (have_prod != 0) {
        *ox = px;
        *oy = py;
        *ointent = pintent;
        *ojob = pjob;
        *oimp = pimp;
        return true;
    }
    return false;
}

static void reassign (GameState& state, u16 x, u16 y, u16 fallback_city) {
    u16 city_idx = TileWorking::get_worker(x, y);
    if (city_idx == U16_KEY_NULL) {
        city_idx = fallback_city;
    }
    City* city = state.m_cities.get_city(city_idx);
    if (city == nullptr) {
        return;
    }
    const u16 player = city->get_owner();
    if (player == U16_KEY_NULL || player >= state.m_player_n) {
        return;
    }
    const u16 start_food = TileYields::get(city->get_x(), city->get_y()).m_food;
    const u16 sanit = city->get_city_sanitation_boost(city_idx);
    CityTileManager::stable_food_max_production(player, city_idx, start_food, sanit);
}

static bool apply_one (
    GameState& state,
    UnitAddStruct* unit,
    PlayerState& ps,
    u16 city_idx,
    u16 x,
    u16 y,
    u16 job,
    u16 imp)
{
    if (!WorkerGuidance::apply_work(x, y, job, imp)) {
        return false;
    }
    const u32 cost = WorkerBuildProgress::work_cost(*state.m_statics, job, imp);
    const i16 deficit = WorkerBuildProgress::mvt_deficit(*state.m_statics, ps.m_worker_mvt_to_build_perc, cost);
    WorkerBuildProgress::apply_deficit(unit, deficit);
    if (ps.m_worker_tile_opt_reassign != 0) {
        reassign(state, x, y, city_idx);
    }
    return true;
}

//================================================================================================================================
//=> - WorkerTurnHandler -
//================================================================================================================================

void WorkerTurnHandler::set_job_note (JobNoteFn fn) {
    m_job_note = fn;
}

void WorkerTurnHandler::clear_work_tgt (GameState& state, u16 unit_idx) {
    UnitAddStruct* unit = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    if (unit != nullptr) {
        wdest_clr(unit);
    }
}

void WorkerTurnHandler::handle (GameState& state, u16 unit_idx) {
    GAME_EXPECT(state.m_player_states != nullptr, "WorkerTurnHandler got nullptr player states");
    GAME_EXPECT(state.m_statics != nullptr, "WorkerTurnHandler got nullptr statics");
    UnitAddStruct* unit = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    GAME_EXPECT(unit != nullptr, "WorkerTurnHandler got nullptr unit");
    GAME_EXPECT(unit->m_x != U16_KEY_NULL, "WorkerTurnHandler unit has null x");
    if (!is_worker_typ(state, unit->m_unit_typ_idx)) {
        return;
    }
    const u16 player = unit->m_player_idx;
    GAME_EXPECT(player < state.m_player_n, "WorkerTurnHandler player out of bounds");
    PlayerState& ps = state.m_player_states[player];
    ps.m_last_turn_worker_count = static_cast<u16>(ps.m_last_turn_worker_count + 1u);
    if (!WorkerBuildProgress::can_start(unit)) {
        return;
    }

    u16 city_idx = 0;
    u16 cx = 0;
    u16 cy = 0;
    if (!home_city(state, unit, &city_idx, &cx, &cy)) {
        return;
    }
    City* city = state.m_cities.get_city(city_idx);
    GAME_EXPECT(city != nullptr, "WorkerTurnHandler home city null");
    u16 x = 0;
    u16 y = 0;
    TileAssignIntent intent = TILE_ASSIGN_FOOD;
    u16 job = U16_KEY_NULL;
    u16 imp = U16_KEY_NULL;
    bool found = pick_active(state, unit_idx, city_idx, &x, &y, &intent, &job, &imp);
    if (!found && !city->city_has_worker()) {
        wdest_clr(unit);
        return;
    }
    bool fully = false;
    bool know_fully = false;
    bool imp_fully = false;
    bool know_imp_fully = false;
    if (!found) {
        found = pick_resource(state, city_idx, cx, cy, &x, &y, &intent, &job, &imp);
    }
    if (!found) {
        found = pick_fort(state, city_idx, cx, cy, &x, &y, &intent, &job, &imp);
    }
    if (!found) {
        imp_fully = local_imp_fully_built(state, city_idx, cx, cy);
        know_imp_fully = true;
        fully = local_is_fully_built(state, city_idx, cx, cy);
        know_fully = true;
        if (!imp_fully) {
            found = pick_first(state, city_idx, cx, cy, &x, &y, &intent, &job, &imp);
        }
    }
    if (!found && ps.m_worker_tile_opt_scan != 0 && imp_fully) {
        found = pick_best(state, city_idx, cx, cy, &x, &y, &intent, &job, &imp);
    }
    if (!found && ps.m_worker_tile_opt_scan == 0 && !imp_fully) {
        found = pick_first(state, city_idx, cx, cy, &x, &y, &intent, &job, &imp);
    }
    if (!found) {
        if (!know_imp_fully) {
            imp_fully = local_imp_fully_built(state, city_idx, cx, cy);
        }
        if (imp_fully) {
            found = pick_virtual_disk(state, city_idx, cx, cy, &x, &y);
        }
    }
    if (!found) {
        if (!know_fully) {
            fully = local_is_fully_built(state, city_idx, cx, cy);
        }
        CityConnector::clear_idle_flag(state, city_idx, city, cx, cy, fully);
        wdest_clr(unit);
        if (CityConnector::on_road_tile(state, unit->m_x, unit->m_y)) {
            if (CityConnector::handle(state, unit_idx)) {
                return;
            }
        }
        if (pick_city_jobs(state, city_idx, cx, cy, &x, &y, &intent, &job, &imp)) {
            found = true;
        } else {
            return;
        }
    }
    if (CityConnector::has_virtual_at(state, x, y)) {
        wdest_clr(unit);
        if (!CityConnector::handle(state, unit_idx)) {
            CityConnector::step_toward(state, unit_idx, x, y);
            if (CityConnector::has_virtual_at(state, unit->m_x, unit->m_y)) {
                CityConnector::handle(state, unit_idx);
            }
        }
        return;
    }
    wdest_set(unit, x, y);
    if (state.m_path_worker != 0) {
        if (!wdest_arrived(unit)) {
            const u16 ox = unit->m_x;
            const u16 oy = unit->m_y;
            WorkerPathing::step_toward(state, unit_idx, x, y);
            const i16 sx = static_cast<i16>(unit->m_x) - static_cast<i16>(ox);
            const i16 sy = static_cast<i16>(unit->m_y) - static_cast<i16>(oy);
            wdest_apply_step(unit, sx, sy);
            if (!wdest_arrived(unit)) {
                return;
            }
        }
    }
    if (apply_one(state, unit, ps, city_idx, x, y, job, imp)) {
        if (m_job_note != nullptr) {
            m_job_note(x, y, job, imp, static_cast<u8>(intent));
        }
        if (job == static_cast<u16>(WorkerJob::Build_Fort)) {
            claim_fort_disc(state, city_idx, x, y);
        }
        if (job == static_cast<u16>(WorkerJob::Build_Dirt_Path)) {
            if (try_fort_after_dirt(state, city_idx, x, y) && m_job_note != nullptr) {
                m_job_note(x, y, static_cast<u16>(WorkerJob::Build_Fort), U16_KEY_NULL,
                    static_cast<u8>(TILE_ASSIGN_FOOD));
            }
            wdest_clr(unit);
        } else if (!tile_has_work(state, city_idx, x, y)) {
            wdest_clr(unit);
        }
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
