//================================================================================================================================
//= Includes =
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "game_primitives.h"
#include "game_state.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "unit_domain_enum.h"
#include "unit_group_management.h"
#include "unit_movement_mng.h"
#include "unit_role_enum.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"
#include "unit_utility_helper.h"

//================================================================================================================================
//= Paths =
//================================================================================================================================

static const char* G_RT_LIB = "../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/unit-group-partition";
static const char* G_OUT_1000 = "/home/w/Projects/simple-map-gen/unit-group-partition/partition_breakdown_1000.txt";
static const char* G_OUT_3000 = "/home/w/Projects/simple-map-gen/unit-group-partition/partition_breakdown_3000.txt";
static const u16 G_PICK_1000 = 1000u;
static const u16 G_PICK_3000 = 3000u;
static const u16 G_PICK_MAX = 3000u;
static const u16 G_SEED = 42u;
static const u16 G_TIME_N = 50u;

//================================================================================================================================
//= Helpers =
//================================================================================================================================

static bool ensure_dir (cstr path) {
    if (path == nullptr) {
        return false;
    }
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return mkdir(path, 0755) == 0;
}

struct Rng32 {
    u32 m_s;
};

static void rng_seed (Rng32* g, u32 seed) {
    g->m_s = seed != 0u ? seed : 1u;
}

static u32 rng_next (Rng32* g) {
    g->m_s = g->m_s * 1664525u + 1013904223u;
    return g->m_s;
}

static void bind_land_types (GameState& st) {
    st.m_land_defense_type_idx = U16_KEY_NULL;
    st.m_land_attack_type_idx = U16_KEY_NULL;
    st.m_land_mobile_type_idx = U16_KEY_NULL;
    st.m_land_artillery_type_idx = U16_KEY_NULL;
    if (st.m_statics == nullptr) {
        return;
    }
    const u16 n = st.m_statics->unit_type().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        cstr nm = st.m_statics->unit_type().get_name(UnitTypeStaticDataKey::from_raw(i));
        if (nm == nullptr) {
            continue;
        }
        if (std::strcmp(nm, "LAND_DEFENSE") == 0) {
            st.m_land_defense_type_idx = i;
        } else if (std::strcmp(nm, "LAND_ATTACK") == 0) {
            st.m_land_attack_type_idx = i;
        } else if (std::strcmp(nm, "LAND_MOBILE") == 0) {
            st.m_land_mobile_type_idx = i;
        } else if (std::strcmp(nm, "LAND_ARTILLERY") == 0) {
            st.m_land_artillery_type_idx = i;
        }
    }
}

static cstr type_name (const RuntimeStatics& st, u16 typ_idx) {
    const u16 un = st.unit().get_item_count();
    if (typ_idx >= un) {
        return "?";
    }
    const u16 ut = st.unit().get_item(UnitStaticDataKey::from_raw(typ_idx)).type;
    cstr nm = st.unit_type().get_name(UnitTypeStaticDataKey::from_raw(ut));
    return nm != nullptr ? nm : "?";
}

static cstr unit_name (const RuntimeStatics& st, u16 typ_idx) {
    cstr nm = st.unit().get_name(UnitStaticDataKey::from_raw(typ_idx));
    return nm != nullptr ? nm : "?";
}

