//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_VECTOR_H
#define UNIT_VECTOR_H

#include <string>
#include <iosfwd>
#include <cstdint>

#include "unit_data.h"  

//================================================================================================================================
//=> - Forward declarations -
//================================================================================================================================

class BitArrayCL;

typedef struct UnitInstance {
    const UnitTypeStats& stats;
    uint16_t strength;
} UnitInstance;

//================================================================================================================================
//=> - UnitVector class -
//================================================================================================================================

class UnitVector {
public:
    friend class TrainableAssessor;

    UnitVector (const BitArrayCL* researched_units);
    ~UnitVector ();

    UnitInstance get_unit (uint32_t index) const;
    bool is_trainable (uint32_t index) const;
    uint32_t get_count () const;

    static uint32_t get_unit_data_count ();
    static const UnitTypeStats* get_unit_data_array ();

protected:
    void set_trainable (uint32_t index);

private:
    UnitVector (const UnitVector& other) = delete;
    UnitVector (UnitVector&& other) = delete;
    UnitVector () = delete;

    const BitArrayCL* m_researched_units;
    BitArrayCL* m_units_unlocked;
};

#endif // UNIT_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
