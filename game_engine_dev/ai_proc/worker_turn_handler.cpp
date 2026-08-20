//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>

#include "worker_turn_handler.h"
#include "assert_log.h"
#include "city.h"
#include "city_tile_manager.h"
#include "circular_tile_areas.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "tile_usage.h"
#include "tile_working.h"
#include "tile_yields.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"
#include "worker_guidance.h"
#include "worker_helper.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

WorkerTurnHandler::JobNoteFn WorkerTurnHandler::m_job_note = nullptr;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_worker_typ (const GameState& state, u16 typ_idx) {
    const UnitStaticDataKey uk = UnitStaticDataKey::from_raw(typ_idx);
    const u16 ut = state.m_statics->unit().get_item(uk).type;
    const UnitTypeStaticDataKey tk = UnitTypeStaticDataKey::from_raw(ut);
    return std::strcmp(state.m_statics->unit_type().get_name(tk), "LAND_WORKER") == 0;
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
    u16* ojob)
{
    if (TileWorking::get_worker(ux, uy) != city_idx) {
        return false;
    }
    if (state.m_map.get_planned_city(ux, uy) != 0u) {
        return false;
    }
    const TileAssignIntent intent = static_cast<TileAssignIntent>(state.m_map.get_tile_usage(ux, uy));
    const u16 job = WorkerGuidance::next_job(ux, uy, intent);
    if (job == U16_KEY_NULL) {
        return false;
    }
    *ointent = intent;
    *ojob = job;
    return true;
}

static bool pick_first (
    GameState& state,
    u16 city_idx,
    u16 cx,
    u16 cy,
    u16* ox,
    u16* oy,
    TileAssignIntent* ointent,
    u16* ojob)
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
        if (!tile_cand(state, city_idx, ux, uy, ointent, ojob)) {
            continue;
        }
        *ox = ux;
        *oy = uy;
        return true;
    }
    return false;
}

static bool pick_best (
    GameState& state,
    u16 city_idx,
    u16 cx,
    u16 cy,
    u16* ox,
    u16* oy,
    TileAssignIntent* ointent,
    u16* ojob)
{
    const CircArea area = CityTileManager::work_area();
    u8 have_food = 0;
    u8 have_prod = 0;
    u16 best_food = 0;
    u16 best_prod = 0;
    u16 fx = 0;
    u16 fy = 0;
    u16 fjob = U16_KEY_NULL;
    u16 px = 0;
    u16 py = 0;
    u16 pjob = U16_KEY_NULL;
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
        if (!tile_cand(state, city_idx, ux, uy, &intent, &job)) {
            continue;
        }
        if (intent == TILE_ASSIGN_FOOD) {
            const u16 score = TileYields::food_no_imp(ux, uy);
            if (have_food == 0 || score > best_food) {
                best_food = score;
                fx = ux;
                fy = uy;
                fjob = job;
                fintent = intent;
                have_food = 1;
            }
        } else {
            const u16 score = TileYields::get(ux, uy).m_production;
            if (have_prod == 0 || score > best_prod) {
                best_prod = score;
                px = ux;
                py = uy;
                pjob = job;
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
        return true;
    }
    if (have_prod != 0) {
        *ox = px;
        *oy = py;
        *ointent = pintent;
        *ojob = pjob;
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

//================================================================================================================================
//=> - WorkerTurnHandler -
//================================================================================================================================

void WorkerTurnHandler::set_job_note (JobNoteFn fn) {
    m_job_note = fn;
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

    u16 city_idx = 0;
    u16 cx = 0;
    u16 cy = 0;
    if (!home_city(state, unit, &city_idx, &cx, &cy)) {
        return;
    }
    u16 x = 0;
    u16 y = 0;
    TileAssignIntent intent = TILE_ASSIGN_FOOD;
    u16 job = U16_KEY_NULL;
    const bool found = (ps.m_worker_tile_opt_scan != 0)
        ? pick_best(state, city_idx, cx, cy, &x, &y, &intent, &job)
        : pick_first(state, city_idx, cx, cy, &x, &y, &intent, &job);
    if (!found) {
        return;
    }
    if (!WorkerGuidance::apply_job(x, y, job)) {
        return;
    }
    if (m_job_note != nullptr) {
        m_job_note(x, y, job, static_cast<u8>(intent));
    }
    if (ps.m_worker_tile_opt_reassign != 0) {
        reassign(state, x, y, city_idx);
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
