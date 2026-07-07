//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "explore_near.h"
#include "walk_coast_mk1.h"
#include "walk_mtn_mk1.h"
#include "walk_river_mk2.h"
#include "game_map_defs.h"
#include "game_map_grid_defs.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u16 enc_coast (u16 st) {
    return static_cast<u16>(10u + st);
}

static u16 enc_mtn (u16 st) {
    return static_cast<u16>(20u + st);
}

static u16 enc_riv (u16 st) {
    return static_cast<u16>(30u + st);
}

//================================================================================================================================
//=> - ExploreNear -
//================================================================================================================================

u16 ExploreNear::can_ai_start_from (
    const GameArraySimple& map,
    MapBitOverlay& ov,
    u16 x,
    u16 y,
    u16 sight,
    const ExOpt& coast,
    const ExOpt& mtn,
    const ExOpt& riv) {
    const u16 ns = WalkNearMk1::can_ai_start_from(map, ov, x, y, sight);
    if (ns != 0u) {
        return ns;
    }
    if (coast.mode != EX_OPT_IGNORE) {
        const u16 cs = WalkCoastMk1::can_ai_start_from(map, ov, x, y, sight);
        if (cs != 0u) {
            return enc_coast(cs);
        }
    }
    if (mtn.mode != EX_OPT_IGNORE) {
        const u16 ms = WalkMtnMk1::can_ai_start_from(map, ov, x, y, sight);
        if (ms != 0u) {
            return enc_mtn(ms);
        }
    }
    if (riv.mode != EX_OPT_IGNORE) {
        const u16 rs = WalkRiverMk2::can_ai_start_from(map, ov, x, y, sight);
        if (rs != 0u) {
            return enc_riv(rs);
        }
    }
    return 0u;
}

ExploreNear::ExploreNear (
    const GameArraySimple& map,
    MapBitOverlay& ov,
    u16 sx,
    u16 sy,
    u16 sight,
    u8 player,
    WalkNearBias bias,
    const ExOpt& coast,
    const ExOpt& mtn,
    const ExOpt& riv) :
    m_map(map),
    m_ov(ov),
    m_path(map),
    m_oc(coast),
    m_om(mtn),
    m_or(riv),
    m_x(sx),
    m_y(sy),
    m_rtx(0),
    m_rty(0),
    m_hhx(sx),
    m_hhy(sy),
    m_sight(sight),
    m_player(player),
    m_bias(static_cast<u8>(bias)),
    m_st(k_st_near),
    m_near_n(0),
    m_path_n(0),
    m_coast_n(0),
    m_mtn_n(0),
    m_riv_n(0),
    m_riv_done(0),
    m_wn(nullptr),
    m_wc(nullptr),
    m_wm(nullptr),
    m_wr(nullptr) {
    spawn_near(sx, sy);
}

ExploreNear::~ExploreNear () {
    delete m_wn;
    delete m_wc;
    delete m_wm;
    delete m_wr;
}

void ExploreNear::rev_around (u16 x, u16 y) {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    for (i16 dy = -static_cast<i16>(m_sight); dy <= static_cast<i16>(m_sight); ++dy) {
        for (i16 dx = -static_cast<i16>(m_sight); dx <= static_cast<i16>(m_sight); ++dx) {
            const i32 adx = (dx < 0) ? -dx : dx;
            const i32 ady = (dy < 0) ? -dy : dy;
            if (static_cast<u32>((adx > ady) ? adx : ady) > static_cast<u32>(m_sight)) {
                continue;
            }
            const i32 xi = static_cast<i32>(x) + dx;
            const i32 yi = static_cast<i32>(y) + dy;
            if (xi < 0 || yi < 0) {
                continue;
            }
            const u16 ax = static_cast<u16>(xi);
            const u16 ay = static_cast<u16>(yi);
            if (ax >= w || ay >= h) {
                continue;
            }
            if (m_ov.get(ax, ay) != 0u) {
                continue;
            }
            m_ov.set(ax, ay);
        }
    }
}

void ExploreNear::stop () {
    delete m_wn;
    delete m_wc;
    delete m_wm;
    delete m_wr;
    m_wn = nullptr;
    m_wc = nullptr;
    m_wm = nullptr;
    m_wr = nullptr;
    m_st = k_st_done;
}

