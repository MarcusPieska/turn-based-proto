//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "adjust_river_to_inlets.h"

#include "generate_river_line_data.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Private adjustment helpers -
//================================================================================================================================

static u16 inlet_lim (u16 max_d, u8 perc, u8 min_sz) {
    if (max_d == 0) {
        return 0;
    }
    u32 lim = (static_cast<u32>(max_d) * static_cast<u32>(perc) + 99u) / 100u;
    if (lim < static_cast<u32>(min_sz)) {
        lim = min_sz;
    }
    if (lim > static_cast<u32>(max_d)) {
        lim = max_d;
    }
    return static_cast<u16>(lim);
}

//================================================================================================================================
//=> - RiverToInlets -
//================================================================================================================================

Adjust_RiverToInlets::Adjust_RiverToInlets (u32 seed) :
    m_seed(seed),
    m_valid_adjust(false) {
}

bool Adjust_RiverToInlets::adjust (u8* terrain, const u8* riv, const RiverLineDataResult* data, u8 perc, u8 min_sz) {
    m_valid_adjust = false;
    if (terrain == nullptr || riv == nullptr || data == nullptr || data->sys == nullptr
        || data->rdep == nullptr || data->rsys == nullptr
        || data->w == 0 || data->h == 0 || data->sys_n == 0 || perc == 0 || min_sz == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(data->w) * static_cast<u32>(data->h);
    for (u32 i = 0; i < n; ++i) {
        if (riv[i] == 0 || data->rdep[i] == 0) {
            continue;
        }
        const u16 si = data->rsys[i];
        if (si >= data->sys_n) {
            continue;
        }
        const u16 lim = inlet_lim(data->sys[si].max_d, perc, min_sz);
        if (data->rdep[i] <= lim) {
            terrain[i] = TERR_COASTAL[0];
        }
    }
    m_valid_adjust = true;
    (void)m_seed;
    return true;
}

bool Adjust_RiverToInlets::is_valid () const {
    return m_valid_adjust;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
