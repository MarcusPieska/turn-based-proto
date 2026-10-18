//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "player_land_level_save_timeline_mng.h"

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
#include "unit_domain_enum.h"
#include "unit_role_enum.h"
#include "unit_static_data.h"
#include "unit_static_key.h"

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

static bool unit_is_land_role (const RuntimeStatics& st, u16 typ_idx) {
    const UnitStaticData& ud = st.unit();
    if (typ_idx >= ud.get_item_count()) {
        return false;
    }
    const UnitStaticDataStruct& u = ud.get_item(UnitStaticDataKey::from_raw(typ_idx));
    return u.domain == static_cast<u16>(UnitDomain::LAND)
        && u.role != static_cast<u16>(UnitRole::NONE);
}

static u16 max_train_lv (const RuntimeStatics& st, const BitArrayCL* tech, u16 civ_idx) {
    EffectCtx ctx = {};
    ctx.m_owner = 0;
    ctx.m_city_idx = 0;
    ctx.m_tech = tech;
    effect_ctx_bind_civ_traits(ctx, st, civ_idx);
    const DynBoosterRegister& reg = st.dyn_booster();
    u16 max_lv = 0u;
    max_lv = apply_booster_u16(max_lv, reg.determine(ItemEffectBoosterType::UNIT_TRAINING_MAX, ItemEffectsScope::LOCAL, ctx));
    max_lv = apply_booster_u16(max_lv, reg.determine(ItemEffectBoosterType::UNIT_TRAINING_MAX, ItemEffectsScope::CITY, ctx));
    max_lv = apply_booster_u16(max_lv, reg.determine(ItemEffectBoosterType::UNIT_TRAINING_MAX, ItemEffectsScope::CIV, ctx));
    max_lv = apply_booster_u16(max_lv, reg.determine(ItemEffectBoosterType::UNIT_TRAINING_MAX, ItemEffectsScope::GLOBAL, ctx));
    const u16 lvl_n = st.unit_level().get_item_count();
    const u16 cat_hi = (lvl_n == 0u) ? 0u : static_cast<u16>(lvl_n - 1u);
    if (max_lv > cat_hi) {
        max_lv = cat_hi;
    }
    return max_lv;
}

//================================================================================================================================
//=> - PlayerLandLevelSaveTimelineMng -
//================================================================================================================================

PlayerLandLevelSaveTimelineMng::PlayerLandLevelSaveTimelineMng ()
    : m_tl(nullptr),
      m_seat(nullptr),
      m_lucky(nullptr),
      m_final_lvl(nullptr),
      m_player_n(0),
      m_save_n(0),
      m_lvl_n(0) {
}

PlayerLandLevelSaveTimelineMng::~PlayerLandLevelSaveTimelineMng () {
    clr();
}

void PlayerLandLevelSaveTimelineMng::clr () {
    delete[] m_tl;
    m_tl = nullptr;
    delete[] m_seat;
    m_seat = nullptr;
    delete[] m_lucky;
    m_lucky = nullptr;
    delete[] m_final_lvl;
    m_final_lvl = nullptr;
    m_player_n = 0;
    m_save_n = 0;
    m_lvl_n = 0;
}

bool PlayerLandLevelSaveTimelineMng::setup (u16 player_n, u16 save_n) {
    clr();
    if (player_n == 0 || save_n == 0) {
        return false;
    }
    m_tl = new PlayerLandLevelSaveTimeline[player_n];
    m_seat = new u16[player_n];
    m_lucky = new u8[player_n];
    m_player_n = player_n;
    m_save_n = save_n;
    for (u16 i = 0; i < player_n; ++i) {
        if (!m_tl[i].setup(save_n)) {
            clr();
            return false;
        }
        m_seat[i] = i;
        m_lucky[i] = 0;
    }
    return true;
}

