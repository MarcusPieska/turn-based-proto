//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RESOURCES_VECTOR_EXTERN_H
#define RESOURCES_VECTOR_EXTERN_H

#include <cstring>
#include <cstdint>

#include "resources_vector.h"
#include "bit_array.h"

//================================================================================================================================
//=> - C-compatible ResourceData struct -
//================================================================================================================================

struct ResourceDataC {
    char name[256];
    int food;
    int shields;
    int income;
};

//================================================================================================================================
//=> - Global instance -
//================================================================================================================================

inline ResourceVector* g_ResourceVectorInstance = nullptr;
inline BitArrayCL* g_ResourcesAvailable = nullptr;

//================================================================================================================================
//=> - C interface -
//================================================================================================================================

extern "C" {

void InitResourceData (const char* filename) {
    if (g_ResourceVectorInstance == nullptr) {
        ResourceIO io(filename);
        io.parse_and_allocate();
        int count = io.validate_and_count();
        if (count > 0) {
            g_ResourcesAvailable = new BitArrayCL(static_cast<uint32_t>(count));
            g_ResourceVectorInstance = new ResourceVector(static_cast<uint32_t>(count), g_ResourcesAvailable);
        }
    }
}

void DestroyResourceVector () {
    delete g_ResourceVectorInstance;
    delete g_ResourcesAvailable;
    g_ResourceVectorInstance = nullptr;
    g_ResourcesAvailable = nullptr;
}

int GetResourceCount () {
    if (g_ResourceVectorInstance) {
        return g_ResourceVectorInstance->get_count();
    }
    return 0;
}

void GetResource (int index, ResourceDataC* out_data) {
    if (g_ResourceVectorInstance && out_data) {
        ResourceData data = g_ResourceVectorInstance->get_resource(static_cast<uint32_t>(index));
        strncpy(out_data->name, data.stats.name.c_str(), sizeof(out_data->name) - 1);
        out_data->name[sizeof(out_data->name) - 1] = '\0';
        out_data->food = static_cast<int>(data.stats.food);
        out_data->shields = static_cast<int>(data.stats.shields);
        out_data->income = static_cast<int>(data.stats.income);
    } else {
        if (out_data) {
            out_data->name[0] = '\0';
            out_data->food = 0;
            out_data->shields = 0;
            out_data->income = 0;
        }
    }
}

void SetAvailable (int index) {
    if (g_ResourceVectorInstance) {
        g_ResourceVectorInstance->set_available(index);
    }
}

int ValidateAndCount (const char* filename) {
    ResourceIO io(filename);
    return io.validate_and_count();
}

}

#endif // RESOURCES_VECTOR_EXTERN_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
