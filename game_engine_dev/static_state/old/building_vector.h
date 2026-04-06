//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDING_VECTOR_H
#define BUILDING_VECTOR_H

#include <string>
#include <iosfwd>
#include <cstdint>

#include "game_primitives.h"
#include "bit_array.h"
#include "building_data.h"

//================================================================================================================================
//=> - Forward declarations -
//================================================================================================================================

class BitArrayCL;

//================================================================================================================================
//=> - BuiltBuildings class -
//================================================================================================================================

class BuiltBuildings {
public:
    BuiltBuildings ();
    ~BuiltBuildings ();

    void set_built (u32 idx);
    void clear_built (u32 idx);
    bool has_been_built (u32 idx) const;
    
    void save (const std::string& filename) const;
    void load (const std::string& filename);

private:
    BuiltBuildings (const BuiltBuildings& other) = delete;
    BuiltBuildings (BuiltBuildings&& other) = delete;

    BitArrayCL m_built;
};

//================================================================================================================================
//=> - BuildableBuildings class -
//================================================================================================================================

class BuildableBuildings {
public:
    friend class BuildingAssessor;

    BuildableBuildings ();
    ~BuildableBuildings ();

    bool can_build (u32 idx) const;

protected:
    void set_buildable (u32 idx);

private:
    BuildableBuildings (const BuildableBuildings& other) = delete;
    BuildableBuildings (BuildableBuildings&& other) = delete;

    BitArrayCL m_buildable;
};

#endif // BUILDING_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
