//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WONDER_VECTOR_H
#define WONDER_VECTOR_H

#include <string>
#include <iosfwd>
#include <cstdint>

#include "game_primitives.h"
#include "bit_array.h"
#include "wonder_data.h"

//================================================================================================================================
//=> - Forward declarations -
//================================================================================================================================

class BitArrayCL;

//================================================================================================================================
//=> - BuiltWonders class (static) -
//================================================================================================================================

class BuiltWonders {
public:
    static void allocate_static_array ();
    static void set_owning_city (u16 idx, u16 city_id);
    static bool has_been_built (u16 idx);

private:
    BuiltWonders () = delete;
    BuiltWonders (const BuiltWonders& other) = delete;
    BuiltWonders (BuiltWonders&& other) = delete;
    ~BuiltWonders () = delete;
};

//================================================================================================================================
//=> - BuildableWonders class -
//================================================================================================================================

class BuildableWonders {
public:
    friend class WonderAssessor;

    BuildableWonders ();
    ~BuildableWonders ();

    bool can_build (u16 idx) const;

protected:
    void set_buildable (u16 idx);

private:
    BuildableWonders (const BuildableWonders& other) = delete;
    BuildableWonders (BuildableWonders&& other) = delete;

    BitArrayCL m_buildable;
};

#endif // WONDER_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
