//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDING_VECTOR_H
#define BUILDING_VECTOR_H

#include <string>
#include <iosfwd>
#include <cstdint>

#include "game_primitives.h"

//================================================================================================================================
//=> - BuildingTypeStats struct -
//================================================================================================================================

class BitArrayCL;

//================================================================================================================================
//=> - BuildingVector class -
//================================================================================================================================

class BuildingVector {
public:
    friend class BuildableAssessor;

    BuildingVector (const BitArrayCL* researched_buildings);
    ~BuildingVector ();

    const BuildingTypeStats& get_building_stats (u32 index) const;
    bool is_buildable (u32 index) const;
    bool is_built (u32 index) const;
    u32 get_count () const;
    
    void save (const std::string& filename) const;
    void load (const std::string& filename);
    
    void toggle_built (u32 index);
    void set_built (u32 index);
    void clear_built (u32 index);

    static u32 get_building_data_count ();
    static const BuildingTypeStats* get_building_data_array ();

protected:
    void set_buildable (u32 index);

private:
    BuildingVector (const BuildingVector& other) = delete;
    BuildingVector (BuildingVector&& other) = delete;
    BuildingVector () = delete;

    const BitArrayCL* m_bld_researched;
    BitArrayCL* m_bld_unlocked;
    BitArrayCL* m_bld_built;
};

#endif // BUILDING_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
