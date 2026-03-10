//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SMALL_WONDER_VECTOR_H
#define SMALL_WONDER_VECTOR_H

#include <string>
#include <iosfwd>
#include <cstdint>

#include "game_primitives.h"
#include "bit_array.h"
#include "small_wonder_data.h"

//================================================================================================================================
//=> - Forward declarations -
//================================================================================================================================

class BitArrayCL;

//================================================================================================================================
//=> - BuiltSmallWonders class -
//================================================================================================================================

class BuiltSmallWonders {
public:
    BuiltSmallWonders ();
    ~BuiltSmallWonders ();

    void set_owning_city (u16 idx, u16 city_id);
    bool has_been_built (u16 idx) const;
    u16 get_owning_city (u16 idx) const;

private:
    BuiltSmallWonders (const BuiltSmallWonders& other) = delete;
    BuiltSmallWonders (BuiltSmallWonders&& other) = delete;

    u16* m_built;
};

//================================================================================================================================
//=> - BuildableSmallWonders class -
//================================================================================================================================

class BuildableSmallWonders {
public:
    friend class SmallWonderAssessor;

    BuildableSmallWonders ();
    ~BuildableSmallWonders ();

    bool can_build (u16 idx) const;

protected:
    void set_buildable (u16 idx);

private:
    BuildableSmallWonders (const BuildableSmallWonders& other) = delete;
    BuildableSmallWonders (BuildableSmallWonders&& other) = delete;

    BitArrayCL m_buildable;
};

#endif // SMALL_WONDER_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