bool PlayerLandLevelSaveTimelineMng::fill (const EvalPaths& paths, const RuntimeStatics& st) {
    if (m_tl == nullptr || m_lucky == nullptr || !paths.ok() || paths.save_turn_n() != m_save_n) {
        return false;
    }
    m_lvl_n = st.unit_level().get_item_count();
    if (m_lvl_n == 0u) {
        m_lvl_n = 1u;
    }
    delete[] m_final_lvl;
    m_final_lvl = new u32[static_cast<u32>(m_player_n) * static_cast<u32>(m_lvl_n)]();
    u32* land_n = new u32[m_player_n];
    u64* lvl_sum = new u64[m_player_n];
    PlayerState* seats = nullptr;
    u16 seat_n = 0;
    bool lucky_set = false;
    for (u16 si = 0; si < m_save_n; ++si) {
        const bool last = (si + 1u == m_save_n);
        const u32 turn = paths.save_turn_at(si);
        char units_p[512];
        char players_p[512];
        if (!paths.units_path(turn, units_p, sizeof(units_p))
            || !paths.players_path(turn, players_p, sizeof(players_p))) {
            delete[] land_n;
            delete[] lvl_sum;
            free_seats(seats, seat_n);
            return false;
        }
        if (!GameIo::load_players(players_p, seats, seat_n)) {
            delete[] land_n;
            delete[] lvl_sum;
            free_seats(seats, seat_n);
            return false;
        }
        if (seat_n < m_player_n) {
            delete[] land_n;
            delete[] lvl_sum;
            free_seats(seats, seat_n);
            return false;
        }
        if (!lucky_set) {
            for (u16 p = 0; p < m_player_n; ++p) {
                m_lucky[p] = (seats[p].m_lucky != 0u) ? 1u : 0u;
            }
            lucky_set = true;
        }
        for (u16 p = 0; p < m_player_n; ++p) {
            land_n[p] = 0;
            lvl_sum[p] = 0;
        }
        {
            UnitAddVector units;
            if (!GameIo::load_units(units_p, units)) {
                delete[] land_n;
                delete[] lvl_sum;
                free_seats(seats, seat_n);
                return false;
            }
            const u16 head = units.get_head_unit_add_idx();
            for (u16 k = 0; k < head; ++k) {
                const UnitAddStruct* u = units.get_unit_add(UnitAddKey::from_raw(k));
                if (u == nullptr || u->m_player_idx >= m_player_n) {
                    continue;
                }
                if (!unit_is_land_role(st, static_cast<u16>(u->m_unit_typ_idx))) {
                    continue;
                }
                const u16 p = u->m_player_idx;
                land_n[p] = land_n[p] + 1u;
                lvl_sum[p] = lvl_sum[p] + static_cast<u64>(u->m_level);
                if (last) {
                    u16 lv = u->m_level;
                    if (lv >= m_lvl_n) {
                        lv = static_cast<u16>(m_lvl_n - 1u);
                    }
                    const u32 ix = static_cast<u32>(p) * static_cast<u32>(m_lvl_n) + static_cast<u32>(lv);
                    m_final_lvl[ix] = m_final_lvl[ix] + 1u;
                }
            }
        }
        for (u16 p = 0; p < m_player_n; ++p) {
            u32 avg_milli = 0;
            if (land_n[p] > 0u) {
                avg_milli = static_cast<u32>((lvl_sum[p] * 1000ull) / static_cast<u64>(land_n[p]));
            }
            const u16 max_lv = max_train_lv(st, seats[p].m_techs_researched, seats[p].m_civ_index);
            if (!m_tl[p].set(si, land_n[p], avg_milli, max_lv)) {
                delete[] land_n;
                delete[] lvl_sum;
                free_seats(seats, seat_n);
                return false;
            }
        }
    }
    delete[] land_n;
    delete[] lvl_sum;
    free_seats(seats, seat_n);
    for (u16 p = 0; p < m_player_n; ++p) {
        m_tl[p].sync_count();
    }
    return true;
}

void PlayerLandLevelSaveTimelineMng::sort () {
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

u16 PlayerLandLevelSaveTimelineMng::player_n () const {
    return m_player_n;
}

u16 PlayerLandLevelSaveTimelineMng::save_n () const {
    return m_save_n;
}

u16 PlayerLandLevelSaveTimelineMng::lvl_n () const {
    return m_lvl_n;
}

u8 PlayerLandLevelSaveTimelineMng::lucky (u16 seat) const {
    if (m_lucky == nullptr || seat >= m_player_n) {
        return 0;
    }
    return m_lucky[seat];
}

u32 PlayerLandLevelSaveTimelineMng::final_lvl_count (u16 seat, u16 lv) const {
    if (m_final_lvl == nullptr || seat >= m_player_n || lv >= m_lvl_n) {
        return 0;
    }
    return m_final_lvl[static_cast<u32>(seat) * static_cast<u32>(m_lvl_n) + static_cast<u32>(lv)];
}

PlayerLandLevelSaveTimeline& PlayerLandLevelSaveTimelineMng::at (u16 i) {
    return m_tl[i];
}

const PlayerLandLevelSaveTimeline& PlayerLandLevelSaveTimelineMng::at (u16 i) const {
    return m_tl[i];
}

u16 PlayerLandLevelSaveTimelineMng::seat (u16 i) const {
    if (m_seat == nullptr || i >= m_player_n) {
        return UINT16_MAX;
    }
    return m_seat[i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
