//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "overlay_yields.h"

#include "game_map_defs.h"
#include "improvement_yield_static_key.h"
#include "item_effects.h"
#include "map_overlay_enum.h"
#include "map_ov_bridge.h"
#include "runtime_statics.h"
#include "tile_attr_tables.h"
#include "tile_attribute_static_key.h"
#include "tile_yield_type_enum.h"
#include "worker_job_imp_index.h"
#include "worker_job_imp_static_key.h"
#include "worker_job_static_key.h"
#include "worker_job_target_enum.h"
#include "worker_job_type_enum.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

const RuntimeStatics* OverlayYields::m_st = nullptr;
OvYldTot* OverlayYields::m_tot = nullptr;
u16* OverlayYields::m_food_rk = nullptr;
u16* OverlayYields::m_prod_rk = nullptr;
u8* OverlayYields::m_res = nullptr;
u16 OverlayYields::m_ov_n = 0;
u16 OverlayYields::m_food_n = 0;
u16 OverlayYields::m_prod_n = 0;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

void OverlayYields::clear () {
    delete[] m_tot;
    delete[] m_food_rk;
    delete[] m_prod_rk;
    delete[] m_res;
    m_tot = nullptr;
    m_food_rk = nullptr;
    m_prod_rk = nullptr;
    m_res = nullptr;
    m_st = nullptr;
    m_ov_n = 0;
    m_food_n = 0;
    m_prod_n = 0;
}

bool OverlayYields::base_ok (u8 kind, u8 id, u16 ov) {
    if (kind == TileAttrTables::k_kind_riv) {
        return false;
    }
    if (kind == TileAttrTables::k_kind_road) {
        return false;
    }
    if (kind == TileAttrTables::k_kind_terr) {
        return id == TERR_PLAINS[0];
    }
    if (kind == TileAttrTables::k_kind_clim) {
        return id == CLIMATE_PLAINS;
    }
    if (kind == TileAttrTables::k_kind_ov) {
        const u8 gen = catalog_ov_to_map_gen(ov);
        return gen == id;
    }
    return false;
}

static void add_booster (OvYldTot* t, const ItemEffectBooster& b) {
    if (t == nullptr || b.scope != ItemEffectsScope::LOCAL) {
        return;
    }
    if (b.amount_mode != ItemEffectAmountMode::COUNT) {
        return;
    }
    if (b.target_id == ItemEffectBoosterType::FOOD) {
        t->m_food = static_cast<i16>(t->m_food + b.amount);
        return;
    }
    if (b.target_id == ItemEffectBoosterType::PRODUCTION) {
        t->m_prod = static_cast<i16>(t->m_prod + b.amount);
        return;
    }
    if (b.target_id == ItemEffectBoosterType::COMMERCE) {
        t->m_comm = static_cast<i16>(t->m_comm + b.amount);
    }
}

static void add_imp_fx (OvYldTot* t, const ItemEffectsStruct& fx) {
    for (u32 j = 0; j < MAX_EFFECT_COUNT; ++j) {
        const ItemEffectStruct& slot = fx.items[j];
        if (slot.type == static_cast<u16>(ItemEffectType::NONE)) {
            break;
        }
        if (slot.type != static_cast<u16>(ItemEffectType::BOOSTER)) {
            continue;
        }
        add_booster(t, slot.effect.booster);
    }
}

bool OverlayYields::rank_cand (const RuntimeStatics& st, u16 ov) {
    if (ov >= m_ov_n || m_res[ov] != 0u) {
        return false;
    }
    const u16 jn = st.worker_job().get_item_count();
    for (u16 j = 0; j < jn; ++j) {
        const WorkerJobStaticDataStruct& row = st.worker_job().get_item(WorkerJobStaticDataKey::from_raw(j));
        if (row.target_kind != static_cast<u16>(WorkerJobTarget::Overlay) || row.target_idx != ov) {
            continue;
        }
        const WorkerJobType typ = static_cast<WorkerJobType>(row.type);
        if (typ == WorkerJobType::Farm || typ == WorkerJobType::Forest) {
            return true;
        }
    }
    return false;
}

