//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDINGS_BUILT_ARRAY_H
#define BUILDINGS_BUILT_ARRAY_H

#include "buildings_built_array_key.h"
#include "bit_array.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - BuildingsBuiltArray class -
//================================================================================================================================

class BuildingsBuiltArray {
public:
    BuildingsBuiltArray (u16 entry_count);
    ~BuildingsBuiltArray ();

    bool is_flagged (BuildingsBuiltKey key) const;
    void set_flag (BuildingsBuiltKey key);
    void clear_flag (BuildingsBuiltKey key);

private:
    BuildingsBuiltArray (const BuildingsBuiltArray& other) = delete;
    BuildingsBuiltArray (BuildingsBuiltArray&& other) = delete;

    BitArrayCL m_flags;
};

#endif // BUILDINGS_BUILT_ARRAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
