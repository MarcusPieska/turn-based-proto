//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_adj_outline_fill.h"
#include "generator_constants.h"
#include "p1_step_log.h"

//================================================================================================================================
//=> - Private adjustment helpers -
//================================================================================================================================

static bool fill_outline_plains (u8* ter, u16 w, u16 h, const u8* ov) {
    if (ter == nullptr || ov == nullptr || w == 0 || h == 0) {
        P1_STEP_ABORT("P1_Adj_OutlineFill", "null terrain or overlay");
    }
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    const u8 ocean = TERR_OCEAN[0];
    const u8 plains = TERR_PLAINS[0];
    for (u32 i = 0; i < npx; ++i) {
        ter[i] = ocean;
        if (ov[i] == WL_OVERLAY_LAND_GRAY) {
            ter[i] = plains;
        }
    }
    return true;
}

//================================================================================================================================
//=> - P1_Adj_OutlineFill -
//================================================================================================================================

P1_Adj_OutlineFill::P1_Adj_OutlineFill (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_adjust(false) {
    (void)m_prm;
}

bool P1_Adj_OutlineFill::adjust (u8* terrain, u16 w, u16 h, const u8* ov) {
    m_valid_adjust = false;
    if (!fill_outline_plains(terrain, w, h, ov)) {
        return false;
    }
    m_valid_adjust = true;
    return true;
}

bool P1_Adj_OutlineFill::is_valid () const {
    return m_valid_adjust;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
