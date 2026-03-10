//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TECH_DATA_TYPES_H
#define TECH_DATA_TYPES_H

#include <cstdint>

#include "game_primitives.h"

//================================================================================================================================
//=> - Tech data types -
//================================================================================================================================

class TechIdx {
    friend class TechData;
    public:
        TechIdx(const TechIdx&) = default;
        TechIdx& operator=(const TechIdx&) = default;
        ~TechIdx() = default;

        TechIdx() : m_idx(0) {
        }

        u32 get_idx() const { 
            return m_idx; 
        }
        
        bool operator==(const TechIdx& other) const { 
            return m_idx == other.m_idx; 
        }
    
    private:
        TechIdx(u16 idx) : m_idx(idx) {
        }

        u16 m_idx;
};

#define MAX_TECHS_PER_ENTITY 4

typedef struct TechIndices {
    TechIdx indices[MAX_TECHS_PER_ENTITY];
} TechIndices;

#endif // TECH_DATA_TYPES_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================

