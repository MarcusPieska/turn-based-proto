//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "map_road_save_timeline_mng.h"

#include <cstdio>

#include "eval_paths.h"
#include "game_array_simple.h"
#include "game_io.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Labels -
//================================================================================================================================

static const char* const G_LABELS[MapRoadSaveTimelineMng::SERIES_N] = {
    "intent_mtn",
    "intent_fort",
    "road_path",
    "road_cobble",
    "road_asphalt",
    "road_rail",
    "road_virtual"
};

//================================================================================================================================
//=> - MapRoadSaveTimelineMng -
//================================================================================================================================

MapRoadSaveTimelineMng::MapRoadSaveTimelineMng ()
    : m_tl(nullptr),
      m_save_n(0) {
}

MapRoadSaveTimelineMng::~MapRoadSaveTimelineMng () {
    clr();
}

void MapRoadSaveTimelineMng::clr () {
    delete[] m_tl;
    m_tl = nullptr;
    m_save_n = 0;
}

bool MapRoadSaveTimelineMng::setup (u16 save_n) {
    clr();
    if (save_n == 0) {
        return false;
    }
    m_tl = new MapRoadSaveTimeline[SERIES_N];
    m_save_n = save_n;
    for (u16 i = 0; i < SERIES_N; ++i) {
        if (!m_tl[i].setup(save_n)) {
            clr();
            return false;
        }
    }
    return true;
}

bool MapRoadSaveTimelineMng::fill (const EvalPaths& paths) {
    if (m_tl == nullptr || !paths.ok() || paths.save_turn_n() != m_save_n) {
        return false;
    }
    u32 tallies[SERIES_N];
    for (u16 si = 0; si < m_save_n; ++si) {
        const u32 turn = paths.save_turn_at(si);
        char map_p[512];
        if (!paths.map_path(turn, map_p, sizeof(map_p))) {
            return false;
        }
        {
            GameArraySimple map;
            if (!GameIo::load_map_tiles(map_p, map)) {
                return false;
            }
            for (u16 k = 0; k < SERIES_N; ++k) {
                tallies[k] = 0;
            }
            const u16 w = map.width();
            const u16 h = map.height();
            for (u16 y = 0; y < h; ++y) {
                for (u16 x = 0; x < w; ++x) {
                    const u8 iv = map.get_ai_ov_intent(x, y);
                    if (iv == AI_TILE_OV_INTENT_MTN_PASS) {
                        tallies[SERIES_INTENT_MTN] = tallies[SERIES_INTENT_MTN] + 1u;
                    } else if (iv == AI_TILE_OV_INTENT_FORT) {
                        tallies[SERIES_INTENT_FORT] = tallies[SERIES_INTENT_FORT] + 1u;
                    }
                    const u8 rd = map.get_road_typ(x, y);
                    if (rd == ROAD_PATH) {
                        tallies[SERIES_PATH] = tallies[SERIES_PATH] + 1u;
                    } else if (rd == ROAD_COBBLE) {
                        tallies[SERIES_COBBLE] = tallies[SERIES_COBBLE] + 1u;
                    } else if (rd == ROAD_ASPHALT) {
                        tallies[SERIES_ASPHALT] = tallies[SERIES_ASPHALT] + 1u;
                    } else if (rd == ROAD_RAIL) {
                        tallies[SERIES_RAIL] = tallies[SERIES_RAIL] + 1u;
                    } else if (rd == ROAD_VIRTUAL) {
                        tallies[SERIES_VIRTUAL] = tallies[SERIES_VIRTUAL] + 1u;
                    }
                }
            }
        }
        for (u16 k = 0; k < SERIES_N; ++k) {
            if (!m_tl[k].set(si, tallies[k])) {
                return false;
            }
        }
    }
    for (u16 k = 0; k < SERIES_N; ++k) {
        m_tl[k].sync_count();
    }
    return true;
}

u16 MapRoadSaveTimelineMng::series_n () const {
    return SERIES_N;
}

u16 MapRoadSaveTimelineMng::save_n () const {
    return m_save_n;
}

cstr MapRoadSaveTimelineMng::label (u16 i) const {
    if (i >= SERIES_N) {
        return "";
    }
    return G_LABELS[i];
}

MapRoadSaveTimeline& MapRoadSaveTimelineMng::at (u16 i) {
    return m_tl[i];
}

const MapRoadSaveTimeline& MapRoadSaveTimelineMng::at (u16 i) const {
    return m_tl[i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
