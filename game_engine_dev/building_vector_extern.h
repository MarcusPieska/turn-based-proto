//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDING_VECTOR_EXTERN_H
#define BUILDING_VECTOR_EXTERN_H

#include "building_vector.h"
#include <cstring>

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
            g_BuildingVectorInstance = new BuildingVector(static_cast<uint32_t>(count));
        }
    }
}

void DestroyBuildingVector () {
    delete g_BuildingVectorInstance;
    g_BuildingVectorInstance = nullptr;
}

int GetBuildingCount () {
    if (g_BuildingVectorInstance) {
        return g_BuildingVectorInstance->get_count();
    }
    return 0;
}

void GetBuilding (int index, BuildingDataC* out_data) {
    if (g_BuildingVectorInstance && out_data) {
        BuildingData data = g_BuildingVectorInstance->get_building(index);
        strncpy(out_data->name, data.name.c_str(), sizeof(out_data->name) - 1);
        out_data->name[sizeof(out_data->name) - 1] = '\0';
        out_data->cost = data.cost;
        strncpy(out_data->effect, data.effect.c_str(), sizeof(out_data->effect) - 1);
        out_data->effect[sizeof(out_data->effect) - 1] = '\0';
        out_data->exists = data.exists ? 1 : 0;
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

void SetAvailable (int index) {
    if (g_BuildingVectorInstance) {
        g_BuildingVectorInstance->set_available(index);
    }
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
