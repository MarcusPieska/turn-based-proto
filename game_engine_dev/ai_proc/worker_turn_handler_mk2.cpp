//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "worker_turn_handler_mk2.h"

#include "assert_log.h"
#include "city.h"
#include "city_tile_manager.h"
#include "circular_tile_areas.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "general_assessor.h"
#include "map_overlay_enum.h"
#include "overlay_yields.h"
#include "resource_static_data.h"
#include "resource_static_key.h"
#include "runtime_statics.h"
#include "tile_imp_helper.h"
#include "tile_usage.h"
#include "tile_work_assessor.h"
#include "tile_working.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"
#include "worker_build_progress.h"
#include "worker_guidance.h"
#include "worker_helper.h"
#include "worker_job_enum.h"
#include "worker_job_imp_index.h"
#include "worker_job_imp_static_data.h"
#include "worker_job_imp_static_key.h"
#include "worker_job_static_data.h"
#include "worker_job_static_key.h"
#include "worker_job_target_enum.h"
#include "worker_job_type_enum.h"
#include "worker_pathing.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

WorkerTurnHandlerMk2::JobNoteFn WorkerTurnHandlerMk2::m_job_note = nullptr;

static bool stamp_disk_cand (const GameState& state, u16 city_idx, u16 ux, u16 uy);

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void wdest_clr (UnitAddStruct* unit) {
    unit_add_clr_work_dest(unit);
}

static bool wdest_none (const UnitAddStruct* unit) {
    return unit->m_delta_x_dest == static_cast<i8>(UNIT_DELTA_DEST_NONE)
        && unit->m_delta_y_dest == static_cast<i8>(UNIT_DELTA_DEST_NONE);
}

static bool wdest_arrived (const UnitAddStruct* unit) {
    return unit->m_delta_x_dest == static_cast<i8>(UNIT_DELTA_DEST_ARRIVED)
        && unit->m_delta_y_dest == static_cast<i8>(UNIT_DELTA_DEST_ARRIVED);
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

static bool is_worker_typ (const GameState& state, u16 typ_idx) {
    const UnitStaticDataKey uk = UnitStaticDataKey::from_raw(typ_idx);
    return state.m_statics->unit().get_item(uk).type == state.m_land_worker_type_idx;
}

static bool base_city (const GameState& state, const UnitAddStruct* unit, u16* city_idx, u16* cx, u16* cy) {
    const u16 idx = WorkerHelper::get_data(unit);
    const City* c = state.m_cities.get_city(idx);
    if (c == nullptr || c->get_owner() != unit->m_player_idx) {
        return false;
    }
    *city_idx = idx;
    *cx = c->get_x();
    *cy = c->get_y();
    return true;
}

static bool tile_pending (GameState& state, u16 city_idx, u16 ux, u16 uy) {
    if (state.m_map.get_planned_city(ux, uy) != 0u) {
        return false;
    }
    if (TileWorking::get_worker(ux, uy) != city_idx && !stamp_disk_cand(state, city_idx, ux, uy)) {
        return false;
    }
    const TileAssignIntent intent = static_cast<TileAssignIntent>(state.m_map.get_tile_usage(ux, uy));
    return WorkerGuidance::has_pending_work(ux, uy, intent);
}

static bool pick_marked (
    GameState& state,
    u16 city_idx,
    u16 wx,
    u16 wy,
    u16* ox,
    u16* oy)
{
    const CircArea area = CityTileManager::work_area();
    const City* city = state.m_cities.get_city(city_idx);
    if (city == nullptr) {
        return false;
    }
    const u16 cx = city->get_x();
    const u16 cy = city->get_y();
    u32 best = 0xffffffffu;
    u16 bx = 0;
    u16 by = 0;
    bool hit = false;
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
        if (state.m_map.get_tile_work_needed(ux, uy) == 0u) {
            continue;
        }
        if (TileWorking::get_worker(ux, uy) != city_idx
            && !stamp_disk_cand(state, city_idx, ux, uy)) {
            continue;
        }
        const i32 dx = static_cast<i32>(ux) - static_cast<i32>(wx);
        const i32 dy = static_cast<i32>(uy) - static_cast<i32>(wy);
        const u32 adx = static_cast<u32>(dx < 0 ? -dx : dx);
        const u32 ady = static_cast<u32>(dy < 0 ? -dy : dy);
        const u32 d = adx > ady ? adx : ady;
        if (d < best) {
            best = d;
            bx = ux;
            by = uy;
            hit = true;
        }
    }
    if (!hit) {
        return false;
    }
    *ox = bx;
    *oy = by;
    return true;
}

