//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "player_land_size_save_timeline_mng.h"

#include <cstdio>
#include <cstring>

#include "booster_apply.h"
#include "dyn_booster_register.h"
#include "effect_ctx.h"
#include "eval_paths.h"
#include "game_io.h"
#include "game_state.h"
#include "item_effects.h"
#include "runtime_statics.h"
#include "tech_age_mng.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_static_data.h"
#include "unit_static_key.h"
#include "unit_type_static_data.h"
#include "unit_type_static_key.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void free_seats (PlayerState*& seats, u16& n) {
    if (seats == nullptr) {
        n = 0;
        return;
    }
    for (u16 i = 0; i < n; ++i) {
        delete[] seats[i].m_small_wonder_city;
        seats[i].m_small_wonder_city = nullptr;
        delete seats[i].m_explored_overlay;
        seats[i].m_explored_overlay = nullptr;
        delete seats[i].m_techs_researched;
        seats[i].m_techs_researched = nullptr;
        delete seats[i].m_tech_age;
        seats[i].m_tech_age = nullptr;
        seats[i].m_res_ledger.clear();
    }
    delete[] seats;
    seats = nullptr;
    n = 0;
}

static bool unit_is_land (const RuntimeStatics& st, u16 typ_idx) {
    const UnitStaticData& ud = st.unit();
    if (typ_idx >= ud.get_item_count()) {
        return false;
    }
    const UnitStaticDataStruct& u = ud.get_item(UnitStaticDataKey::from_raw(typ_idx));
    cstr nm = st.unit_type().get_name(UnitTypeStaticDataKey::from_raw(u.type));
    return nm != nullptr && std::strncmp(nm, "LAND_", 5) == 0;
}

static u16 max_unit_size_eff (const RuntimeStatics& st, const BitArrayCL* tech) {
    EffectCtx ctx = {};
    ctx.m_owner = 0;
    ctx.m_city_idx = 0;
    ctx.m_tech = tech;
    const u16 raw = apply_booster_u16(0, st.dyn_booster().determine(
        ItemEffectBoosterType::MAX_UNIT_SIZE, ItemEffectsScope::CIV, ctx));
    return static_cast<u16>(1u + raw);
}

//================================================================================================================================
//=> - PlayerLandSizeSaveTimelineMng -
//================================================================================================================================

PlayerLandSizeSaveTimelineMng::PlayerLandSizeSaveTimelineMng ()
    : m_tl(nullptr),
      m_seat(nullptr),
      m_player_n(0),
      m_save_n(0) {
}

PlayerLandSizeSaveTimelineMng::~PlayerLandSizeSaveTimelineMng () {
    clr();
}

void PlayerLandSizeSaveTimelineMng::clr () {
    delete[] m_tl;
    m_tl = nullptr;
    delete[] m_seat;
    m_seat = nullptr;
    m_player_n = 0;
    m_save_n = 0;
}

bool PlayerLandSizeSaveTimelineMng::setup (u16 player_n, u16 save_n) {
    clr();
    if (player_n == 0 || save_n == 0) {
        return false;
    }
    m_tl = new PlayerLandSizeSaveTimeline[player_n];
    m_seat = new u16[player_n];
    m_player_n = player_n;
    m_save_n = save_n;
    for (u16 i = 0; i < player_n; ++i) {
        if (!m_tl[i].setup(save_n)) {
            clr();
            return false;
        }
        m_seat[i] = i;
    }
    return true;
}

bool PlayerLandSizeSaveTimelineMng::fill (const EvalPaths& paths, const RuntimeStatics& st) {
    if (m_tl == nullptr || !paths.ok() || paths.save_turn_n() != m_save_n) {
        return false;
    }
    u32* land_n = new u32[m_player_n];
    u64* size_sum = new u64[m_player_n];
    PlayerState* seats = nullptr;
    u16 seat_n = 0;
    for (u16 si = 0; si < m_save_n; ++si) {
        const u32 turn = paths.save_turn_at(si);
        char units_p[512];
        char players_p[512];
        if (!paths.units_path(turn, units_p, sizeof(units_p))
            || !paths.players_path(turn, players_p, sizeof(players_p))) {
            delete[] land_n;
            delete[] size_sum;
            free_seats(seats, seat_n);
            return false;
        }
        if (!GameIo::load_players(players_p, seats, seat_n)) {
            delete[] land_n;
            delete[] size_sum;
            free_seats(seats, seat_n);
            return false;
        }
        if (seat_n < m_player_n) {
            delete[] land_n;
            delete[] size_sum;
            free_seats(seats, seat_n);
            return false;
        }
        for (u16 p = 0; p < m_player_n; ++p) {
            land_n[p] = 0;
            size_sum[p] = 0;
        }
        {
            UnitAddVector units;
            if (!GameIo::load_units(units_p, units)) {
                delete[] land_n;
                delete[] size_sum;
                free_seats(seats, seat_n);
                return false;
            }
            const u16 head = units.get_head_unit_add_idx();
            for (u16 k = 0; k < head; ++k) {
                const UnitAddStruct* u = units.get_unit_add(UnitAddKey::from_raw(k));
                if (u == nullptr || u->m_player_idx >= m_player_n) {
                    continue;
                }
                if (!unit_is_land(st, static_cast<u16>(u->m_unit_typ_idx))) {
                    continue;
                }
                const u16 p = u->m_player_idx;
                land_n[p] = land_n[p] + 1u;
                size_sum[p] = size_sum[p] + 1ull + static_cast<u64>(u->m_unit_size);
            }
        }
        for (u16 p = 0; p < m_player_n; ++p) {
            u32 avg_milli = 0;
            if (land_n[p] > 0u) {
                avg_milli = static_cast<u32>((size_sum[p] * 1000ull) / static_cast<u64>(land_n[p]));
            }
            const u16 max_sz = max_unit_size_eff(st, seats[p].m_techs_researched);
            if (!m_tl[p].set(si, land_n[p], avg_milli, max_sz)) {
                delete[] land_n;
                delete[] size_sum;
                free_seats(seats, seat_n);
                return false;
            }
        }
    }
    delete[] land_n;
    delete[] size_sum;
    free_seats(seats, seat_n);
    for (u16 p = 0; p < m_player_n; ++p) {
        m_tl[p].sync_count();
    }
    return true;
}

void PlayerLandSizeSaveTimelineMng::sort () {
    if (m_tl == nullptr || m_player_n < 2) {
        return;
    }
    for (u16 i = 0; i < m_player_n; ++i) {
        u16 best = i;
        for (u16 j = static_cast<u16>(i + 1u); j < m_player_n; ++j) {
            if (m_tl[j].count() > m_tl[best].count()) {
                best = j;
            }
        }
        if (best != i) {
            m_tl[i].swap(m_tl[best]);
            const u16 s = m_seat[i];
            m_seat[i] = m_seat[best];
            m_seat[best] = s;
        }
    }
}

u16 PlayerLandSizeSaveTimelineMng::player_n () const {
    return m_player_n;
}

u16 PlayerLandSizeSaveTimelineMng::save_n () const {
    return m_save_n;
}

PlayerLandSizeSaveTimeline& PlayerLandSizeSaveTimelineMng::at (u16 i) {
    return m_tl[i];
}

const PlayerLandSizeSaveTimeline& PlayerLandSizeSaveTimelineMng::at (u16 i) const {
    return m_tl[i];
}

u16 PlayerLandSizeSaveTimelineMng::seat (u16 i) const {
    if (m_seat == nullptr || i >= m_player_n) {
        return UINT16_MAX;
    }
    return m_seat[i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
