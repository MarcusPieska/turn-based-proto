//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_VECTOR_H
#define UNIT_VECTOR_H

#include <string>
#include <iosfwd>
#include <cstdint>

#include "game_primitives.h"
#include "bit_array.h"
#include "unit_data.h"

//================================================================================================================================
//=> - Forward declarations -
//================================================================================================================================

class BitArrayCL;

//================================================================================================================================
//=> - UnitInstance wrapper -
//================================================================================================================================
//
struct UnitInstance {
    const UnitTypeStats& stats;
    u16 strength;
};

//================================================================================================================================
//=> - BestUnitTypeInstance struct -
//================================================================================================================================

typedef struct BestUnitTypeInstance {
    u16 active_type;
    u16 unit_idx;
} BestUnitTypeInstance;

//================================================================================================================================
//=> - BestBuildableUnits class -
//================================================================================================================================

class BestBuildableUnits {
public:
    friend class UnitAssessor;

    BestBuildableUnits();
    ~BestBuildableUnits();

    u16 get_unit_type_count() const;
    u16 is_type_active(u16 unit_type_idx) const;
    u16 get_best_unit_idx_of_type(u16 unit_type_idx) const;

protected:
    void set_best_unit_idx_of_type(u16 unit_type_idx, u16 unit_idx);

private:
    BestBuildableUnits(const BestBuildableUnits& other) = delete;
    BestBuildableUnits(BestBuildableUnits&& other) = delete;

    BestUnitTypeInstance* m_buildable;
};

#endif // UNIT_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
