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
//=> - WondersBuiltVector class (static) -
//================================================================================================================================

class WondersBuiltVector {
public:
    static void allocate_static_array ();
    static void set_owning_city (u16 idx, u16 city_id);
    static bool has_been_built (u16 idx);

private:
    WondersBuiltVector () = delete;
    WondersBuiltVector (const WondersBuiltVector& other) = delete;
    WondersBuiltVector (WondersBuiltVector&& other) = delete;
    ~WondersBuiltVector () = delete;
};

//================================================================================================================================
//=> - WonderBuildableVector class -
//================================================================================================================================

class WonderBuildableVector {
public:
    friend class WonderAssessor;

    WonderBuildableVector ();
    ~WonderBuildableVector ();

    bool can_build (u16 idx) const;

protected:
    void set_buildable (u16 idx);

private:
    WonderBuildableVector (const WonderBuildableVector& other) = delete;
    WonderBuildableVector (WonderBuildableVector&& other) = delete;

    BitArrayCL* m_buildable;
};

#endif // WONDER_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
