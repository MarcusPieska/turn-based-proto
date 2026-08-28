//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "worker_pathing.h"
#include "assert_log.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const i32 k_dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const i32 k_dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
static const u8 k_dir_n = 8u;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static i32 sgn (i32 v) {
    return (v > 0) - (v < 0);
}

static u32 cheb (u16 ax, u16 ay, u16 bx, u16 by) {
    const i32 dx = static_cast<i32>(ax) - static_cast<i32>(bx);
    const i32 dy = static_cast<i32>(ay) - static_cast<i32>(by);
    const u32 adx = static_cast<u32>(dx < 0 ? -dx : dx);
    const u32 ady = static_cast<u32>(dy < 0 ? -dy : dy);
    return adx > ady ? adx : ady;
}

static bool step_linear (u16 x0, u16 y0, u16 x1, u16 y1, u16* ox, u16* oy) {
    if (x0 == x1 && y0 == y1) {
        return false;
    }
    const i32 ax = static_cast<i32>(x1) - static_cast<i32>(x0);
    const i32 ay = static_cast<i32>(y1) - static_cast<i32>(y0);
    const i32 adx = ax < 0 ? -ax : ax;
    const i32 ady = ay < 0 ? -ay : ay;
    const i32 sx = sgn(ax);
    const i32 sy = sgn(ay);
    i32 nx = static_cast<i32>(x0);
    i32 ny = static_cast<i32>(y0);
    if (adx > ady) {
        if (ady == 0 || (ady + ady) <= adx) {
            nx += sx;
        } else {
            nx += sx;
            ny += sy;
        }
    } else if (ady > adx) {
        if (adx == 0 || (adx + adx) <= ady) {
            ny += sy;
        } else {
            nx += sx;
            ny += sy;
        }
    } else {
        nx += sx;
        ny += sy;
    }
    if (nx < 0 || ny < 0) {
        return false;
    }
    *ox = static_cast<u16>(nx);
    *oy = static_cast<u16>(ny);
    return true;
}

static bool try_worker_step (GameState& state, u16 unit_idx, u16 ux, u16 uy) {
    const UnitAddKey key = UnitAddKey::from_raw(unit_idx);
    i16 cost = 0;
    if (!UnitMovementMng::can_step_worker(state, key, ux, uy, &cost)) {
        return false;
    }
    return UnitMovementMng::apply_step_worker(state, key, ux, uy);
}

static bool pick_worker_step (
    const GameState& state,
    u16 unit_idx,
    u16 wx,
    u16 wy,
    u16 tx,
    u16 ty,
    u16* ox,
    u16* oy)
{
    const UnitAddKey key = UnitAddKey::from_raw(unit_idx);
    u16 nx = 0;
    u16 ny = 0;
    if (step_linear(wx, wy, tx, ty, &nx, &ny)) {
        i16 cost = 0;
        if (UnitMovementMng::can_step_worker(state, key, nx, ny, &cost)) {
            *ox = nx;
            *oy = ny;
            return true;
        }
    }
    u32 best_d = 0xffffffffu;
    u16 best_x = U16_KEY_NULL;
    u16 best_y = U16_KEY_NULL;
    for (u8 dir = 0; dir < k_dir_n; ++dir) {
        const i32 x = static_cast<i32>(wx) + k_dx[dir];
        const i32 y = static_cast<i32>(wy) + k_dy[dir];
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (ux >= state.m_map.width() || uy >= state.m_map.height()) {
            continue;
        }
        i16 cost = 0;
        if (!UnitMovementMng::can_step_worker(state, key, ux, uy, &cost)) {
            continue;
        }
        const u32 d = cheb(ux, uy, tx, ty);
        if (d < best_d) {
            best_d = d;
            best_x = ux;
            best_y = uy;
        }
    }
    if (best_x == U16_KEY_NULL) {
        return false;
    }
    *ox = best_x;
    *oy = best_y;
    return true;
}

//================================================================================================================================
//=> - WorkerPathing -
//================================================================================================================================

bool WorkerPathing::step_toward (GameState& state, u16 unit_idx, u16 tx, u16 ty) {
    UnitAddStruct* unit = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    GAME_EXPECT(unit != nullptr, "WorkerPathing step_toward got nullptr unit");
    GAME_EXPECT(unit->m_x != U16_KEY_NULL, "WorkerPathing step_toward unit has null x");
    if (unit->m_x == tx && unit->m_y == ty) {
        return true;
    }
    u16 nx = 0;
    u16 ny = 0;
    if (!pick_worker_step(state, unit_idx, unit->m_x, unit->m_y, tx, ty, &nx, &ny)) {
        return false;
    }
    if (!try_worker_step(state, unit_idx, nx, ny)) {
        return false;
    }
    return unit->m_x == tx && unit->m_y == ty;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
