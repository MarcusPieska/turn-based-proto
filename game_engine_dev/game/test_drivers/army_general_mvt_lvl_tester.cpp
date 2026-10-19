//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstring>
#include <limits>

#include "army_general.h"
#include "game_state.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_level_static_data.h"
#include "unit_level_static_key.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static RuntimeStaticLoader g_ld;
static RuntimeStatics* g_st = nullptr;
static const u16 k_army_n = 5u;
static const u32 k_time_n = 1000u;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool load_statics () {
    if (g_st != nullptr) {
        return true;
    }
    if (!g_ld.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        return false;
    }
    g_st = &g_ld.statics();
    return g_st != nullptr;
}

static void dist_lvls (u8* out, u16 n, u16 sum, u16 max_lv) {
    for (u16 i = 0u; i < n; ++i) {
        out[i] = 0u;
    }
    u16 left = sum;
    for (u16 i = 0u; i < n && left > 0u; ++i) {
        u16 take = max_lv;
        if ((i & 1u) != 0u && take > 1u) {
            take = static_cast<u16>(take / 2u);
        }
        if (take > left) {
            take = left;
        }
        out[i] = static_cast<u8>(take);
        left = static_cast<u16>(left - take);
    }
    for (u16 i = 0u; i < n && left > 0u; ++i) {
        const u16 room = static_cast<u16>(max_lv - static_cast<u16>(out[i]));
        u16 take = room;
        if (take > left) {
            take = left;
        }
        out[i] = static_cast<u8>(static_cast<u16>(out[i]) + take);
        left = static_cast<u16>(left - take);
    }
}

static u16 expect_bonus (const UnitLevelStaticData& lvls, u16 avg) {
    u16 bonus = 0u;
    for (u16 i = 0u; i <= avg; ++i) {
        bonus = static_cast<u16>(bonus + lvls.get_item(UnitLevelStaticDataKey::from_raw(i)).mvt_boost);
    }
    return bonus;
}

static bool mk_army (GameState& st, const u8* lvls, u16 n, ArmyGeneral& gen) {
    UnitAddKey hd = UnitAddKey::None();
    UnitAddKey prev = UnitAddKey::None();
    for (u16 i = 0u; i < n; ++i) {
        const UnitAddKey key = st.m_units.get_next_new_unit_add_key();
        UnitAddStruct* u = st.m_units.get_unit_add(key);
        if (u == nullptr) {
            return false;
        }
        std::memset(u, 0, sizeof(*u));
        u->m_x = 0u;
        u->m_y = 0u;
        u->m_unit_typ_idx = 0u;
        u->m_next_unit_on_tile = U16_KEY_NULL;
        u->m_next_unit_in_group = U16_KEY_NULL;
        u->m_player_idx = 0u;
        u->m_health = UNIT_HEALTH;
        u->m_level = lvls[i];
        u->m_mvt_points = 0;
        if (!hd.is_valid()) {
            hd = key;
        } else {
            UnitAddStruct* pu = st.m_units.get_unit_add(prev);
            if (pu == nullptr) {
                return false;
            }
            pu->m_next_unit_in_group = key.value();
        }
        prev = key;
    }
    gen.asgn(hd.value());
    return true;
}

static void set_mps (GameState& st, u16 hd, i16 mp) {
    u16 cur = hd;
    while (cur != U16_KEY_NULL) {
        UnitAddStruct* u = st.m_units.get_unit_add(UnitAddKey::from_raw(cur));
        if (u == nullptr) {
            break;
        }
        u->m_mvt_points = mp;
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = static_cast<u16>(u->m_next_unit_in_group);
    }
}

static bool chk_mps (GameState& st, u16 hd, i16 want) {
    u16 cur = hd;
    while (cur != U16_KEY_NULL) {
        const UnitAddStruct* u = st.m_units.get_unit_add(UnitAddKey::from_raw(cur));
        if (u == nullptr) {
            return false;
        }
        if (u->m_mvt_points != want) {
            return false;
        }
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = static_cast<u16>(u->m_next_unit_in_group);
    }
    return true;
}