void ExploreNear::spawn_near (u16 sx, u16 sy) {
    delete m_wn;
    m_wn = new WalkNearMk1(
        m_map,
        m_ov,
        sx,
        sy,
        m_sight,
        m_player,
        static_cast<WalkNearBias>(m_bias));
    m_x = sx;
    m_y = sy;
    m_hhx = m_wn->hx();
    m_hhy = m_wn->hy();
    m_st = k_st_near;
}

void ExploreNear::spawn_coast (u16 sx, u16 sy) {
    delete m_wc;
    m_wc = new WalkCoastMk1(m_map, m_ov, sx, sy, m_sight, m_player);
    m_x = sx;
    m_y = sy;
    m_st = k_st_coast;
}

void ExploreNear::spawn_mtn (u16 sx, u16 sy) {
    delete m_wm;
    m_wm = new WalkMtnMk1(m_map, m_ov, sx, sy, m_sight, m_player);
    m_x = sx;
    m_y = sy;
    m_st = k_st_mtn;
}

void ExploreNear::spawn_river (u16 sx, u16 sy) {
    delete m_wr;
    m_wr = new WalkRiverMk2(m_map, m_ov, sx, sy, m_sight, m_player);
    m_x = sx;
    m_y = sy;
    m_st = k_st_river;
}

void ExploreNear::resume_near_or_stop (u16 sx, u16 sy) {
    if (WalkNearMk1::can_ai_start_from(m_map, m_ov, sx, sy, m_sight) != 0u) {
        spawn_near(sx, sy);
    } else {
        m_x = sx;
        m_y = sy;
        stop();
    }
}

void ExploreNear::finish_coast () {
    u16 sx = m_wc->x();
    u16 sy = m_wc->y();
    if (m_oc.mode == EX_OPT_RECENTER) {
        sx = m_hhx;
        sy = m_hhy;
    }
    delete m_wc;
    m_wc = nullptr;
    resume_near_or_stop(sx, sy);
}

void ExploreNear::finish_mtn () {
    u16 sx = m_wm->x();
    u16 sy = m_wm->y();
    if (m_om.mode == EX_OPT_RECENTER) {
        sx = m_hhx;
        sy = m_hhy;
    }
    delete m_wm;
    m_wm = nullptr;
    resume_near_or_stop(sx, sy);
}

void ExploreNear::finish_river () {
    u16 sx = m_wr->x();
    u16 sy = m_wr->y();
    if (m_or.mode == EX_OPT_RECENTER) {
        sx = m_hhx;
        sy = m_hhy;
    }
    delete m_wr;
    m_wr = nullptr;
    resume_near_or_stop(sx, sy);
}

bool ExploreNear::try_coast () {
    if (m_oc.mode == EX_OPT_IGNORE || m_wn == nullptr) {
        return false;
    }
    if (WalkCoastMk1::can_ai_start_from(m_map, m_ov, m_wn->x(), m_wn->y(), m_sight) == 0u) {
        return false;
    }
    m_hhx = m_wn->hx();
    m_hhy = m_wn->hy();
    m_x = m_wn->x();
    m_y = m_wn->y();
    delete m_wn;
    m_wn = nullptr;
    spawn_coast(m_x, m_y);
    return true;
}

bool ExploreNear::try_mtn () {
    if (m_om.mode == EX_OPT_IGNORE || m_wn == nullptr) {
        return false;
    }
    if (WalkMtnMk1::can_ai_start_from(m_map, m_ov, m_wn->x(), m_wn->y(), m_sight) == 0u) {
        return false;
    }
    m_hhx = m_wn->hx();
    m_hhy = m_wn->hy();
    m_x = m_wn->x();
    m_y = m_wn->y();
    delete m_wn;
    m_wn = nullptr;
    spawn_mtn(m_x, m_y);
    return true;
}

