//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "map_own_xfer_save_timeline_mng.h"

#include <cstdio>

#include "eval_paths.h"
#include "game_array_simple.h"
#include "game_io.h"
#include "map_own_xfer_save_snap.h"

//================================================================================================================================
//=> - MapOwnXferSaveTimelineMng -
//================================================================================================================================

MapOwnXferSaveTimelineMng::MapOwnXferSaveTimelineMng ()
    : m_save_n(0),
      m_wrote_n(0) {
}

MapOwnXferSaveTimelineMng::~MapOwnXferSaveTimelineMng () {
    clr();
}

void MapOwnXferSaveTimelineMng::clr () {
    m_save_n = 0;
    m_wrote_n = 0;
}

bool MapOwnXferSaveTimelineMng::setup (u16 save_n) {
    clr();
    if (save_n < 2) {
        return false;
    }
    m_save_n = save_n;
    return true;
}

bool MapOwnXferSaveTimelineMng::fill (const EvalPaths& paths, cstr out_dir) {
    if (out_dir == nullptr || out_dir[0] == 0 || !paths.ok() || paths.save_turn_n() != m_save_n) {
        return false;
    }
    m_wrote_n = 0;
    char map_p[512];
    if (!paths.map_path(paths.save_turn_at(0), map_p, sizeof(map_p))) {
        return false;
    }
    GameArraySimple buf_a;
    GameArraySimple buf_b;
    if (!GameIo::load_map_tiles(map_p, buf_a)) {
        return false;
    }
    const u16 w = buf_a.width();
    const u16 h = buf_a.height();
    if (w == 0 || h == 0) {
        return false;
    }
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* acc = new u8[tile_n];
    if (acc == nullptr) {
        return false;
    }
    for (u32 i = 0; i < tile_n; ++i) {
        acc[i] = U8_KEY_NULL;
    }
    GameArraySimple* prev = &buf_a;
    GameArraySimple* cur = &buf_b;
    for (u16 si = 1; si < m_save_n; ++si) {
        const u32 turn = paths.save_turn_at(si);
        if (!paths.map_path(turn, map_p, sizeof(map_p))) {
            delete[] acc;
            return false;
        }
        char out_p[512];
        if (std::snprintf(out_p, sizeof(out_p), "%s/xfer_t%04u.ppm", out_dir, turn) <= 0) {
            delete[] acc;
            return false;
        }
        cur->clear();
        if (!GameIo::load_map_tiles(map_p, *cur)) {
            delete[] acc;
            return false;
        }
        if (cur->width() != w || cur->height() != h) {
            delete[] acc;
            return false;
        }
        for (u16 y = 0; y < h; ++y) {
            for (u16 x = 0; x < w; ++x) {
                const u8 now = cur->get_civ_owner(x, y);
                const u8 was = prev->get_civ_owner(x, y);
                if (now == was || was == U8_KEY_NULL || now == U8_KEY_NULL) {
                    continue;
                }
                const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
                acc[i] = now;
            }
        }
        if (!MapOwnXferSaveSnap::write(out_p, *cur, *prev)) {
            delete[] acc;
            return false;
        }
        GameArraySimple* tmp = prev;
        prev = cur;
        cur = tmp;
        m_wrote_n = static_cast<u16>(m_wrote_n + 1u);
        std::printf("wrote %s\n", out_p);
    }
    char all_p[512];
    if (std::snprintf(all_p, sizeof(all_p), "%s/xfer_all.ppm", out_dir) <= 0) {
        delete[] acc;
        return false;
    }
    if (!MapOwnXferSaveSnap::write_seats(all_p, *prev, acc)) {
        delete[] acc;
        return false;
    }
    std::printf("wrote %s\n", all_p);
    delete[] acc;
    return true;
}

u16 MapOwnXferSaveTimelineMng::save_n () const {
    return m_save_n;
}

u16 MapOwnXferSaveTimelineMng::wrote_n () const {
    return m_wrote_n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