static u16 pick_conn_busy (GameState& state, u16 home_idx) {
    City* home = state.m_cities.get_city(home_idx);
    if (home == nullptr) {
        return U16_KEY_NULL;
    }
    u16 best = U16_KEY_NULL;
    u8 best_n = 0;
    for (u8 d = 0; d < 4u; ++d) {
        const u16 j = home->get_conn_city(d);
        if (j == U16_KEY_NULL) {
            continue;
        }
        City* c = state.m_cities.get_city(j);
        if (c == nullptr) {
            continue;
        }
        const u8 n = c->get_tile_imp_count();
        if (n > best_n) {
            best_n = n;
            best = j;
        }
    }
    return best;
}

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

static AssessorCtx make_city_ctx (GameState& state, u16 city_idx, u16 x, u16 y) {
    AssessorCtx ctx = {};
    City* c = state.m_cities.get_city(city_idx);
    if (c != nullptr && c->get_owner() < state.m_player_n) {
        ctx.m_tech = state.m_player_states[c->get_owner()].m_techs_researched;
    }
    ctx.m_map = &state.m_map;
    ctx.m_x = x;
    ctx.m_y = y;
    return ctx;
}

static u16 count_imps_unlocked (
    const RuntimeStatics& st,
    const GameArraySimple& map,
    u16 x,
    u16 y,
    u16 ov,
    const AssessorCtx& ctx)
{
    const WorkerJobImpIndex& ix = st.worker_job_imp_index();
    const u16 imp_n = ix.imp_n(ov);
    const u16* imps = ix.imps(ov);
    if (imps == nullptr || imp_n == 0u) {
        return 0;
    }
    u16 n = 0;
    const GameTileSimple* t = map.tile(x, y);
    for (u16 i = 0; i < imp_n; ++i) {
        const u16 imp = imps[i];
        if (imp >= st.worker_job_imp().get_item_count()) {
            continue;
        }
        const WorkerJobImpStaticDataStruct& row =
            st.worker_job_imp().get_item(WorkerJobImpStaticDataKey::from_raw(imp));
        if (!GeneralAssessor::chk(row.reqs, ctx)) {
            continue;
        }
        if (t != nullptr && TileImpHelper::has_imp(t, st, imp)) {
            continue;
        }
        n = static_cast<u16>(n + 1u);
    }
    return n;
}

