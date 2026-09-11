//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "target_sector_integral.h"

#include "sector_support.h"

//================================================================================================================================
//=> - TargetSector_Integral -
//================================================================================================================================

bool TargetSector_Integral::pick (
    u16 player,
    u16* tgts,
    u16 cap,
    u16* out_n,
    u16* out_sector,
    u16* out_enemy)
{
    if (tgts == nullptr || out_n == nullptr || out_sector == nullptr || out_enemy == nullptr || cap == 0u) {
        return false;
    }
    *out_n = 0u;
    *out_sector = U16_KEY_NULL;
    *out_enemy = U16_KEY_NULL;
    u16 sector = SectorSupport::get_shared_sector(player);
    if (sector == U16_KEY_NULL) {
        sector = SectorSupport::best_integral_sector(player);
    }
    if (sector == U16_KEY_NULL) {
        return false;
    }
    const SectorPresence pr = SectorSupport::sector_presence(player, sector);
    if (pr.m_own + pr.m_other == 0u) {
        return false;
    }
    // Prefer the foreign seat with the most cities in the sector. TODO: for robust targeting, pick
    // the enemy with the largest shared border length against our fully owned neighbors instead.
    if (pr.m_rival == U16_KEY_NULL) {
        return false;
    }
    const u16 n = SectorSupport::enemy_cities(sector, pr.m_rival, tgts, cap);
    if (n == 0u) {
        return false;
    }
    *out_n = n;
    *out_sector = sector;
    *out_enemy = pr.m_rival;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
