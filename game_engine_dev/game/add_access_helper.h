//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef ADD_ACCESS_HELPER_H
#define ADD_ACCESS_HELPER_H

#include "game_primitives.h"

struct GameTileSimple;
class GeneralBitBank;
class RuntimeStatics;

//================================================================================================================================
//=> - AddAccessHelper -
//================================================================================================================================
//
//  Overlay payload access: City is an external key into CityArray; other overlays store imp bits in m_add_idx while
//  imp_n <= 16, else a GeneralBitBank per overlay (m_add_idx = batch). City overlay index resolved at setup by name.
//
//================================================================================================================================

class AddAccessHelper {
public:
    static bool setup (const RuntimeStatics& st);
    static void clear ();

    static u16 city_ov (); // Catalog index for "City"; U16_KEY_NULL until setup
    static bool is_city (u16 ov); // True when ov is the setup-time City slot
    static bool uses_bank (u16 ov); // True when this overlay has a GeneralBitBank
    static GeneralBitBank* bank (u16 ov); // Bank for ov, or null
    static u16 empty_add (u16 ov); // Cleared payload: 0 inline/city, U16_KEY_NULL for bank
    static u16 payload_mask (u16 ov); // Inline bit mask; 0xFFFF for city/bank
    static bool add_ok (u16 ov, u16 add_idx); // Validates m_add_idx for ov
    static bool has_imp (const GameTileSimple* t, u16 imp_idx); // Imp bit/flag on tile
    static bool set_imp (GameTileSimple* t, u16 imp_idx); // Sets imp bit/flag on tile

private:
    static const RuntimeStatics* m_st; // Bound catalogs; null until setup
    static GeneralBitBank** m_banks; // One slot per map_overlay; null = inline or city
    static u16 m_ov_n; // map_overlay catalog size
    static u16 m_city_ov; // Resolved "City" index

    AddAccessHelper () = delete;
};

#endif // ADD_ACCESS_HELPER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