int OverlayYields::cmp_food (u16 a, u16 b) {
    const OvYldTot& ta = m_tot[a];
    const OvYldTot& tb = m_tot[b];
    if (ta.m_food != tb.m_food) {
        return (ta.m_food > tb.m_food) ? -1 : 1;
    }
    if (ta.m_prod != tb.m_prod) {
        return (ta.m_prod > tb.m_prod) ? -1 : 1;
    }
    if (ta.m_comm != tb.m_comm) {
        return (ta.m_comm > tb.m_comm) ? -1 : 1;
    }
    if (a != b) {
        return (a < b) ? -1 : 1;
    }
    return 0;
}

int OverlayYields::cmp_prod (u16 a, u16 b) {
    const OvYldTot& ta = m_tot[a];
    const OvYldTot& tb = m_tot[b];
    if (ta.m_prod != tb.m_prod) {
        return (ta.m_prod > tb.m_prod) ? -1 : 1;
    }
    if (ta.m_food != tb.m_food) {
        return (ta.m_food > tb.m_food) ? -1 : 1;
    }
    if (ta.m_comm != tb.m_comm) {
        return (ta.m_comm > tb.m_comm) ? -1 : 1;
    }
    if (a != b) {
        return (a < b) ? -1 : 1;
    }
    return 0;
}

void OverlayYields::sort_rank (u16* ids, u16 n, int (*cmp)(u16, u16)) {
    for (u16 i = 1; i < n; ++i) {
        const u16 v = ids[i];
        u16 j = i;
        while (j > 0u && cmp(ids[j - 1u], v) > 0) {
            ids[j] = ids[j - 1u];
            j = static_cast<u16>(j - 1u);
        }
        ids[j] = v;
    }
}

void OverlayYields::fill_tots (const RuntimeStatics& st) {
    const TileAttributeStaticDataStruct& terr = TileAttrTables::terr(TERR_PLAINS[0]);
    const TileAttributeStaticDataStruct& clim = TileAttrTables::clim(CLIMATE_PLAINS);
    for (u16 ov = 0; ov < m_ov_n; ++ov) {
        OvYldTot& t = m_tot[ov];
        t.m_food = static_cast<i16>(terr.food + clim.food);
        t.m_prod = static_cast<i16>(terr.production + clim.production);
        t.m_comm = static_cast<i16>(terr.commerce + clim.commerce);
        if (ov == static_cast<u16>(MapOverlay::Forest) || ov == static_cast<u16>(MapOverlay::Swamp)
            || ov == static_cast<u16>(MapOverlay::Jungle) || ov == static_cast<u16>(MapOverlay::Glacier)) {
            const TileAttributeStaticDataStruct& ova = TileAttrTables::ov(catalog_ov_to_map_gen(ov));
            t.m_food = static_cast<i16>(t.m_food + ova.food);
            t.m_prod = static_cast<i16>(t.m_prod + ova.production);
            t.m_comm = static_cast<i16>(t.m_comm + ova.commerce);
        }
        t.m_imp_n = st.worker_job_imp_index().imp_n(ov);
        m_res[ov] = 0u;
    }
    const u16 jn = st.worker_job().get_item_count();
    for (u16 j = 0; j < jn; ++j) {
        const WorkerJobStaticDataStruct& row = st.worker_job().get_item(WorkerJobStaticDataKey::from_raw(j));
        if (row.target_kind != static_cast<u16>(WorkerJobTarget::Overlay) || row.target_idx >= m_ov_n) {
            continue;
        }
        if (static_cast<WorkerJobType>(row.type) == WorkerJobType::Resource) {
            m_res[row.target_idx] = 1u;
        }
    }
    const ImprovementYieldStaticData& iy = st.improvement_yield();
    const TileAttributeStaticData& attrs = st.tile_attribute();
    const u16 iy_n = iy.get_item_count();
    for (u16 i = 0; i < iy_n; ++i) {
        const ImprovementYieldStaticDataStruct& row = iy.get_item(ImprovementYieldStaticDataKey::from_raw(i));
        if (row.site_kind != static_cast<u16>(WorkerJobTarget::Overlay) || row.site_idx >= m_ov_n) {
            continue;
        }
        if (row.cond_attr >= attrs.get_item_count()) {
            continue;
        }
        u8 kind = 0u;
        u8 id = 0u;
        cstr aname = attrs.get_name(TileAttributeStaticDataKey::from_raw(row.cond_attr));
        if (!TileAttrTables::map_name(aname, &kind, &id)) {
            continue;
        }
        if (!base_ok(kind, id, row.site_idx)) {
            continue;
        }
        OvYldTot& t = m_tot[row.site_idx];
        if (row.yield_type == static_cast<u16>(TileYieldType::FOOD)) {
            t.m_food = static_cast<i16>(t.m_food + row.amount);
        } else if (row.yield_type == static_cast<u16>(TileYieldType::PRODUCTION)) {
            t.m_prod = static_cast<i16>(t.m_prod + row.amount);
        } else if (row.yield_type == static_cast<u16>(TileYieldType::COMMERCE)) {
            t.m_comm = static_cast<i16>(t.m_comm + row.amount);
        }
    }
    const WorkerJobImpIndex& ix = st.worker_job_imp_index();
    const WorkerJobImpStaticData& imps = st.worker_job_imp();
    for (u16 ov = 0; ov < m_ov_n; ++ov) {
        const u16 n = ix.imp_n(ov);
        const u16* ids = ix.imps(ov);
        if (ids == nullptr || n == 0u) {
            continue;
        }
        for (u16 i = 0; i < n; ++i) {
            if (ids[i] >= imps.get_item_count()) {
                continue;
            }
            add_imp_fx(&m_tot[ov], imps.get_item(WorkerJobImpStaticDataKey::from_raw(ids[i])).effects);
        }
    }
}

