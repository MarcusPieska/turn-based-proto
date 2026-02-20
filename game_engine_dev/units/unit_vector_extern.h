//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_VECTOR_EXTERN_H
#define UNIT_VECTOR_EXTERN_H

#include <cstring>

#include "unit_vector.h"
#include "bit_array.h"

//================================================================================================================================
//=> - C-compatible UnitData struct -
//================================================================================================================================

struct UnitDataC {
    char name[256];
    int cost;
    int attack;
    int defense;
    int moves;
    int strength;
};

//================================================================================================================================
//=> - Global instance -
//================================================================================================================================

inline UnitVector* g_UnitVectorInstance = nullptr;
inline BitArrayCL* g_UnitsUnlocked = nullptr;

//================================================================================================================================
//=> - C interface -
//================================================================================================================================

extern "C" {

void InitUnitData (const char* filename) {
    if (g_UnitVectorInstance == nullptr) {
        UnitIO io(filename);
        io.parse_and_allocate();
        int count = io.validate_and_count();
        if (count > 0) {
            g_UnitsUnlocked = new BitArrayCL(static_cast<uint32_t>(count));
            g_UnitVectorInstance = new UnitVector(static_cast<uint32_t>(count), g_UnitsUnlocked);
        }
    }
}

void DestroyUnitVector () {
    delete g_UnitVectorInstance;
    delete g_UnitsUnlocked;
    g_UnitVectorInstance = nullptr;
    g_UnitsUnlocked = nullptr;
}

int GetUnitCount () {
    if (g_UnitVectorInstance) {
        return g_UnitVectorInstance->get_count();
    }
    return 0;
}

void GetUnit (int index, UnitDataC* out_data) {
    if (g_UnitVectorInstance && out_data) {
        UnitData data = g_UnitVectorInstance->get_unit(static_cast<uint32_t>(index));
        strncpy(out_data->name, data.stats.name.c_str(), sizeof(out_data->name) - 1);
        out_data->name[sizeof(out_data->name) - 1] = '\0';
        out_data->cost = static_cast<int>(data.stats.cost);
        out_data->attack = static_cast<int>(data.stats.attack);
        out_data->defense = static_cast<int>(data.stats.defense);
        out_data->moves = static_cast<int>(data.stats.moves);
        out_data->strength = static_cast<int>(data.strength);
    } else {
        if (out_data) {
            out_data->name[0] = '\0';
            out_data->cost = 0;
            out_data->attack = 0;
            out_data->defense = 0;
            out_data->moves = 0;
            out_data->strength = 0;
        }
    }
}

void SetAvailable (int index) {
    if (g_UnitVectorInstance) {
        g_UnitVectorInstance->set_available(index);
    }
}

int ValidateAndCount (const char* filename) {
    UnitIO io(filename);
    return io.validate_and_count();
}

}

#endif // UNIT_VECTOR_EXTERN_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
