//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_H
#define CITY_H

#include <cstdint>

#include "game_primitives.h"
#include "building_vector.h"

class BitArrayCL;

class BuildableWonders;
class BuiltWonders;

class BuildableSmallWonders;
class BuiltSmallWonders;

class BuildableUnits;
class BuiltUnits;

//================================================================================================================================
//=> - City class -
//================================================================================================================================

class City {
public:
    City ();
    ~City ();

    BuildableBuildings* get_buildable_buildings (BitArrayCL* techs);
    BuildableWonders* get_buildable_wonders (BitArrayCL* techs);
    BuildableSmallWonders* get_buildable_small_wonders (BitArrayCL* techs, BuiltSmallWonders* built);
    BuildableUnits* get_trainable_units (BitArrayCL* techs);

    void build_building (u16 building_idx);
    void build_wonder (u16 wonder_idx);
    void build_small_wonder (u16 small_wonder_idx);
    void build_unit (u16 unit_idx);

    void add_food (u16 amount);
    void add_shields (u16 amount);

    bool is_build_done () const;
    bool is_ready_to_grow () const;

private:
    u16 m_accumulated_food;
    u16 m_accumulated_shields;
    u8 m_build_type;
    u16 m_bld_idx;

    BitArrayCL m_flags;
    BuiltBuildings m_buildings;
    BitArrayCL m_resources;
};

#endif // CITY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================