void OverlayYields::fill_ranks (const RuntimeStatics& st) {
    m_food_n = 0;
    m_prod_n = 0;
    for (u16 ov = 0; ov < m_ov_n; ++ov) {
        if (!rank_cand(st, ov)) {
            continue;
        }
        m_food_rk[m_food_n] = ov;
        m_prod_rk[m_prod_n] = ov;
        m_food_n = static_cast<u16>(m_food_n + 1u);
        m_prod_n = static_cast<u16>(m_prod_n + 1u);
    }
    sort_rank(m_food_rk, m_food_n, cmp_food);
    sort_rank(m_prod_rk, m_prod_n, cmp_prod);
}

//================================================================================================================================
//=> - OverlayYields -
//================================================================================================================================

bool OverlayYields::setup (const RuntimeStatics& st) {
    clear();
    if (!TileAttrTables::ready()) {
        return false;
    }
    m_ov_n = st.map_overlay().get_item_count();
    if (m_ov_n == 0u) {
        return false;
    }
    m_tot = new OvYldTot[m_ov_n]();
    m_food_rk = new u16[m_ov_n]();
    m_prod_rk = new u16[m_ov_n]();
    m_res = new u8[m_ov_n]();
    if (m_tot == nullptr || m_food_rk == nullptr || m_prod_rk == nullptr || m_res == nullptr) {
        clear();
        return false;
    }
    m_st = &st;
    fill_tots(st);
    fill_ranks(st);
    return true;
}

u16 OverlayYields::ov_n () {
    return m_ov_n;
}

OvYldTot OverlayYields::tot (u16 ov) {
    OvYldTot z = {};
    if (m_tot == nullptr || ov >= m_ov_n) {
        return z;
    }
    return m_tot[ov];
}

bool OverlayYields::is_res (u16 ov) {
    if (m_res == nullptr || ov >= m_ov_n) {
        return false;
    }
    return m_res[ov] != 0u;
}

const u16* OverlayYields::rank (TileAssignIntent intent, u16* out_n) {
    if (out_n == nullptr) {
        return nullptr;
    }
    if (intent == TILE_ASSIGN_FOOD) {
        *out_n = m_food_n;
        return m_food_rk;
    }
    if (intent == TILE_ASSIGN_PROD) {
        *out_n = m_prod_n;
        return m_prod_rk;
    }
    *out_n = 0;
    return nullptr;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
