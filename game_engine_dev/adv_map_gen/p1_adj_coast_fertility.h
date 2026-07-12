//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_ADJ_COAST_FERTILITY_H
#define P1_ADJ_COAST_FERTILITY_H

#include "game_primitives.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1 coast fertility adjust defaults -
//================================================================================================================================

#define P1_COAST_FERT_GRASS_PCT_DEF 4u
#define P1_COAST_FERT_PLAINS_PCT_DEF 8u

//================================================================================================================================
//=> - P1_CoastFertTile -
//================================================================================================================================

struct P1_CoastFertTile {
    u16 m_val;
    u16 m_x;
    u16 m_y;
};

//================================================================================================================================
//=> - P1_Adj_CoastFertilityPrm -
//================================================================================================================================

struct P1_Adj_CoastFertilityPrm {
    u16 m_grass_pct;
    u16 m_plains_pct;
};

static inline P1_Adj_CoastFertilityPrm p1_adj_coast_fertility_prm_def () {
    P1_Adj_CoastFertilityPrm p;
    p.m_grass_pct = static_cast<u16>(P1_COAST_FERT_GRASS_PCT_DEF);
    p.m_plains_pct = static_cast<u16>(P1_COAST_FERT_PLAINS_PCT_DEF);
    return p;
}

//================================================================================================================================
//=> - P1_Adj_CoastFertility -
//================================================================================================================================
//
//  Sort fertile land tiles by overlay value; top pct to grassland, next pct to plains.
//
//================================================================================================================================

class P1_Adj_CoastFertility {
public:
    explicit P1_Adj_CoastFertility (
        const P1_RunPrm& prm,
        const P1_Adj_CoastFertilityPrm& sp = p1_adj_coast_fertility_prm_def ()); 

    bool adjust (u8* climate, u16 w, u16 h, const u16* fert);
    bool is_valid () const;
    u32 nz_n () const;
    u32 grass_n () const;
    u32 plains_n () const;

private:
    P1_Adj_CoastFertility (const P1_Adj_CoastFertility& other) = delete;
    P1_Adj_CoastFertility (P1_Adj_CoastFertility&& other) = delete;

    P1_RunPrm m_prm;
    P1_Adj_CoastFertilityPrm m_sp;
    bool m_valid_adjust;
    u32 m_nz_n;
    u32 m_grass_n;
    u32 m_plains_n;
};

#endif // P1_ADJ_COAST_FERTILITY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
