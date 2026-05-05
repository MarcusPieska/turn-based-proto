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
    static void set_map (StaticBitBank* bit_bank, u16 civ_trait_count, u16 building_count);
    static bool civ_trait_has_discount_for_bld (u16 civ_trait_idx, u16 building_idx);
    static u16 get_civ_trait_count ();
    static u16 get_building_count ();

private:
    CivBldDiscountMap () = delete;
    CivBldDiscountMap (const CivBldDiscountMap& other) = delete;
    CivBldDiscountMap (CivBldDiscountMap&& other) = delete;

    static StaticBitBank* m_bit_bank;
    static u16 m_civ_trait_count;
    static u16 m_building_count;
};

#endif // CIV_BLD_DISCOUNT_MAP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
