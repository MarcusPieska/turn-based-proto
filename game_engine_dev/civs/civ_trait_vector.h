//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_TRAIT_VECTOR_H
#define CIV_TRAIT_VECTOR_H

#include <string>
#include <iosfwd>
#include <cstdint>

#include "game_primitives.h"
#include "bit_array.h"
#include "civ_trait_data.h"

//================================================================================================================================
//=> - Forward declarations -
//================================================================================================================================

class BitArrayCL;

//================================================================================================================================
//=> - CivTraitVector class -
//================================================================================================================================

class CivTraitVector {
public:
    friend class CivStarter;

    CivTraitVector();
    ~CivTraitVector();

    bool has_trait(u16 trait_idx) const;
    const CivTraitStats& get_trait_data(u16 trait_idx) const;

protected:
    void set_trait(u16 trait_idx);

private:
    CivTraitVector(const CivTraitVector& other) = delete;
    CivTraitVector(CivTraitVector&& other) = delete;

    BitArrayCL m_traits;
};

#endif // CIV_TRAIT_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
