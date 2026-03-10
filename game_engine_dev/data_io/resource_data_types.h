//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RESOURCE_DATA_TYPES_H
#define RESOURCE_DATA_TYPES_H

#include <cstdint>

#include "game_primitives.h"

//================================================================================================================================
//=> - Tech data types -
//================================================================================================================================

class ResourceIdx {
    friend class ResourceData;
    public:
        ResourceIdx(const ResourceIdx&) = default;
        ResourceIdx& operator=(const ResourceIdx&) = default;
        ~ResourceIdx() = default;

        ResourceIdx() : m_idx(0) {
        }

        u32 get_idx() const { 
            return m_idx; 
        }
        
        bool operator==(const ResourceIdx& other) const { 
            return m_idx == other.m_idx; 
        }
    
    private:
        ResourceIdx(u16 idx) : m_idx(idx) {
        }

        u16 m_idx;
};

#define MAX_RESOURCES_PER_ENTITY 4

typedef struct ResourceIndices {
    ResourceIdx indices[MAX_RESOURCES_PER_ENTITY];
} ResourceIndices;

#endif // RESOURCE_DATA_TYPES_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================

