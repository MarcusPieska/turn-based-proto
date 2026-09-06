//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "siege_snapshot.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#include "building_static_key.h"
#include "city.h"
#include "city_array.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "general_bit_bank.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static const u16 k_unit_cap = 512u;

static bool mkdir_p (const char* dir) {
    return ::mkdir(dir, 0755) == 0 || errno == EEXIST;
}

static City* city_at (GameState& s, u16 x, u16 y) {
    if (s.m_map.get_add_typ(x, y) != BUILD_ADD_CITY) {
        return nullptr;
    }
    return s.m_cities.get_city(s.m_map.get_add_idx(x, y));
}

static const City* city_at_c (const GameState& s, u16 x, u16 y) {
    if (s.m_map.get_add_typ(x, y) != BUILD_ADD_CITY) {
        return nullptr;
    }
    return s.m_cities.get_city(s.m_map.get_add_idx(x, y));
}

static u16 find_unit_typ (const RuntimeStatics& st, const char* name) {
    if (name == nullptr) {
        return U16_KEY_NULL;
    }
    const u16 un = st.unit().get_item_count();
    for (u16 i = 0; i < un; ++i) {
        const char* nm = st.unit().get_name(UnitStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, name) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static u16 find_bld_typ (const RuntimeStatics& st, const char* name) {
    if (name == nullptr) {
        return U16_KEY_NULL;
    }
    const u16 n = st.building().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        const char* nm = st.building().get_name(BuildingStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, name) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static bool clear_tile (GameState& s, u16 x, u16 y) {
    for (;;) {
        const u16 hd = s.m_map.get_unit_hd(x, y);
        if (hd == U16_KEY_NULL) {
            return true;
        }
        if (!UnitMovementMng::destroy_unit(s, UnitAddKey::from_raw(hd))) {
            return false;
        }
    }
}

static bool clear_city_bld (GameState& s, u16 city_idx) {
    if (s.m_statics == nullptr) {
        return false;
    }
    GeneralBitBank* bank = s.m_cities.get_bld_bank();
    if (bank == nullptr) {
        return false;
    }
    const u16 bn = s.m_statics->building().get_item_count();
    for (u16 i = 0; i < bn; ++i) {
        if (bank->is_flagged(city_idx, i)) {
            bank->clear_flag(city_idx, i);
        }
    }
    return true;
}

//================================================================================================================================
//=> - SiegeSnapshot -
//================================================================================================================================

bool SiegeSnapshot::ensure_dir (const char* dir) {
    return dir != nullptr && mkdir_p(dir);
}

bool SiegeSnapshot::save (
    const GameState& s,
    UnitAddKey army_hd,
    u16 city_x,
    u16 city_y,
    u16 def_seat,
    const char* path)
{
    if (path == nullptr || s.m_statics == nullptr || !army_hd.is_valid()) {
        return false;
    }
    const City* city = city_at_c(s, city_x, city_y);
    if (city == nullptr) {
        return false;
    }
    const UnitAddStruct* head = s.m_units.get_unit_add(army_hd);
    if (head == nullptr || head->m_x == U16_KEY_NULL) {
        return false;
    }
    const u16 ax = static_cast<u16>(head->m_x);
    const u16 ay = static_cast<u16>(head->m_y);
    const u16 city_idx = s.m_map.get_add_idx(city_x, city_y);
    const GeneralBitBank* bank = s.m_cities.get_bld_bank();
    if (bank == nullptr) {
        return false;
    }

    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "CITY %u %u %u %u\n",
        static_cast<unsigned>(city_x),
        static_cast<unsigned>(city_y),
        static_cast<unsigned>(city->get_owner()),
        static_cast<unsigned>(city->get_defense_deduction()));

    const u16 bn = s.m_statics->building().get_item_count();
    u16 bld_n = 0;
    for (u16 i = 0; i < bn; ++i) {
        if (bank->is_flagged(city_idx, i)) {
            bld_n++;
        }
    }
    std::fprintf(fp, "BLD %u\n", static_cast<unsigned>(bld_n));
    for (u16 i = 0; i < bn; ++i) {
        if (!bank->is_flagged(city_idx, i)) {
            continue;
        }
        const char* nm = s.m_statics->building().get_name(BuildingStaticDataKey::from_raw(i));
        if (nm == nullptr) {
            nm = "?";
        }
        std::fprintf(fp, "%s\n", nm);
    }

    std::fprintf(fp, "ATK_TILE %u %u\n",
        static_cast<unsigned>(ax), static_cast<unsigned>(ay));

    u16 atk_n = 0;
    UnitAddKey cur = army_hd;
    while (cur.is_valid() && atk_n < k_unit_cap) {
        const UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        atk_n++;
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    std::fprintf(fp, "ATK %u\n", static_cast<unsigned>(atk_n));
    cur = army_hd;
    for (u16 i = 0; i < atk_n; ++i) {
        const UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr) {
            std::fclose(fp);
            return false;
        }
        const char* nm = s.m_statics->unit().get_name(UnitStaticDataKey::from_raw(u->m_unit_typ_idx));
        if (nm == nullptr) {
            nm = "?";
        }
        std::fprintf(fp, "%u:%s:%u:%u:%d\n",
            static_cast<unsigned>(u->m_player_idx),
            nm,
            static_cast<unsigned>(u->m_health),
            static_cast<unsigned>(u->m_level),
            static_cast<int>(u->m_mvt_points));
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }

    u16 def_n = 0;
    const u32 scan_n = static_cast<u32>(UnitAddVector::MAX_PAGES)
        * static_cast<u32>(UnitAddVector::UNIT_ADD_ITEMS_PER_PAGE);
    for (u32 idx = 0; idx < scan_n && def_n < k_unit_cap; ++idx) {
        const UnitAddStruct* u = s.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
        if (u == nullptr || u->m_x != city_x || u->m_y != city_y || u->m_player_idx != def_seat) {
            continue;
        }
        def_n++;
    }
    std::fprintf(fp, "DEF %u\n", static_cast<unsigned>(def_n));
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const UnitAddStruct* u = s.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
        if (u == nullptr || u->m_x != city_x || u->m_y != city_y || u->m_player_idx != def_seat) {
            continue;
        }
        const char* nm = s.m_statics->unit().get_name(UnitStaticDataKey::from_raw(u->m_unit_typ_idx));
        if (nm == nullptr) {
            nm = "?";
        }
        std::fprintf(fp, "%u:%s:%u:%u:%d\n",
            static_cast<unsigned>(u->m_player_idx),
            nm,
            static_cast<unsigned>(u->m_health),
            static_cast<unsigned>(u->m_level),
            static_cast<int>(u->m_mvt_points));
    }

    std::fclose(fp);
    return true;
}

bool SiegeSnapshot::apply (
    GameState& s,
    const char* path,
    UnitAddKey* out_army,
    u16* out_cx,
    u16* out_cy,
    u16* out_def)
{
    if (path == nullptr || s.m_statics == nullptr || out_army == nullptr
        || out_cx == nullptr || out_cy == nullptr || out_def == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "r");
    if (fp == nullptr) {
        return false;
    }

    unsigned cx = 0;
    unsigned cy = 0;
    unsigned owner = 0;
    unsigned ded = 0;
    if (std::fscanf(fp, "CITY %u %u %u %u\n", &cx, &cy, &owner, &ded) != 4) {
        std::fclose(fp);
        return false;
    }
    City* city = city_at(s, static_cast<u16>(cx), static_cast<u16>(cy));
    if (city == nullptr) {
        std::fclose(fp);
        return false;
    }
    const u16 city_idx = s.m_map.get_add_idx(static_cast<u16>(cx), static_cast<u16>(cy));
    city->set_owner(static_cast<u16>(owner));
    city->set_defense_deduction(static_cast<u16>(ded > 65535u ? 65535u : ded));
    if (!clear_city_bld(s, city_idx)) {
        std::fclose(fp);
        return false;
    }
    GeneralBitBank* bank = s.m_cities.get_bld_bank();
    if (bank == nullptr) {
        std::fclose(fp);
        return false;
    }

    unsigned bld_n = 0;
    if (std::fscanf(fp, "BLD %u\n", &bld_n) != 1) {
        std::fclose(fp);
        return false;
    }
    for (unsigned i = 0; i < bld_n; ++i) {
        char name[128];
        if (std::fscanf(fp, "%127[^\n]\n", name) != 1) {
            std::fclose(fp);
            return false;
        }
        const u16 typ = find_bld_typ(*s.m_statics, name);
        if (typ == U16_KEY_NULL) {
            std::fclose(fp);
            return false;
        }
        bank->set_flag(city_idx, typ);
    }

    unsigned ax = 0;
    unsigned ay = 0;
    if (std::fscanf(fp, "ATK_TILE %u %u\n", &ax, &ay) != 2) {
        std::fclose(fp);
        return false;
    }
    if (!clear_tile(s, static_cast<u16>(cx), static_cast<u16>(cy))) {
        std::fclose(fp);
        return false;
    }
    if (!clear_tile(s, static_cast<u16>(ax), static_cast<u16>(ay))) {
        std::fclose(fp);
        return false;
    }

    unsigned atk_n = 0;
    if (std::fscanf(fp, "ATK %u\n", &atk_n) != 1) {
        std::fclose(fp);
        return false;
    }
    UnitAddKey army = UnitAddKey::None();
    for (unsigned i = 0; i < atk_n; ++i) {
        unsigned player = 0;
        unsigned hp = 0;
        unsigned lvl = 0;
        int mvt = 0;
        char name[128];
        if (std::fscanf(fp, "%u:%127[^:]:%u:%u:%d\n", &player, name, &hp, &lvl, &mvt) != 5) {
            std::fclose(fp);
            return false;
        }
        const u16 typ = find_unit_typ(*s.m_statics, name);
        if (typ == U16_KEY_NULL) {
            std::fclose(fp);
            return false;
        }
        UnitAddKey key = UnitAddKey::None();
        if (!UnitMovementMng::place_on_tile(s, static_cast<u16>(ax), static_cast<u16>(ay),
                static_cast<u16>(player), typ, &key)) {
            std::fclose(fp);
            return false;
        }
        UnitAddStruct* u = s.m_units.get_unit_add(key);
        if (u == nullptr) {
            std::fclose(fp);
            return false;
        }
        u->m_health = static_cast<u8>(hp > 255u ? 255u : hp);
        u->m_level = static_cast<u8>(lvl > 255u ? 255u : lvl);
        u->m_mvt_points = static_cast<i16>(mvt);
        if (!army.is_valid()) {
            army = key;
        } else if (!UnitMovementMng::link_group(s, army, key)) {
            std::fclose(fp);
            return false;
        }
    }

    unsigned def_n = 0;
    if (std::fscanf(fp, "DEF %u\n", &def_n) != 1) {
        std::fclose(fp);
        return false;
    }
    u16 def_seat = U16_KEY_NULL;
    for (unsigned i = 0; i < def_n; ++i) {
        unsigned player = 0;
        unsigned hp = 0;
        unsigned lvl = 0;
        int mvt = 0;
        char name[128];
        if (std::fscanf(fp, "%u:%127[^:]:%u:%u:%d\n", &player, name, &hp, &lvl, &mvt) != 5) {
            std::fclose(fp);
            return false;
        }
        const u16 typ = find_unit_typ(*s.m_statics, name);
        if (typ == U16_KEY_NULL) {
            std::fclose(fp);
            return false;
        }
        UnitAddKey key = UnitAddKey::None();
        if (!UnitMovementMng::place_on_tile(s, static_cast<u16>(cx), static_cast<u16>(cy),
                static_cast<u16>(player), typ, &key)) {
            std::fclose(fp);
            return false;
        }
        UnitAddStruct* u = s.m_units.get_unit_add(key);
        if (u == nullptr) {
            std::fclose(fp);
            return false;
        }
        u->m_health = static_cast<u8>(hp > 255u ? 255u : hp);
        u->m_level = static_cast<u8>(lvl > 255u ? 255u : lvl);
        u->m_mvt_points = static_cast<i16>(mvt);
        def_seat = static_cast<u16>(player);
    }

    std::fclose(fp);
    *out_army = army;
    *out_cx = static_cast<u16>(cx);
    *out_cy = static_cast<u16>(cy);
    *out_def = def_seat;
    return army.is_valid();
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