bool ExploreNear::try_river () {
    if (m_or.mode == EX_OPT_IGNORE || m_wn == nullptr) {
        return false;
    }
    if (WalkRiverMk2::can_ai_start_from(m_map, m_ov, m_wn->x(), m_wn->y(), m_sight) == 0u) {
        return false;
    }
    m_hhx = m_wn->hx();
    m_hhy = m_wn->hy();
    m_x = m_wn->x();
    m_y = m_wn->y();
    delete m_wn;
    m_wn = nullptr;
    spawn_river(m_x, m_y);
    return true;
}

void ExploreNear::tick_near () {
    if (m_wn == nullptr) {
        return;
    }
    if (m_or.mode != EX_OPT_IGNORE && m_wn->riv_spot()) {
        m_rtx = m_wn->riv_x();
        m_rty = m_wn->riv_y();
        m_wn->clr_riv_spot();
        m_x = m_wn->x();
        m_y = m_wn->y();
        delete m_wn;
        m_wn = nullptr;
        m_st = k_st_path;
        return;
    }
    if (m_wn->done()) {
        m_x = m_wn->x();
        m_y = m_wn->y();
        stop();
        return;
    }
    if (try_coast()) {
        return;
    }
    if (try_mtn()) {
        return;
    }
    if (try_river()) {
        return;
    }
    m_wn->move(1u);
    m_x = m_wn->x();
    m_y = m_wn->y();
    ++m_near_n;
}

void ExploreNear::tick_coast () {
    if (m_wc == nullptr) {
        return;
    }
    if (m_wc->done()) {
        finish_coast();
        return;
    }
    m_wc->move(1u);
    m_x = m_wc->x();
    m_y = m_wc->y();
    ++m_coast_n;
}

void ExploreNear::tick_mtn () {
    if (m_wm == nullptr) {
        return;
    }
    if (m_wm->done()) {
        finish_mtn();
        return;
    }
    m_wm->move(1u);
    m_x = m_wm->x();
    m_y = m_wm->y();
    ++m_mtn_n;
}

void ExploreNear::tick_path () {
    if (m_x == m_rtx && m_y == m_rty) {
        spawn_river(m_x, m_y);
        return;
    }
    if (m_map.get_river(m_x, m_y) != 0u) {
        spawn_river(m_x, m_y);
        return;
    }
    u16 nx = m_x;
    u16 ny = m_y;
    if (!m_path.one_step(m_x, m_y, m_rtx, m_rty, nx, ny)) {
        resume_near_or_stop(m_x, m_y);
        return;
    }
    m_x = nx;
    m_y = ny;
    rev_around(m_x, m_y);
    ++m_path_n;
}

void ExploreNear::tick_river () {
    if (m_wr == nullptr) {
        return;
    }
    const u16 burst = (m_or.param > 0u) ? m_or.param : 1u;
    for (u16 m = 0u; m < burst; ++m) {
        if (m_wr->phase() == k_riv_done) {
            break;
        }
        m_wr->move(1u);
        ++m_riv_n;
    }
    m_x = m_wr->x();
    m_y = m_wr->y();
    if (m_wr->phase() == k_riv_done) {
        ++m_riv_done;
        finish_river();
    }
}

void ExploreNear::move (u16 moves) {
    for (u16 m = 0u; m < moves && m_st != k_st_done; ++m) {
        if (m_st == k_st_near) {
            tick_near();
        } else if (m_st == k_st_coast) {
            tick_coast();
        } else if (m_st == k_st_mtn) {
            tick_mtn();
        } else if (m_st == k_st_path) {
            tick_path();
        } else if (m_st == k_st_river) {
            tick_river();
        }
    }
}

u16 ExploreNear::x () const {
    return m_x;
}

u16 ExploreNear::y () const {
    return m_y;
}

u8 ExploreNear::st () const {
    return m_st;
}

bool ExploreNear::done () const {
    return m_st == k_st_done;
}

u32 ExploreNear::near_n () const {
    return m_near_n;
}

u32 ExploreNear::path_n () const {
    return m_path_n;
}

u32 ExploreNear::coast_n () const {
    return m_coast_n;
}

u32 ExploreNear::mtn_n () const {
    return m_mtn_n;
}

u32 ExploreNear::riv_n () const {
    return m_riv_n;
}

u32 ExploreNear::riv_done () const {
    return m_riv_done;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
