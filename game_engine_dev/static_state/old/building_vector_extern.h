//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDING_VECTOR_EXTERN_H
#define BUILDING_VECTOR_EXTERN_H

#include <cstring>

#include "building_vector.h"
#include "bit_array.h"

//================================================================================================================================
//=> - Mock BuildableAssessor class -
//================================================================================================================================

class BuildableAssessor {
public:
    void set_buildable (BuildingVector* bv, uint32_t index) {
        if (bv) {
            bv->set_buildable(index);
        }
    }
    
    void clear_buildable (BuildingVector* bv, uint32_t index) {
        if (bv && index < bv->m_bld_unlocked->get_count()) {
            bv->m_bld_unlocked->clear_bit(index);
        }
    }
    
    void toggle_buildable (BuildingVector* bv, uint32_t index) {
        if (bv && index < bv->m_bld_unlocked->get_count()) {
            if (bv->m_bld_unlocked->get_bit(index) == 1) {
                bv->m_bld_unlocked->clear_bit(index);
            } else {
                bv->m_bld_unlocked->set_bit(index);
            }
        }
    }
    
    int is_unlocked (BuildingVector* bv, uint32_t index) {
        if (bv && index < bv->m_bld_unlocked->get_count()) {
            return bv->m_bld_unlocked->get_bit(index);
        }
        return 0;
    }
};

//================================================================================================================================
//=> - C-compatible BuildingData struct -
//================================================================================================================================

struct BuildingDataC {
    char name[256];
    int cost;
    char effect[256];
    int exists;
};

//================================================================================================================================
//=> - Global instance -
//================================================================================================================================

inline BuildingVector* g_BuildingVectorInstance = nullptr;
inline BitArrayCL* g_ResearchedBuildings = nullptr;
inline BuildableAssessor* g_MockAssessor = nullptr;

//================================================================================================================================
//=> - C interface -
//================================================================================================================================

extern "C" {

void InitBuildingData (const char* filename) {
    if (g_BuildingVectorInstance == nullptr) {
        BuildingIO io(filename);
        io.parse_and_allocate();
        int count = io.validate_and_count();
        if (count > 0) {
            g_ResearchedBuildings = new BitArrayCL(static_cast<uint32_t>(count));
            g_BuildingVectorInstance = new BuildingVector(g_ResearchedBuildings);
            g_MockAssessor = new BuildableAssessor();
        }
    }
}

void DestroyBuildingVector () {
    delete g_MockAssessor;
    delete g_BuildingVectorInstance;
    delete g_ResearchedBuildings;
    g_MockAssessor = nullptr;
    g_BuildingVectorInstance = nullptr;
    g_ResearchedBuildings = nullptr;
}

int GetBuildingCount () {
    if (g_BuildingVectorInstance) {
        return g_BuildingVectorInstance->get_count();
    }
    return 0;
}

void GetBuilding (int index, BuildingDataC* out_data) {
    if (g_BuildingVectorInstance && out_data) {
        const BuildingTypeStats& stats = g_BuildingVectorInstance->get_building_stats(index);
        strncpy(out_data->name, stats.name.c_str(), sizeof(out_data->name) - 1);
        out_data->name[sizeof(out_data->name) - 1] = '\0';
        out_data->cost = stats.cost;
        strncpy(out_data->effect, stats.effect.c_str(), sizeof(out_data->effect) - 1);
        out_data->effect[sizeof(out_data->effect) - 1] = '\0';
        out_data->exists = g_BuildingVectorInstance->is_built(index) ? 1 : 0;
    } else {
        if (out_data) {
            out_data->name[0] = '\0';
            out_data->cost = 0;
            out_data->effect[0] = '\0';
            out_data->exists = 0;
        }
    }
}

void SaveBuildingVector (const char* filename) {
    if (g_BuildingVectorInstance) {
        g_BuildingVectorInstance->save(filename);
    }
}

void LoadBuildingVector (const char* filename) {
    if (g_BuildingVectorInstance) {
        g_BuildingVectorInstance->load(filename);
    }
}

void ToggleBuilt (int index) {
    if (g_BuildingVectorInstance) {
        g_BuildingVectorInstance->toggle_built(index);
    }
}

void SetBuilt (int index) {
    if (g_BuildingVectorInstance) {
        g_BuildingVectorInstance->set_built(index);
    }
}

void ClearBuilt (int index) {
    if (g_BuildingVectorInstance) {
        g_BuildingVectorInstance->clear_built(index);
    }
}

void ToggleResearched (int index) {
    if (g_ResearchedBuildings) {
        if (g_ResearchedBuildings->get_bit(index) == 1) {
            g_ResearchedBuildings->clear_bit(index);
        } else {
            g_ResearchedBuildings->set_bit(index);
        }
    }
}

void SetResearched (int index) {
    if (g_ResearchedBuildings) {
        g_ResearchedBuildings->set_bit(index);
    }
}

void ClearResearched (int index) {
    if (g_ResearchedBuildings) {
        g_ResearchedBuildings->clear_bit(index);
    }
}

void ToggleUnlocked (int index) {
    if (g_MockAssessor && g_BuildingVectorInstance) {
        g_MockAssessor->toggle_buildable(g_BuildingVectorInstance, static_cast<uint32_t>(index));
    }
}

void SetUnlocked (int index) {
    if (g_MockAssessor && g_BuildingVectorInstance) {
        g_MockAssessor->set_buildable(g_BuildingVectorInstance, static_cast<uint32_t>(index));
    }
}

void ClearUnlocked (int index) {
    if (g_MockAssessor && g_BuildingVectorInstance) {
        g_MockAssessor->clear_buildable(g_BuildingVectorInstance, static_cast<uint32_t>(index));
    }
}

int IsUnlocked (int index) {
    if (g_MockAssessor && g_BuildingVectorInstance) {
        return g_MockAssessor->is_unlocked(g_BuildingVectorInstance, static_cast<uint32_t>(index));
    }
    return 0;
}

int IsBuilt (int index) {
    if (g_BuildingVectorInstance) {
        return g_BuildingVectorInstance->is_built(static_cast<uint32_t>(index)) ? 1 : 0;
    }
    return 0;
}

int IsBuildable (int index) {
    if (g_BuildingVectorInstance) {
        return g_BuildingVectorInstance->is_buildable(static_cast<uint32_t>(index)) ? 1 : 0;
    }
    return 0;
}

int IsResearched (int index) {
    if (g_ResearchedBuildings) {
        return g_ResearchedBuildings->get_bit(index);
    }
    return 0;
}

int ValidateAndCount (const char* filename) {
    BuildingIO io(filename);
    return io.validate_and_count();
}

}

#endif // BUILDING_VECTOR_EXTERN_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
