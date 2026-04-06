//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIVS_LOADED_ARRAY_H
#define CIVS_LOADED_ARRAY_H

#include "civs_loaded_array_key.h"
#include "bit_array.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - CivsLoadedArray class -
//================================================================================================================================

class CivsLoadedArray {
public:
    CivsLoadedArray (u16 entry_count);
    ~CivsLoadedArray ();

    bool is_flagged (CivsLoadedKey key) const;
    void set_flag (CivsLoadedKey key);
    void clear_flag (CivsLoadedKey key);

private:
    CivsLoadedArray (const CivsLoadedArray& other) = delete;
    CivsLoadedArray (CivsLoadedArray&& other) = delete;

    BitArrayCL m_flags;
};

#endif // CIVS_LOADED_ARRAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