static u16 count_mother_chain (GameState& state, u16 city_idx, u16 x, u16 y, u16 mj, u16 ov) {
    TileWorkCand buf[32];
    const u16 n = TileWorkAssessor::assess_job(x, y, mj, buf, 32);
    if (n == 0u) {
        return 0;
    }
    if (state.m_map.get_overlay(x, y) == ov) {
        return n;
    }
    const AssessorCtx ctx = make_city_ctx(state, city_idx, x, y);
    return static_cast<u16>(n + count_imps_unlocked(*state.m_statics, state.m_map, x, y, ov, ctx));
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

static bool dirt_needed (const GameArraySimple& map, u16 x, u16 y) {
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

static bool stamp_disk_cand (const GameState& state, u16 city_idx, u16 ux, u16 uy) {
    if (!fort_territory_ok(state, city_idx, ux, uy)) {
        return false;
    }
    if (road_is_virtual(state.m_map.get_road_typ(ux, uy))) {
        return true;
    }
    const u8 iv = state.m_map.get_ai_ov_intent(ux, uy);
    return iv == AI_TILE_OV_INTENT_FORT || iv == AI_TILE_OV_INTENT_MTN_PASS;
}

static u16 count_tile_needed (GameState& state, u16 city_idx, u16 x, u16 y) {
    const RuntimeStatics* st = state.m_statics;
    GameArraySimple& map = state.m_map;
    const u8 iv = map.get_ai_ov_intent(x, y);
    u16 n = 0;
    if (dirt_needed(map, x, y)) {
        n = static_cast<u16>(n + 1u);
    }
    if (iv == AI_TILE_OV_INTENT_FORT) {
        const u16 fj = static_cast<u16>(WorkerJob::Build_Fort);
        n = static_cast<u16>(n + count_mother_chain(
            state, city_idx, x, y, fj, static_cast<u16>(MapOverlay::Fort)));
        return n;
    }
    if (iv == AI_TILE_OV_INTENT_MTN_PASS) {
        return n;
    }

    const TileAssignIntent intent = static_cast<TileAssignIntent>(map.get_tile_usage(x, y));
    const u16 rj = res_job_idx(st, map.get_res(x, y));
    if (rj != U16_KEY_NULL) {
        const u16 cj = clear_job_for_ov(map.get_overlay(x, y));
        if (cj != U16_KEY_NULL && TileWorkAssessor::tile_ok(cj, x, y)) {
            return static_cast<u16>(n + 1u);
        }
        const u16 job_ov = st->worker_job().get_item(WorkerJobStaticDataKey::from_raw(rj)).target_idx;
        return static_cast<u16>(n + count_mother_chain(state, city_idx, x, y, rj, job_ov));
    }

    if (intent == TILE_ASSIGN_FOOD) {
        const u16 cj = clear_job_for_ov(map.get_overlay(x, y));
        if (cj != U16_KEY_NULL && TileWorkAssessor::tile_ok(cj, x, y)) {
            return static_cast<u16>(n + 1u);
        }
    }

    u16 rn = 0;
    const u16* ranks = OverlayYields::rank(intent, &rn);
    if (ranks == nullptr) {
        return n;
    }
    for (u16 i = 0; i < rn; ++i) {
        const u16 mj = mother_job_for_ov(st, ranks[i]);
        if (mj == U16_KEY_NULL) {
            continue;
        }
        const u16 c = count_mother_chain(state, city_idx, x, y, mj, ranks[i]);
        if (c != 0u) {
            return static_cast<u16>(n + c);
        }
    }
    return n;
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
    City* city = state.m_cities.get_city(city_idx);
    if (city != nullptr) {
        city->add_tile_imp_count(-1);
    }
    if (!tile_pending(state, city_idx, x, y)) {
        state.m_map.set_tile_work_needed(x, y, 0u);
    }
    return true;
}

//================================================================================================================================
//=> - WorkerTurnHandlerMk2 -
//================================================================================================================================

void WorkerTurnHandlerMk2::set_job_note (JobNoteFn fn) {
    m_job_note = fn;
}

void WorkerTurnHandlerMk2::clear_work_tgt (GameState& state, u16 unit_idx) {
    UnitAddStruct* unit = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    if (unit != nullptr) {
        wdest_clr(unit);
    }
}

u32 WorkerTurnHandlerMk2::assess (GameState& state, u16 city_idx) {
    City* city = state.m_cities.get_city(city_idx);
    if (city == nullptr) {
        return 0;
    }
    static TileWorkCtx s_wctx;
    s_wctx.m_tech = nullptr;
    s_wctx.m_resource = nullptr;
    if (city->get_owner() < state.m_player_n) {
        s_wctx.m_tech = state.m_player_states[city->get_owner()].m_techs_researched;
    }
    TileWorkAssessor::bind_ctx(&s_wctx);

    const u16 cx = city->get_x();
    const u16 cy = city->get_y();
    const CircArea area = CityTileManager::work_area();
    u32 n = 0;
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
        if (TileWorking::get_worker(ux, uy) != city_idx
            && !stamp_disk_cand(state, city_idx, ux, uy)) {
            state.m_map.set_tile_work_needed(ux, uy, 0u);
            continue;
        }
        if (state.m_map.get_planned_city(ux, uy) != 0u) {
            state.m_map.set_tile_work_needed(ux, uy, 0u);
            continue;
        }
        const u16 jobs = count_tile_needed(state, city_idx, ux, uy);
        if (jobs != 0u) {
            state.m_map.set_tile_work_needed(ux, uy, 1u);
            n = n + static_cast<u32>(jobs);
        } else {
            state.m_map.set_tile_work_needed(ux, uy, 0u);
        }
    }
    city->set_tile_imp_count(n > 255u ? 255u : static_cast<u8>(n));
    return n;
}

void WorkerTurnHandlerMk2::handle (GameState& state, u16 unit_idx) {
    GAME_EXPECT(state.m_player_states != nullptr, "WorkerTurnHandlerMk2 got nullptr player states");
    GAME_EXPECT(state.m_statics != nullptr, "WorkerTurnHandlerMk2 got nullptr statics");
    UnitAddStruct* unit = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    GAME_EXPECT(unit != nullptr, "WorkerTurnHandlerMk2 got nullptr unit");
    GAME_EXPECT(unit->m_x != U16_KEY_NULL, "WorkerTurnHandlerMk2 unit has null x");
    if (!is_worker_typ(state, unit->m_unit_typ_idx)) {
        return;
    }
    const u16 player = unit->m_player_idx;
    GAME_EXPECT(player < state.m_player_n, "WorkerTurnHandlerMk2 player out of bounds");
    PlayerState& ps = state.m_player_states[player];
    ps.m_last_turn_worker_count = static_cast<u16>(ps.m_last_turn_worker_count + 1u);
    if (!WorkerBuildProgress::can_start(unit)) {
        return;
    }

    u16 base_idx = 0;
    u16 bx = 0;
    u16 by = 0;
    if (!base_city(state, unit, &base_idx, &bx, &by)) {
        return;
    }
    City* base = state.m_cities.get_city(base_idx);
    GAME_EXPECT(base != nullptr, "WorkerTurnHandlerMk2 base city null");

    u16 cur_idx = base_idx;
    if (base->get_tile_imp_count() == 0u) {
        const u16 alt = pick_conn_busy(state, base_idx);
        if (alt != U16_KEY_NULL) {
            cur_idx = alt;
        }
    }
    City* cur = state.m_cities.get_city(cur_idx);
    if (cur == nullptr) {
        return;
    }
    if (cur_idx != base_idx && cur->get_tile_imp_count() == 0u) {
        if (base->get_tile_imp_count() != 0u) {
            cur_idx = base_idx;
            cur = base;
        } else {
            const u16 alt = pick_conn_busy(state, cur_idx);
            if (alt != U16_KEY_NULL) {
                cur_idx = alt;
                cur = state.m_cities.get_city(cur_idx);
                if (cur == nullptr) {
                    return;
                }
            }
        }
    }
    if (cur->get_tile_imp_count() == 0u) {
        if (!tile_pending(state, cur_idx, unit->m_x, unit->m_y)) {
            wdest_clr(unit);
            return;
        }
    }

    u16 x = 0;
    u16 y = 0;
    TileAssignIntent intent = TILE_ASSIGN_FOOD;
    u16 job = U16_KEY_NULL;
    u16 imp = U16_KEY_NULL;
    bool have = false;
    if (wdest_tile(unit, &x, &y)) {
        if (state.m_map.get_tile_work_needed(x, y) != 0u
            && (TileWorking::get_worker(x, y) == cur_idx || stamp_disk_cand(state, cur_idx, x, y))
            && WorkerGuidance::next_work(x, y, static_cast<TileAssignIntent>(state.m_map.get_tile_usage(x, y)), &job, &imp)) {
            intent = static_cast<TileAssignIntent>(state.m_map.get_tile_usage(x, y));
            have = true;
        } else {
            wdest_clr(unit);
        }
    }
    if (!have) {
        x = unit->m_x;
        y = unit->m_y;
        intent = static_cast<TileAssignIntent>(state.m_map.get_tile_usage(x, y));
        if (tile_pending(state, cur_idx, x, y)
            && WorkerGuidance::next_work(x, y, intent, &job, &imp)) {
            have = true;
        }
    }
    if (!have) {
        if (!pick_marked(state, cur_idx, unit->m_x, unit->m_y, &x, &y)) {
            wdest_clr(unit);
            return;
        }
        intent = static_cast<TileAssignIntent>(state.m_map.get_tile_usage(x, y));
        if (!WorkerGuidance::next_work(x, y, intent, &job, &imp)) {
            state.m_map.set_tile_work_needed(x, y, 0u);
            cur->add_tile_imp_count(-1);
            wdest_clr(unit);
            return;
        }
        have = true;
    }
    if (!have) {
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
    if (apply_one(state, unit, ps, cur_idx, x, y, job, imp)) {
        if (m_job_note != nullptr) {
            m_job_note(x, y, job, imp, static_cast<u8>(intent));
        }
        if (!tile_pending(state, cur_idx, x, y)) {
            wdest_clr(unit);
        }
    } else {
        wdest_clr(unit);
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