static void pr_dist (const u8* lvls, u16 n) {
    std::printf("  dist=[");
    for (u16 i = 0u; i < n; ++i) {
        std::printf("%s%u", (i == 0u) ? "" : ",", static_cast<u32>(lvls[i]));
    }
    std::printf("]");
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!load_statics()) {
        std::printf("FAILED load statics\n");
        return 1;
    }
    const UnitLevelStaticData& ul = g_st->unit_level();
    const u16 lvl_n = ul.get_item_count();
    if (lvl_n == 0u) {
        std::printf("FAILED no unit levels\n");
        return 1;
    }
    const u16 max_lv = static_cast<u16>(lvl_n - 1u);
    const u16 turn_mp = g_st->config().get_mov_pt_per_turn();

    GameState st;
    st.m_statics = g_st;

    std::printf("army_general mvt_lvl: army_n=%u turn_mp=%u time_n=%u\n",
        k_army_n, turn_mp, k_time_n);
    std::printf("%-14s %6s %8s %8s %8s %8s %8s %8s\n",
        "target", "avg", "bonus", "obs", "ok", "min_us", "max_us", "avg_us");

    int fails = 0;
    u8 dist[k_army_n];
    for (u16 avg = 0u; avg < lvl_n; ++avg) {
        const u16 sum = static_cast<u16>(avg * k_army_n);
        dist_lvls(dist, k_army_n, sum, max_lv);
        u32 got_sum = 0u;
        for (u16 i = 0u; i < k_army_n; ++i) {
            got_sum += dist[i];
        }
        const f64 real_avg = static_cast<f64>(got_sum) / static_cast<f64>(k_army_n);
        const u16 floor_avg = static_cast<u16>(got_sum / k_army_n);
        const u16 bonus = expect_bonus(ul, floor_avg);
        const i16 want = static_cast<i16>(static_cast<i32>(turn_mp) + static_cast<i32>(bonus));

        ArmyGeneral gen;
        if (!mk_army(st, dist, k_army_n, gen)) {
            std::printf("FAILED mk_army avg=%u\n", avg);
            fails++;
            break;
        }
        const u16 hd = gen.grp();
        set_mps(st, hd, 0);
        gen.boost_mvt_pts(st);
        gen.boost_mvt_lvl(st);
        const bool ok = chk_mps(st, hd, want);
        if (!ok) {
            fails++;
        }
        const i16 obs = st.m_units.get_unit_add(UnitAddKey::from_raw(hd))->m_mvt_points;

        f64 t_sum = 0.0;
        f64 t_min = std::numeric_limits<f64>::max();
        f64 t_max = 0.0;
        for (u32 t = 0u; t < k_time_n; ++t) {
            set_mps(st, hd, 0);
            const auto t0 = std::chrono::steady_clock::now();
            gen.boost_mvt_pts(st);
            gen.boost_mvt_lvl(st);
            const auto t1 = std::chrono::steady_clock::now();
            const f64 us = static_cast<f64>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()) / 1000.0;
            t_sum += us;
            if (us < t_min) {
                t_min = us;
            }
            if (us > t_max) {
                t_max = us;
            }
        }
        const f64 t_avg = t_sum / static_cast<f64>(k_time_n);
        const cstr lname = ul.get_name(UnitLevelStaticDataKey::from_raw(avg));
        std::printf("%-14s %6.2f %8u %8d %8s %8.2f %8.2f %8.2f",
            lname, real_avg, static_cast<u32>(bonus), static_cast<i32>(obs),
            ok ? "PASS" : "FAIL", t_min, t_max, t_avg);
        pr_dist(dist, k_army_n);
        std::printf("\n");
        gen.unasgn();
    }
    std::printf("army_general_mvt_lvl_tester: %s (%d fails)\n",
        (fails == 0) ? "ok" : "FAILED", fails);
    return (fails == 0) ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
