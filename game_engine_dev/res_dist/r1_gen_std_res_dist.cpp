//================================================================================================================================
//= - Includes -
//================================================================================================================================

#include "r1_gen_std_res_dist.h"

#include "r1_gen_empty_resource_overlay.h"
#include "r1_gen_res_sectors.h"
#include "r1_adj_gemstone_placements.h"
#include "r1_adj_livestock_placements.h"
#include "r1_adj_food_crop_placements.h"
#include "r1_adj_metal_placements.h"
#include "r1_adj_spice_placements.h"
#include "r1_adj_cash_crop_placements.h"
#include "r1_adj_produce_placements.h"
#include "r1_adj_general_placement.h"
#include "res_dist_state.h"
#include "land_mass_index.h"

//================================================================================================================================
//= - R1_Gen_StdResDist -
//================================================================================================================================

R1_Gen_StdResDist::R1_Gen_StdResDist ()
    : m_ok(false)
    , m_w(0)
    , m_h(0)
    , m_ov(nullptr)
{
}

R1_Gen_StdResDist::~R1_Gen_StdResDist () {
    clr();
}

void R1_Gen_StdResDist::clr () {
    delete[] m_ov;
    m_ov = nullptr;
    m_ok = false;
    m_w = 0;
    m_h = 0;
}

bool R1_Gen_StdResDist::generate (const ResPlcMapCtx& ctx, const RuntimeStatics& s, u32 base_n, u32 seed) {
    clr();
    if (ctx.m_terrain == nullptr || ctx.m_w == 0 || ctx.m_h == 0 || base_n == 0) {
        return false;
    }
    R1_Gen_EmptyResourceOverlay empty;
    if (!empty.generate(ctx.m_w, ctx.m_h) || !empty.is_valid()) {
        return false;
    }
    LandMassIndex mass;
    if (!mass.generate(ctx.m_terrain, ctx.m_w, ctx.m_h) || !mass.is_valid()) {
        return false;
    }
    ResDistState st;
    if (!st.reset(s.resource().get_item_count())) {
        return false;
    }
    R1_Gen_ResSectors def_sec;
    if (!def_sec.generate(ctx.m_terrain, ctx.m_w, ctx.m_h, mass.result(), seed,
            static_cast<u16>(R1_RES_SECTOR_PCT_DEF)) || !def_sec.is_valid()) {
        return false;
    }
    const R1_Gen_ResSectorsRslt& dsr = def_sec.result();
    R1_Adj_GemstonePlacements gems;
    if (!gems.adjust(empty.overlay_mut(), s, st, ctx, dsr.m_ov, dsr.m_sec_n, base_n, seed) || !gems.is_valid()) {
        return false;
    }
    R1_Adj_LivestockPlacements livs;
    if (!livs.adjust(empty.overlay_mut(), s, st, ctx, dsr.m_ov, dsr.m_sec_n, base_n, seed) || !livs.is_valid()) {
        return false;
    }
    R1_Adj_FoodCropPlacements crops;
    if (!crops.adjust(empty.overlay_mut(), s, st, ctx, dsr.m_ov, dsr.m_sec_n, base_n, seed) || !crops.is_valid()) {
        return false;
    }
    R1_Gen_ResSectors met_sec;
    if (!met_sec.generate(ctx.m_terrain, ctx.m_w, ctx.m_h, mass.result(), seed,
            static_cast<u16>(R1_RES_SECTOR_PCT_MET)) || !met_sec.is_valid()) {
        return false;
    }
    const R1_Gen_ResSectorsRslt& msr = met_sec.result();
    R1_Adj_MetalPlacements mets;
    if (!mets.adjust(empty.overlay_mut(), s, st, ctx, msr.m_ov, msr.m_sec_n, base_n, seed) || !mets.is_valid()) {
        return false;
    }
    R1_Adj_SpicePlacements spices;
    if (!spices.adjust(empty.overlay_mut(), s, st, ctx, msr.m_ov, msr.m_sec_n, base_n, seed) || !spices.is_valid()) {
        return false;
    }
    R1_Adj_CashCropPlacements cash;
    if (!cash.adjust(empty.overlay_mut(), s, st, ctx, msr.m_ov, msr.m_sec_n, base_n, seed) || !cash.is_valid()) {
        return false;
    }
    R1_Adj_ProducePlacements prod;
    if (!prod.adjust(empty.overlay_mut(), s, st, ctx, msr.m_ov, msr.m_sec_n, base_n, seed) || !prod.is_valid()) {
        return false;
    }
    R1_Adj_GeneralPlacement gen;
    if (!gen.adjust(empty.overlay_mut(), ctx.m_w, ctx.m_h, ctx, s, st, base_n, seed) || !gen.is_valid()) {
        return false;
    }
    m_ov = empty.take_overlay();
    if (m_ov == nullptr) {
        return false;
    }
    m_w = ctx.m_w;
    m_h = ctx.m_h;
    m_ok = true;
    return true;
}

bool R1_Gen_StdResDist::is_valid () const {
    return m_ok;
}

u16 R1_Gen_StdResDist::width () const {
    return m_w;
}

u16 R1_Gen_StdResDist::height () const {
    return m_h;
}

const u16* R1_Gen_StdResDist::overlay () const {
    return m_ov;
}

u16* R1_Gen_StdResDist::take_overlay () {
    u16* p = m_ov;
    m_ov = nullptr;
    m_ok = false;
    m_w = 0;
    m_h = 0;
    return p;
}

//================================================================================================================================
//= - End of file -
//================================================================================================================================