static bool collect_grp (
    GameState& st,
    UnitAddKey head,
    UnitAddKey* keys,
    u16 cap,
    u16* out_n) 
{
    *out_n = 0u;
    UnitAddKey cur = head;
    while (cur.is_valid() && *out_n < cap) {
        keys[*out_n] = cur;
        *out_n = static_cast<u16>(*out_n + 1u);
        const UnitAddStruct* u = st.m_units.get_unit_add(cur);
        if (u == nullptr || u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    return *out_n > 0u;
}

static void write_breakdown (
    std::FILE* fp,
    const GameState& st,
    const UnitAddKey* keys,
    u16 n,
    cstr title) 
{
    if (fp == nullptr || st.m_statics == nullptr) {
        return;
    }
    std::fprintf(fp, "%s\n", title);
    const u16 un = st.m_statics->unit().get_item_count();
    const u16 tn = st.m_statics->unit_type().get_item_count();
    for (u16 ut = 0; ut < tn; ++ut) {
        u16 tot = 0;
        for (u16 i = 0; i < n; ++i) {
            const UnitAddStruct* u = st.m_units.get_unit_add(keys[i]);
            if (u == nullptr || u->m_unit_typ_idx >= un) {
                continue;
            }
            if (st.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).type == ut) {
                tot++;
            }
        }
        if (tot == 0u) {
            continue;
        }
        cstr tnm = st.m_statics->unit_type().get_name(UnitTypeStaticDataKey::from_raw(ut));
        std::fprintf(fp, "  %s %u :", tnm != nullptr ? tnm : "?", static_cast<unsigned>(tot));
        bool first = true;
        for (u16 ui = 0; ui < un; ++ui) {
            if (st.m_statics->unit().get_item(UnitStaticDataKey::from_raw(ui)).type != ut) {
                continue;
            }
            u16 c = 0;
            for (u16 i = 0; i < n; ++i) {
                const UnitAddStruct* u = st.m_units.get_unit_add(keys[i]);
                if (u != nullptr && u->m_unit_typ_idx == ui) {
                    c++;
                }
            }
            if (c == 0u) {
                continue;
            }
            if (!first) {
                std::fprintf(fp, ",");
            }
            first = false;
            std::fprintf(fp, " %s %u", unit_name(*st.m_statics, ui), static_cast<unsigned>(c));
        }
        std::fprintf(fp, "\n");
    }
    std::fprintf(fp, "\n");
}

static bool run_case (
    const RuntimeStatics& rst,
    const u16* land_idx,
    u16 land_n,
    u16 pick_n,
    cstr out_path) 
{
    GameState st;
    st.m_statics = &rst;
    st.m_player_n = 1u;
    bind_land_types(st);
    if (!Factory_GameArraySimple::init_test_grid(&st.m_map, 4u, 4u)) {
        std::printf("FAILED init_test_grid pick_n=%u\n", static_cast<unsigned>(pick_n));
        return false;
    }
    for (u16 y = 0; y < 4u; ++y) {
        for (u16 x = 0; x < 4u; ++x) {
            GameTileSimple* t = st.m_map.tile(x, y);
            t->m_terr = TERR_PLAINS[0];
            t->m_clim = CLIMATE_NONE;
            t->m_ov = OV_NONE[0];
            t->m_riv = 0u;
            t->m_road_typ = ROAD_NONE;
        }
    }
    Rng32 rng;
    rng_seed(&rng, G_SEED);
    const u16 tx = 1u;
    const u16 ty = 1u;
    UnitAddKey placed[G_PICK_MAX];
    u16 pn = 0;
    for (u16 i = 0; i < pick_n; ++i) {
        const u16 pick = land_idx[rng_next(&rng) % land_n];
        UnitAddKey key = UnitAddKey::None();
        if (!UnitMovementMng::place_on_tile(st, tx, ty, 0u, pick, &key)) {
            std::printf("FAILED place_on_tile at %u pick_n=%u\n", static_cast<unsigned>(i), static_cast<unsigned>(pick_n));
            return false;
        }
        placed[pn++] = key;
    }
    double t_min = 0.0;
    double t_max = 0.0;
    double t_sum = 0.0;
    UnitAddKey out_hd[UnitGroupManagement::k_arty_lim];
    u16 out_n = 0;
    UnitAddKey head = UnitAddKey::None();
    if (!UnitMovementMng::form_group_chain(st, placed, pn, &head)) {
        std::printf("FAILED form_group_chain pick_n=%u\n", static_cast<unsigned>(pick_n));
        return false;
    }
    for (u16 ti = 0; ti < G_TIME_N; ++ti) {
        if (ti > 0u) {
            if (!UnitMovementMng::flatten_groups(st, out_hd, out_n, &head)) {
                std::printf("FAILED flatten_groups pick_n=%u\n", static_cast<unsigned>(pick_n));
                return false;
            }
            for (u16 i = 0; i < pn; ++i) {
                UnitAddStruct* u = st.m_units.get_unit_add(placed[i]);
                if (u == nullptr) {
                    std::printf("FAILED placed null pick_n=%u\n", static_cast<unsigned>(pick_n));
                    return false;
                }
                u->m_next_unit_in_group = U16_KEY_NULL;
                if (u->m_x == U16_KEY_NULL || u->m_y == U16_KEY_NULL) {
                    if (!UnitMovementMng::stack_append(st, placed[i], tx, ty)) {
                        std::printf("FAILED stack_append pick_n=%u\n", static_cast<unsigned>(pick_n));
                        return false;
                    }
                }
            }
            if (!UnitMovementMng::form_group_chain(st, placed, pn, &head)) {
                std::printf("FAILED form_group_chain re pick_n=%u\n", static_cast<unsigned>(pick_n));
                return false;
            }
        }
        const auto t0 = std::chrono::steady_clock::now();
        if (!UnitGroupManagement::partition_land_armies_by_arty(
                st, head, out_hd, UnitGroupManagement::k_arty_lim, &out_n)) {
            std::printf("FAILED partition_land_armies_by_arty pick_n=%u\n", static_cast<unsigned>(pick_n));
            return false;
        }
        const auto t1 = std::chrono::steady_clock::now();
        const double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        if (ti == 0u) {
            t_min = us;
            t_max = us;
        } else {
            if (us < t_min) {
                t_min = us;
            }
            if (us > t_max) {
                t_max = us;
            }
        }
        t_sum += us;
    }
    const double t_avg = t_sum / static_cast<double>(G_TIME_N);
    std::printf("pick_n=%u armies=%u partition us: min=%.2f max=%.2f avg=%.2f (n=%u)\n",
        static_cast<unsigned>(pick_n),
        static_cast<unsigned>(out_n),
        t_min,
        t_max,
        t_avg,
        static_cast<unsigned>(G_TIME_N));
    std::FILE* fp = std::fopen(out_path, "w");
    if (fp == nullptr) {
        std::printf("FAILED open %s\n", out_path);
        return false;
    }
    std::fprintf(fp, "seed=%u pick_n=%u land_catalog=%u armies=%u arty_lim=%u\n",
        static_cast<unsigned>(G_SEED),
        static_cast<unsigned>(pick_n),
        static_cast<unsigned>(land_n),
        static_cast<unsigned>(out_n),
        static_cast<unsigned>(UnitGroupManagement::k_arty_lim));
    std::fprintf(fp, "partition_us min=%.2f max=%.2f avg=%.2f samples=%u\n\n",
        t_min, t_max, t_avg, static_cast<unsigned>(G_TIME_N));
    write_breakdown(fp, st, placed, pn, "Total:");
    UnitAddKey left[G_PICK_MAX];
    u16 left_n = 0;
    u16 cur = st.m_map.get_unit_hd(tx, ty);
    while (cur != U16_KEY_NULL && left_n < pick_n) {
        const UnitAddKey k = UnitAddKey::from_raw(cur);
        const UnitAddStruct* u = st.m_units.get_unit_add(k);
        if (u == nullptr) {
            break;
        }
        const u16 next = u->m_next_unit_on_tile;
        bool in_army = false;
        for (u16 ai = 0; ai < out_n && !in_army; ++ai) {
            UnitAddKey g = out_hd[ai];
            while (g.is_valid()) {
                if (g == k) {
                    in_army = true;
                    break;
                }
                const UnitAddStruct* gu = st.m_units.get_unit_add(g);
                if (gu == nullptr || gu->m_next_unit_in_group == U16_KEY_NULL) {
                    break;
                }
                g = UnitAddKey::from_raw(gu->m_next_unit_in_group);
            }
        }
        if (!in_army) {
            left[left_n++] = k;
        }
        cur = next;
    }
    if (left_n > 0u) {
        write_breakdown(fp, st, left, left_n, "Left behind (unassigned):");
    }
    for (u16 ai = 0; ai < out_n; ++ai) {
        UnitAddKey gk[G_PICK_MAX];
        u16 gn = 0;
        collect_grp(st, out_hd[ai], gk, pick_n, &gn);
        char title[64];
        bool is_none = false;
        if (gn > 0u && st.m_statics != nullptr) {
            const UnitAddStruct* gu = st.m_units.get_unit_add(gk[0]);
            const u16 un = st.m_statics->unit().get_item_count();
            if (gu != nullptr && gu->m_unit_typ_idx < un) {
                const u16 role = st.m_statics->unit().get_item(UnitStaticDataKey::from_raw(gu->m_unit_typ_idx)).role;
                is_none = role == static_cast<u16>(UnitRole::NONE);
            }
        }
        if (is_none) {
            std::snprintf(title, sizeof(title), "NONE group:");
        } else {
            std::snprintf(title, sizeof(title), "Group %u:", static_cast<unsigned>(ai + 1u));
        }
        write_breakdown(fp, st, gk, gn, title);
    }
    std::fclose(fp);
    std::printf("wrote %s (groups=%u)\n", out_path, static_cast<unsigned>(out_n));
    return true;
}

static bool run_link_check (
    const RuntimeStatics& rst,
    const u16* land_idx,
    u16 land_n,
    u16 pick_n) 
{
    if (pick_n == 0u || pick_n > G_PICK_MAX) {
        std::printf("FAILED link_check bad pick_n=%u\n", static_cast<unsigned>(pick_n));
        return false;
    }
    GameState st;
    st.m_statics = &rst;
    st.m_player_n = 1u;
    bind_land_types(st);
    if (!Factory_GameArraySimple::init_test_grid(&st.m_map, 4u, 4u)) {
        std::printf("FAILED link_check init_test_grid pick_n=%u\n", static_cast<unsigned>(pick_n));
        return false;
    }
    for (u16 y = 0; y < 4u; ++y) {
        for (u16 x = 0; x < 4u; ++x) {
            GameTileSimple* t = st.m_map.tile(x, y);
            t->m_terr = TERR_PLAINS[0];
            t->m_clim = CLIMATE_NONE;
            t->m_ov = OV_NONE[0];
            t->m_riv = 0u;
            t->m_road_typ = ROAD_NONE;
        }
    }
    Rng32 rng;
    rng_seed(&rng, G_SEED);
    const u16 tx = 1u;
    const u16 ty = 1u;
    UnitAddKey placed[G_PICK_MAX];
    u16 pn = 0;
    for (u16 i = 0; i < pick_n; ++i) {
        const u16 pick = land_idx[rng_next(&rng) % land_n];
        UnitAddKey key = UnitAddKey::None();
        if (!UnitMovementMng::place_on_tile(st, tx, ty, 0u, pick, &key)) {
            std::printf("FAILED link_check place at %u pick_n=%u\n", static_cast<unsigned>(i), static_cast<unsigned>(pick_n));
            return false;
        }
        placed[pn++] = key;
    }
    UnitAddKey head = UnitAddKey::None();
    if (!UnitMovementMng::form_group_chain(st, placed, pn, &head)) {
        std::printf("FAILED link_check form_group_chain pick_n=%u\n", static_cast<unsigned>(pick_n));
        return false;
    }
    UnitAddKey out_hd[UnitGroupManagement::k_arty_lim];
    u16 out_n = 0;
    if (!UnitGroupManagement::partition_land_armies_by_arty(
            st, head, out_hd, UnitGroupManagement::k_arty_lim, &out_n)) {
        std::printf("FAILED link_check partition pick_n=%u\n", static_cast<unsigned>(pick_n));
        return false;
    }
    if (out_n == 0u) {
        std::printf("FAILED link_check no groups pick_n=%u\n", static_cast<unsigned>(pick_n));
        return false;
    }
    bool hit[G_PICK_MAX];
    for (u16 i = 0; i < pn; ++i) {
        hit[i] = false;
    }
    u16 reached = 0;
    for (u16 gi = 0; gi < out_n; ++gi) {
        UnitAddKey cur = out_hd[gi];
        if (!cur.is_valid()) {
            std::printf("FAILED link_check null head g=%u pick_n=%u\n",
                static_cast<unsigned>(gi), static_cast<unsigned>(pick_n));
            return false;
        }
        u16 steps = 0;
        while (cur.is_valid()) {
            if (steps >= pick_n) {
                std::printf("FAILED link_check cycle/overflow g=%u pick_n=%u\n",
                    static_cast<unsigned>(gi), static_cast<unsigned>(pick_n));
                return false;
            }
            const UnitAddStruct* u = st.m_units.get_unit_add(cur);
            if (u == nullptr) {
                std::printf("FAILED link_check missing unit g=%u pick_n=%u\n",
                    static_cast<unsigned>(gi), static_cast<unsigned>(pick_n));
                return false;
            }
            u16 pi = U16_KEY_NULL;
            for (u16 i = 0; i < pn; ++i) {
                if (placed[i] == cur) {
                    pi = i;
                    break;
                }
            }
            if (pi == U16_KEY_NULL) {
                std::printf("FAILED link_check unknown key g=%u pick_n=%u\n",
                    static_cast<unsigned>(gi), static_cast<unsigned>(pick_n));
                return false;
            }
            if (hit[pi]) {
                std::printf("FAILED link_check duplicate key g=%u pick_n=%u\n",
                    static_cast<unsigned>(gi), static_cast<unsigned>(pick_n));
                return false;
            }
            hit[pi] = true;
            reached = static_cast<u16>(reached + 1u);
            steps = static_cast<u16>(steps + 1u);
            if (u->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
        }
    }
    if (reached != pick_n) {
        std::printf("FAILED link_check reached=%u expected=%u groups=%u\n",
            static_cast<unsigned>(reached),
            static_cast<unsigned>(pick_n),
            static_cast<unsigned>(out_n));
        return false;
    }
    for (u16 i = 0; i < pn; ++i) {
        if (!hit[i]) {
            std::printf("FAILED link_check unreachable i=%u pick_n=%u\n",
                static_cast<unsigned>(i), static_cast<unsigned>(pick_n));
            return false;
        }
    }
    std::printf("link_check ok pick_n=%u groups=%u reached=%u\n",
        static_cast<unsigned>(pick_n),
        static_cast<unsigned>(out_n),
        static_cast<unsigned>(reached));
    return true;
}

//================================================================================================================================
//= Main =
//================================================================================================================================

int main () {
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("FAILED load statics\n");
        return 1;
    }
    const RuntimeStatics& rst = loader.statics();
    if (!UnitUtilityHelper::setup(rst)) {
        std::printf("FAILED UnitUtilityHelper::setup\n");
        return 1;
    }
    if (!ensure_dir(G_OUT_DIR)) {
        std::printf("FAILED ensure out dir\n");
        return 1;
    }
    const u16 un = rst.unit().get_item_count();
    u16 land_idx[4096];
    u16 land_n = 0;
    for (u16 i = 0; i < un && land_n < 4096u; ++i) {
        const UnitStaticDataStruct& u = rst.unit().get_item(UnitStaticDataKey::from_raw(i));
        if (u.domain == static_cast<u16>(UnitDomain::LAND)) {
            land_idx[land_n++] = i;
        }
    }
    if (land_n == 0u) {
        std::printf("FAILED no LAND units in catalog\n");
        return 1;
    }
    if (!run_case(rst, land_idx, land_n, G_PICK_1000, G_OUT_1000)) {
        UnitUtilityHelper::clear();
        return 1;
    }
    if (!run_case(rst, land_idx, land_n, G_PICK_3000, G_OUT_3000)) {
        UnitUtilityHelper::clear();
        return 1;
    }
    if (!run_link_check(rst, land_idx, land_n, G_PICK_1000)) {
        UnitUtilityHelper::clear();
        return 1;
    }
    if (!run_link_check(rst, land_idx, land_n, G_PICK_3000)) {
        UnitUtilityHelper::clear();
        return 1;
    }
    (void)type_name;
    UnitUtilityHelper::clear();
    return 0;
}

//================================================================================================================================
//= End =
//================================================================================================================================
