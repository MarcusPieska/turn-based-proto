//================================================================================================================================
//= - Includes -
//================================================================================================================================

#include <cstring>

#include "tile_attr_tables.h"
#include "map_attribute_static_data.h"
#include "map_attribute_static_key.h"
#include "map_climate_static_data.h"
#include "map_climate_static_key.h"
#include "map_overlay_static_data.h"
#include "map_overlay_static_key.h"
#include "map_terrain_static_data.h"
#include "map_terrain_static_key.h"
#include "runtime_statics.h"

//================================================================================================================================
//= - Statics -
//================================================================================================================================

bool TileAttrTables::m_ready = false;
TileAttrRow TileAttrTables::m_terr[TileAttrTables::k_terr_n] = {};
TileAttrRow TileAttrTables::m_clim[TileAttrTables::k_clim_n] = {};
TileAttrRow TileAttrTables::m_ov[TileAttrTables::k_ov_n] = {};
TileAttrRow TileAttrTables::m_road[TileAttrTables::k_road_n] = {};
TileAttrRow TileAttrTables::m_riv = {};
TileAttrRow TileAttrTables::m_zero = {};
u16 TileAttrTables::m_terr_n = 0u;
u16 TileAttrTables::m_clim_n = 0u;
u16 TileAttrTables::m_ov_n = 0u;
u16 TileAttrTables::m_road_n = 0u;

//================================================================================================================================
//= - Helpers -
//================================================================================================================================

static void s_copy_row (TileAttrRow* dst, u16 mvt, i16 food, u16 prod, u16 comm, u16 cult, u16 sci, u16 rel, u16 atk, u16 def) {
    dst->mvt_cost = mvt;
    dst->food = food;
    dst->production = prod;
    dst->commerce = comm;
    dst->culture = cult;
    dst->science = sci;
    dst->religion = rel;
    dst->attack_mod = atk;
    dst->defense_mod = def;
}

static void s_from_terr (TileAttrRow* dst, const MapTerrainStaticDataStruct& src) {
    s_copy_row(dst, src.mvt_cost, src.food, src.production, src.commerce, src.culture, src.science, src.religion,
        src.attack_mod, src.defense_mod);
}

static void s_from_clim (TileAttrRow* dst, const MapClimateStaticDataStruct& src) {
    s_copy_row(dst, src.mvt_cost, src.food, src.production, src.commerce, src.culture, src.science, src.religion,
        src.attack_mod, src.defense_mod);
}

static void s_from_ov (TileAttrRow* dst, const MapOverlayStaticDataStruct& src) {
    s_copy_row(dst, src.mvt_cost, src.food, src.production, src.commerce, src.culture, src.science, src.religion,
        src.attack_mod, src.defense_mod);
}

static void s_from_attr (TileAttrRow* dst, const MapAttributeStaticDataStruct& src) {
    s_copy_row(dst, src.mvt_cost, src.food, src.production, src.commerce, src.culture, src.science, src.religion,
        src.attack_mod, src.defense_mod);
}

//================================================================================================================================
//= - TileAttrTables -
//================================================================================================================================

void TileAttrTables::clear () {
    m_ready = false;
    m_terr_n = 0u;
    m_clim_n = 0u;
    m_ov_n = 0u;
    m_road_n = 0u;
    for (u16 i = 0; i < k_terr_n; ++i) {
        m_terr[i] = m_zero;
    }
    for (u16 i = 0; i < k_clim_n; ++i) {
        m_clim[i] = m_zero;
    }
    for (u16 i = 0; i < k_ov_n; ++i) {
        m_ov[i] = m_zero;
    }
    for (u16 i = 0; i < k_road_n; ++i) {
        m_road[i] = m_zero;
    }
    m_riv = m_zero;
}

bool TileAttrTables::setup (const RuntimeStatics& st) {
    clear ();
    const MapTerrainStaticData& terr = st.map_terrain();
    const MapClimateStaticData& clim = st.map_climate();
    const MapOverlayStaticData& ov = st.map_overlay();
    const MapAttributeStaticData& attr = st.map_attribute();
    const u16 tn = terr.get_item_count();
    const u16 cn = clim.get_item_count();
    const u16 on = ov.get_item_count();
    const u16 an = attr.get_item_count();
    if (tn == 0u || cn == 0u || on == 0u || an == 0u) {
        clear ();
        return false;
    }
    if (tn > k_terr_n || cn > k_clim_n || on > k_ov_n) {
        clear ();
        return false;
    }
    for (u16 i = 0; i < tn; ++i) {
        s_from_terr(&m_terr[i], terr.get_item(MapTerrainStaticDataKey::from_raw(i)));
    }
    for (u16 i = 0; i < cn; ++i) {
        s_from_clim(&m_clim[i], clim.get_item(MapClimateStaticDataKey::from_raw(i)));
    }
    for (u16 i = 0; i < on; ++i) {
        s_from_ov(&m_ov[i], ov.get_item(MapOverlayStaticDataKey::from_raw(i)));
    }
    u16 road_i = 0u;
    bool got_riv = false;
    for (u16 i = 0; i < an; ++i) {
        const MapAttributeStaticDataStruct& row = attr.get_item(MapAttributeStaticDataKey::from_raw(i));
        cstr nm = attr.get_name(MapAttributeStaticDataKey::from_raw(i));
        if (row.kind == 0u) {
            if (nm != nullptr && std::strcmp(nm, "River") == 0) {
                s_from_attr(&m_riv, row);
                got_riv = true;
            }
            continue;
        }
        if (row.kind != 1u) {
            continue;
        }
        if (road_i >= k_road_n) {
            clear ();
            return false;
        }
        s_from_attr(&m_road[road_i], row);
        road_i = static_cast<u16>(road_i + 1u);
    }
    if (!got_riv || road_i == 0u) {
        clear ();
        return false;
    }
    m_terr_n = tn;
    m_clim_n = cn;
    m_ov_n = on;
    m_road_n = road_i;
    m_ready = true;
    return true;
}

bool TileAttrTables::ready () {
    return m_ready;
}

u16 TileAttrTables::pack_cond (u8 kind, u16 id) {
    return static_cast<u16>((static_cast<u16>(kind) << 8) | (id & 0x00FFu));
}

bool TileAttrTables::unpack_cond (u16 packed, u8* out_kind, u8* out_id) {
    if (out_kind == nullptr || out_id == nullptr) {
        return false;
    }
    *out_kind = static_cast<u8>((packed >> 8) & 0x00FFu);
    *out_id = static_cast<u8>(packed & 0x00FFu);
    return true;
}

u16 TileAttrTables::terr_n () {
    return m_terr_n;
}

u16 TileAttrTables::clim_n () {
    return m_clim_n;
}

u16 TileAttrTables::ov_n () {
    return m_ov_n;
}

u16 TileAttrTables::road_n () {
    return m_road_n;
}

const TileAttrRow& TileAttrTables::terr (u8 id) {
    if (id >= m_terr_n) {
        return m_zero;
    }
    return m_terr[id];
}

const TileAttrRow& TileAttrTables::clim (u8 id) {
    if (id >= m_clim_n) {
        return m_zero;
    }
    return m_clim[id];
}

const TileAttrRow& TileAttrTables::ov (u8 id) {
    if (id >= m_ov_n) {
        return m_zero;
    }
    return m_ov[id];
}

const TileAttrRow& TileAttrTables::road (u8 id) {
    if (id >= m_road_n) {
        return m_zero;
    }
    return m_road[id];
}

const TileAttrRow& TileAttrTables::riv () {
    return m_riv;
}

//================================================================================================================================
//= - End of file -
//================================================================================================================================
