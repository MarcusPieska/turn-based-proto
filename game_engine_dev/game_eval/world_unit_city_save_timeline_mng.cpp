//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "world_unit_city_save_timeline_mng.h"

#include "city.h"
#include "city_array.h"
#include "eval_paths.h"
#include "game_io.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"

//================================================================================================================================
//=> - WorldUnitCitySaveTimelineMng -
//================================================================================================================================

WorldUnitCitySaveTimelineMng::WorldUnitCitySaveTimelineMng ()
    : m_save_n(0) {
}

WorldUnitCitySaveTimelineMng::~WorldUnitCitySaveTimelineMng () {
    clr();
}

void WorldUnitCitySaveTimelineMng::clr () {
    m_tl.clr();
    m_save_n = 0;
}

bool WorldUnitCitySaveTimelineMng::setup (u16 save_n) {
    clr();
    if (save_n == 0) {
        return false;
    }
    if (!m_tl.setup(save_n)) {
        return false;
    }
    m_save_n = save_n;
    return true;
}

bool WorldUnitCitySaveTimelineMng::fill (const EvalPaths& paths) {
    if (m_save_n == 0 || !paths.ok() || paths.save_turn_n() != m_save_n) {
        return false;
    }
    for (u16 si = 0; si < m_save_n; ++si) {
        const u32 turn = paths.save_turn_at(si);
        char units_p[512];
        char cities_p[512];
        if (!paths.units_path(turn, units_p, sizeof(units_p))
            || !paths.cities_path(turn, cities_p, sizeof(cities_p))) {
            return false;
        }
        u32 unit_tot = 0;
        u32 city_tot = 0;
        {
            UnitAddVector units;
            CityArray cities;
            if (!GameIo::load_units(units_p, units) || !GameIo::load_cities(cities_p, cities)) {
                return false;
            }
            const u16 head = units.get_head_unit_add_idx();
            for (u16 k = 0; k < head; ++k) {
                const UnitAddStruct* u = units.get_unit_add(UnitAddKey::from_raw(k));
                if (u == nullptr || u->m_x == U16_KEY_NULL) {
                    continue;
                }
                unit_tot = unit_tot + 1u;
            }
            city_tot = cities.get_city_count();
        }
        if (!m_tl.set(si, unit_tot, city_tot)) {
            return false;
        }
    }
    return true;
}

u16 WorldUnitCitySaveTimelineMng::save_n () const {
    return m_save_n;
}

WorldUnitCitySaveTimeline& WorldUnitCitySaveTimelineMng::tl () {
    return m_tl;
}

const WorldUnitCitySaveTimeline& WorldUnitCitySaveTimelineMng::tl () const {
    return m_tl;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
