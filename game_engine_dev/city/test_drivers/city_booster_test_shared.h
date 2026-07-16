//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_BOOSTER_TEST_SHARED_H
#define CITY_BOOSTER_TEST_SHARED_H

#include <cstdio>
#include <cstring>

#include "bit_array.h"
#include "booster_apply.h"
#include "building_static_key.h"
#include "city.h"
#include "city_array.h"
#include "factory_game_array_simple.h"
#include "game_state.h"
#include "player_ledger.h"
#include "runtime_static_loader.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"
#include "wonder_static_key.h"

//================================================================================================================================
//=> - Miscellaneous -
//================================================================================================================================

typedef const char* cstr;

struct CityBoosterTestEnv {
    RuntimeStaticLoader m_loader;
    RuntimeStatics* m_st = nullptr;
    CityArray m_array;
    GameState m_state = {};
    PlayerState m_seat = {};
    BitArrayCL* m_techs = nullptr;
    u16* m_wonder_city = nullptr;
    u16 m_city_idx = U16_KEY_NULL;

    ~CityBoosterTestEnv () {
        delete m_techs;
        delete[] m_wonder_city;
        delete[] m_seat.m_small_wonder_city;
        PlayerLedger::bind_state(nullptr);
        UnitMovementMng::bind_state(nullptr);
        City::bind_wonder_cities(nullptr);
        City::bind_player_states(nullptr, 0);
        City::bind_units(nullptr);
    }

    bool bind () {
        if (!m_loader.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
            std::printf("ERROR: failed to load runtime statics\n");
            return false;
        }
        m_st = &m_loader.statics();
        if (!m_array.bind_statics(*m_st)) {
            return false;
        }
        const u16 tech_n = m_st->tech().get_item_count();
        const u16 wonder_n = m_st->wonder().get_item_count();
        const u16 sw_n = m_st->small_wonder().get_item_count();
        m_techs = new BitArrayCL(tech_n);
        m_wonder_city = new u16[wonder_n];
        for (u16 i = 0; i < wonder_n; ++i) {
            m_wonder_city[i] = U16_KEY_NULL;
        }
        m_seat = {};
        if (sw_n > 0) {
            m_seat.m_small_wonder_city = new u16[sw_n];
            for (u16 i = 0; i < sw_n; ++i) {
                m_seat.m_small_wonder_city[i] = U16_KEY_NULL;
            }
        }
        m_seat.m_techs_researched = m_techs;
        m_state.m_statics = m_st;
        m_state.m_player_n = 1;
        m_state.m_player_states = &m_seat;
        m_state.m_wonder_city = m_wonder_city;
        m_state.m_wonder_count = wonder_n;
        m_state.m_small_wonder_count = sw_n;
        if (!Factory_GameArraySimple::init_test_grid(&m_state.m_map, 8, 8)) {
            return false;
        }
        City::bind_wonder_cities(m_wonder_city);
        City::bind_player_states(m_state.m_player_states, m_state.m_player_n);
        City::bind_units(&m_state.m_units);
        PlayerLedger::bind_state(&m_state);
        UnitMovementMng::bind_state(&m_state);
        m_city_idx = m_array.get_next_new_city_idx();
        City* city = m_array.get_city(m_city_idx);
        if (city == nullptr) {
            return false;
        }
        city->init(0, 5, 5);
        return true;
    }

    City* city () {
        return m_array.get_city(m_city_idx);
    }

    u16 find_unit_idx (cstr nm) const {
        if (m_st == nullptr || nm == nullptr) {
            return U16_KEY_NULL;
        }
        const u16 n = m_st->unit().get_item_count();
        for (u16 i = 0; i < n; ++i) {
            cstr item_nm = m_st->unit().get_name(UnitStaticDataKey::from_raw(i));
            if (item_nm != nullptr && std::strcmp(item_nm, nm) == 0) {
                return i;
            }
        }
        return U16_KEY_NULL;
    }

    u16 find_bld_idx (cstr nm) const {
        if (m_st == nullptr || nm == nullptr) {
            return U16_KEY_NULL;
        }
        const u16 n = m_st->building().get_item_count();
        for (u16 i = 0; i < n; ++i) {
            cstr item_nm = m_st->building().get_name(BuildingStaticDataKey::from_raw(i));
            if (item_nm != nullptr && std::strcmp(item_nm, nm) == 0) {
                return i;
            }
        }
        return U16_KEY_NULL;
    }

    u16 find_wonder_idx (cstr nm) const {
        if (m_st == nullptr || nm == nullptr) {
            return U16_KEY_NULL;
        }
        const u16 n = m_st->wonder().get_item_count();
        for (u16 i = 0; i < n; ++i) {
            cstr item_nm = m_st->wonder().get_name(WonderStaticDataKey::from_raw(i));
            if (item_nm != nullptr && std::strcmp(item_nm, nm) == 0) {
                return i;
            }
        }
        return U16_KEY_NULL;
    }

    bool finish_unit_build (u16 unit_idx) {
        City* c = city();
        if (c == nullptr || m_st == nullptr) {
            return false;
        }
        c->build_unit(unit_idx);
        const u32 cost = m_st->unit().get_item(UnitStaticDataKey::from_raw(unit_idx)).cost;
        c->add_production(m_city_idx, static_cast<u16>(cost));
        return c->finish_if_ready(m_city_idx);
    }

    const UnitAddStruct* find_spawned_unit (u16 typ, u16 x, u16 y) const {
        const UnitAddStruct* best = nullptr;
        u16 best_key = 0;
        for (u16 k = 0; k < 4096; ++k) {
            const UnitAddStruct* u = m_state.m_units.get_unit_add(UnitAddKey::from_raw(k));
            if (u == nullptr) {
                continue;
            }
            if (u->m_unit_typ_idx != typ || u->m_x != x || u->m_y != y) {
                continue;
            }
            if (best == nullptr || k >= best_key) {
                best_key = k;
                best = u;
            }
        }
        return best;
    }
};

inline void city_booster_note (bool ok, cstr msg) {
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", msg);
}

#endif // CITY_BOOSTER_TEST_SHARED_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
