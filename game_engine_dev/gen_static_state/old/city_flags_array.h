//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_FLAGS_ARRAY_H
#define CITY_FLAGS_ARRAY_H

#include "city_flags_array_key.h"
#include "bit_array.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - CityFlagsArray class -
//================================================================================================================================

class CityFlagsArray {
public:
    CityFlagsArray (u16 entry_count);
    ~CityFlagsArray ();

    bool is_flagged (CityFlagsKey key) const;
    void set_flag (CityFlagsKey key);
    void clear_flag (CityFlagsKey key);

private:
    CityFlagsArray (const CityFlagsArray& other) = delete;
    CityFlagsArray (CityFlagsArray&& other) = delete;

    BitArrayCL m_flags;
};

#endif // CITY_FLAGS_ARRAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
