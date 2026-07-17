//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>

#include "tile_attr_tables.h"
#include "game_map_defs.h"
#include "runtime_statics.h"
#include "tile_attribute_static_key.h"

//================================================================================================================================
//=> - Name map -
//================================================================================================================================

struct TileAttrNameRow {
    cstr m_name;
    u8 m_kind;
    u8 m_id;
};

static const TileAttrNameRow k_name_rows[] = {
    {"TERR_NONE", TileAttrTables::k_kind_terr, TERR_NONE[0]},
    {"TERR_OCEAN", TileAttrTables::k_kind_terr, TERR_OCEAN[0]},
    {"TERR_SEA", TileAttrTables::k_kind_terr, TERR_SEA[0]},
    {"TERR_COASTAL", TileAttrTables::k_kind_terr, TERR_COASTAL[0]},
    {"TERR_PLAINS", TileAttrTables::k_kind_terr, TERR_PLAINS[0]},
    {"TERR_HILLS", TileAttrTables::k_kind_terr, TERR_HILLS[0]},
    {"TERR_MOUNTAINS", TileAttrTables::k_kind_terr, TERR_MOUNTAINS[0]},
    {"TERR_VOLCANO", TileAttrTables::k_kind_terr, TERR_VOLCANO[0]},
    {"TERR_INLAND_SEA", TileAttrTables::k_kind_terr, TERR_INLAND_SEA[0]},
    {"TERR_INLAND_LAKE", TileAttrTables::k_kind_terr, TERR_INLAND_LAKE[0]},
    {"TERR_TILE_SENTINEL", TileAttrTables::k_kind_terr, TERR_TILE_SENTINEL[0]},
    {"CLIMATE_NONE", TileAttrTables::k_kind_clim, CLIMATE_NONE},
    {"CLIMATE_PLAINS", TileAttrTables::k_kind_clim, CLIMATE_PLAINS},
    {"CLIMATE_DESERT", TileAttrTables::k_kind_clim, CLIMATE_DESERT},
    {"CLIMATE_GRASSLAND", TileAttrTables::k_kind_clim, CLIMATE_GRASSLAND},
    {"CLIMATE_BLACK_SOIL", TileAttrTables::k_kind_clim, CLIMATE_BLACK_SOIL},
    {"OV_NONE", TileAttrTables::k_kind_ov, OV_NONE[0]},
    {"OV_FORESTS", TileAttrTables::k_kind_ov, OV_FOREST[0]},
    {"OV_SWAMPS", TileAttrTables::k_kind_ov, OV_SWAMP[0]},
    {"OV_JUNGLES", TileAttrTables::k_kind_ov, OV_JUNGLE[0]},
    {"OV_GLACIER", TileAttrTables::k_kind_ov, OV_GLACIER[0]},
    {"OV_RIVERS", TileAttrTables::k_kind_riv, 0u},
    {"ROAD_NONE", TileAttrTables::k_kind_road, ROAD_NONE},
    {"ROAD_PATH", TileAttrTables::k_kind_road, ROAD_PATH},
    {"ROAD_COBBLE", TileAttrTables::k_kind_road, ROAD_COBBLE},
    {"ROAD_ASPHALT", TileAttrTables::k_kind_road, ROAD_ASPHALT},
    {"ROAD_RAIL", TileAttrTables::k_kind_road, ROAD_RAIL},
};

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

bool TileAttrTables::m_ready = false;
TileAttributeStaticDataStruct TileAttrTables::m_terr[TileAttrTables::k_terr_n] = {};
TileAttributeStaticDataStruct TileAttrTables::m_clim[TileAttrTables::k_clim_n] = {};
TileAttributeStaticDataStruct TileAttrTables::m_ov[TileAttrTables::k_ov_n] = {};
TileAttributeStaticDataStruct TileAttrTables::m_road[TileAttrTables::k_road_n] = {};
TileAttributeStaticDataStruct TileAttrTables::m_riv = {};
TileAttributeStaticDataStruct TileAttrTables::m_zero = {};

//================================================================================================================================
//=> - TileAttrTables -
//================================================================================================================================

void TileAttrTables::clear () {
    m_ready = false;
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
    const TileAttributeStaticData& src = st.tile_attribute();
    const u16 n = src.get_item_count();
    u16 mapped = 0u;
    for (u16 i = 0; i < n; ++i) {
        const TileAttributeStaticDataKey key = TileAttributeStaticDataKey::from_raw(i);
        cstr nm = src.get_name(key);
        u8 kind = 0u;
        u8 id = 0u;
        if (!map_name(nm, &kind, &id)) {
            clear ();
            return false;
        }
        const TileAttributeStaticDataStruct& row = src.get_item(key);
        if (kind == k_kind_terr) {
            if (id >= k_terr_n) {
                clear ();
                return false;
            }
            m_terr[id] = row;
        } else if (kind == k_kind_clim) {
            if (id >= k_clim_n) {
                clear ();
                return false;
            }
            m_clim[id] = row;
        } else if (kind == k_kind_ov) {
            if (id >= k_ov_n) {
                clear ();
                return false;
            }
            m_ov[id] = row;
        } else if (kind == k_kind_road) {
            if (id >= k_road_n) {
                clear ();
                return false;
            }
            m_road[id] = row;
        } else if (kind == k_kind_riv) {
            m_riv = row;
        } else {
            clear ();
            return false;
        }
        mapped = static_cast<u16>(mapped + 1u);
    }
    if (mapped != n) {
        clear ();
        return false;
    }
    m_ready = true;
    return true;
}

bool TileAttrTables::ready () {
    return m_ready;
}

bool TileAttrTables::map_name (cstr name, u8* out_kind, u8* out_id) {
    if (name == nullptr || out_kind == nullptr || out_id == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(sizeof(k_name_rows) / sizeof(k_name_rows[0]));
    for (u32 i = 0; i < n; ++i) {
        if (std::strcmp(name, k_name_rows[i].m_name) == 0) {
            *out_kind = k_name_rows[i].m_kind;
            *out_id = k_name_rows[i].m_id;
            return true;
        }
    }
    return false;
}

u16 TileAttrTables::terr_n () {
    return k_terr_n;
}

u16 TileAttrTables::clim_n () {
    return k_clim_n;
}

u16 TileAttrTables::ov_n () {
    return k_ov_n;
}

u16 TileAttrTables::road_n () {
    return k_road_n;
}

const TileAttributeStaticDataStruct& TileAttrTables::terr (u8 id) {
    if (id >= k_terr_n) {
        return m_zero;
    }
    return m_terr[id];
}

const TileAttributeStaticDataStruct& TileAttrTables::clim (u8 id) {
    if (id >= k_clim_n) {
        return m_zero;
    }
    return m_clim[id];
}

const TileAttributeStaticDataStruct& TileAttrTables::ov (u8 id) {
    if (id >= k_ov_n) {
        return m_zero;
    }
    return m_ov[id];
}

const TileAttributeStaticDataStruct& TileAttrTables::road (u8 id) {
    if (id >= k_road_n) {
        return m_zero;
    }
    return m_road[id];
}

const TileAttributeStaticDataStruct& TileAttrTables::riv () {
    return m_riv;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
