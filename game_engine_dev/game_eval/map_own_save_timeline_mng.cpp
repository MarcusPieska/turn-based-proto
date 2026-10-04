//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "map_own_save_timeline_mng.h"

#include <cstdio>

#include "eval_paths.h"
#include "game_array_simple.h"
#include "game_io.h"
#include "map_own_save_snap.h"

//================================================================================================================================
//=> - MapOwnSaveTimelineMng -
//================================================================================================================================

MapOwnSaveTimelineMng::MapOwnSaveTimelineMng ()
    : m_save_n(0),
      m_wrote_n(0) {
}

MapOwnSaveTimelineMng::~MapOwnSaveTimelineMng () {
    clr();
}

void MapOwnSaveTimelineMng::clr () {
    m_save_n = 0;
    m_wrote_n = 0;
}

bool MapOwnSaveTimelineMng::setup (u16 save_n) {
    clr();
    if (save_n == 0) {
        return false;
    }
    m_save_n = save_n;
    return true;
}

bool MapOwnSaveTimelineMng::fill (const EvalPaths& paths, cstr out_dir, cstr share_dir) {
    if (out_dir == nullptr || out_dir[0] == 0 || !paths.ok() || paths.save_turn_n() != m_save_n) {
        return false;
    }
    m_wrote_n = 0;
    for (u16 si = 0; si < m_save_n; ++si) {
        const u32 turn = paths.save_turn_at(si);
        char map_p[512];
        if (!paths.map_path(turn, map_p, sizeof(map_p))) {
            return false;
        }
        char out_p[512];
        if (std::snprintf(out_p, sizeof(out_p), "%s/own_t%04u.ppm", out_dir, turn) <= 0) {
            return false;
        }
        {
            GameArraySimple map;
            if (!GameIo::load_map_tiles(map_p, map)) {
                return false;
            }
            if (!MapOwnSaveSnap::write(out_p, map)) {
                return false;
            }
            if (si + 1u == m_save_n && share_dir != nullptr && share_dir[0] != 0) {
                char share_p[512];
                if (std::snprintf(share_p, sizeof(share_p), "%s/tile_own_last.ppm", share_dir) <= 0) {
                    return false;
                }
                if (!MapOwnSaveSnap::write(share_p, map)) {
                    return false;
                }
                std::printf("wrote %s\n", share_p);
            }
        }
        m_wrote_n = static_cast<u16>(m_wrote_n + 1u);
        std::printf("wrote %s\n", out_p);
    }
    return true;
}

u16 MapOwnSaveTimelineMng::save_n () const {
    return m_save_n;
}

u16 MapOwnSaveTimelineMng::wrote_n () const {
    return m_wrote_n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
