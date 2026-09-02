//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "add_access_helper.h"
#include "game_array_simple.h"
#include "tile_imp_helper.h" 

const RuntimeStatics* TileImpHelper::m_st = nullptr;

//================================================================================================================================
//=> - TileImpHelper -
//================================================================================================================================

void TileImpHelper::bind_statics (const RuntimeStatics*) {
}

bool TileImpHelper::add_idx_ok (u16, u16) {
    return true;
}

const RuntimeStatics* AddAccessHelper::m_st = nullptr;
GeneralBitBank** AddAccessHelper::m_banks = nullptr;
u16 AddAccessHelper::m_ov_n = 0;
u16 AddAccessHelper::m_city_ov = U16_KEY_NULL;

bool AddAccessHelper::setup (const RuntimeStatics&) {
    return true;
}

void AddAccessHelper::clear () {
}

u16 AddAccessHelper::empty_add (u16) {
    return 0;
}

u16 AddAccessHelper::payload_mask (u16) {
    return 0;
}

bool AddAccessHelper::add_ok (u16, u16) {
    return false;
}

bool AddAccessHelper::has_imp (const GameTileSimple*, u16) {
    return false;
}

bool AddAccessHelper::set_imp (GameTileSimple*, u16) {
    return false;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================