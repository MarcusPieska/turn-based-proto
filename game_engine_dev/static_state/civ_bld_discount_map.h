//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_BLD_DISCOUNT_MAP_H
#define CIV_BLD_DISCOUNT_MAP_H

#include "static_bit_bank.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - CivBldDiscountMap class -
//================================================================================================================================

class CivBldDiscountMap {
public:
    CivBldDiscountMap () = default;
    void set_map (StaticBitBank* bit_bank, u16 civ_trait_count, u16 building_count);
    void take_ownership ();
    void release_map ();
    bool civ_trait_has_discount_for_bld (u16 civ_trait_idx, u16 building_idx) const;
    u16 get_civ_trait_count () const;
    u16 get_building_count () const;

private:
    CivBldDiscountMap (const CivBldDiscountMap& other) = delete;
    CivBldDiscountMap (CivBldDiscountMap&& other) = delete;
    CivBldDiscountMap& operator= (const CivBldDiscountMap& other) = delete;
    CivBldDiscountMap& operator= (CivBldDiscountMap&& other) = delete;

    StaticBitBank* m_bit_bank = nullptr;
    u16 m_civ_trait_count = 0;
    u16 m_building_count = 0;
};

#endif // CIV_BLD_DISCOUNT_MAP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
