//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDING_DATA_TYPES_H
#define BUILDING_DATA_TYPES_H

#include <cstdint>

#include "game_primitives.h"

//================================================================================================================================
//=> - Building data types -
//================================================================================================================================

class BuildingIdx {
    friend class BuildingData;
    public:
        BuildingIdx(const BuildingIdx&) = default;
        BuildingIdx& operator=(const BuildingIdx&) = default;
        ~BuildingIdx() = default;

        BuildingIdx() : m_idx(0) {
        }

        u32 get_idx() const { 
            return m_idx; 
        }
        
        bool operator==(const BuildingIdx& other) const { 
            return m_idx == other.m_idx; 
        }
    
    private:
        BuildingIdx(u16 idx) : m_idx(idx) {
        }

        u16 m_idx;
};

#endif // BUILDING_DATA_TYPES_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================

