//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "game_loop_cache.h"

#include <cstdio>
#include <cstring>

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u32 k_map_magic = 0x504d4c47u;
static const u32 k_starts_magic = 0x54534c47u;
static const u32 k_cache_ver = 1u;

//================================================================================================================================
//=> - GameLoopCache -
//================================================================================================================================

bool GameLoopCache::map_exists (cstr map_path, cstr starts_path) {
    if (map_path == nullptr || starts_path == nullptr) {
        return false;
    }
    std::FILE* fm = std::fopen(map_path, "rb");
    if (fm == nullptr) {
        return false;
    }
    std::fclose(fm);
    std::FILE* fs = std::fopen(starts_path, "rb");
    if (fs == nullptr) {
        return false;
    }
    std::fclose(fs);
    return true;
}

bool GameLoopCache::save_map (cstr path, const GameArraySimple& map) {
    if (path == nullptr) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = map.tile_n();
    if (w == 0 || h == 0 || n == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    const u32 magic = k_map_magic;
    const u32 ver = k_cache_ver;
    if (std::fwrite(&magic, sizeof(magic), 1, fp) != 1
        || std::fwrite(&ver, sizeof(ver), 1, fp) != 1
        || std::fwrite(&w, sizeof(w), 1, fp) != 1
        || std::fwrite(&h, sizeof(h), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        const GameTileSimple& t = map.m_tiles[i];
        if (std::fwrite(&t.m_terr, sizeof(t.m_terr), 1, fp) != 1
            || std::fwrite(&t.m_clim, sizeof(t.m_clim), 1, fp) != 1
            || std::fwrite(&t.m_ov, sizeof(t.m_ov), 1, fp) != 1
            || std::fwrite(&t.m_riv, sizeof(t.m_riv), 1, fp) != 1
            || std::fwrite(&t.m_res, sizeof(t.m_res), 1, fp) != 1) {
            std::fclose(fp);
            return false;
        }
    }
    std::fclose(fp);
    return true;
}

bool GameLoopCache::load_map (cstr path, GameArraySimple* out) {
    if (path == nullptr || out == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    u32 magic = 0;
    u32 ver = 0;
    u16 w = 0;
    u16 h = 0;
    if (std::fread(&magic, sizeof(magic), 1, fp) != 1
        || std::fread(&ver, sizeof(ver), 1, fp) != 1
        || std::fread(&w, sizeof(w), 1, fp) != 1
        || std::fread(&h, sizeof(h), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    if (magic != k_map_magic || ver != k_cache_ver || w == 0 || h == 0) {
        std::fclose(fp);
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    GameTileSimple* tiles = new GameTileSimple[n];
    for (u32 i = 0; i < n; ++i) {
        GameTileSimple* t = &tiles[i];
        t->m_unit_hd = U16_KEY_NULL;
        t->m_add_idx = U16_KEY_NULL;
        t->m_add_typ = 0;
        if (std::fread(&t->m_terr, sizeof(t->m_terr), 1, fp) != 1
            || std::fread(&t->m_clim, sizeof(t->m_clim), 1, fp) != 1
            || std::fread(&t->m_ov, sizeof(t->m_ov), 1, fp) != 1
            || std::fread(&t->m_riv, sizeof(t->m_riv), 1, fp) != 1
            || std::fread(&t->m_res, sizeof(t->m_res), 1, fp) != 1) {
            delete[] tiles;
            std::fclose(fp);
            return false;
        }
    }
    std::fclose(fp);
    out->clear();
    out->m_w = w;
    out->m_h = h;
    out->m_tiles = tiles;
    return true;
}

bool GameLoopCache::save_starts (cstr path, const SpgPickCoords& starts, u16 player_n) {
    if (path == nullptr || player_n == 0 || starts.n != static_cast<u32>(player_n)) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    const u32 magic = k_starts_magic;
    const u32 ver = k_cache_ver;
    const u32 pn = static_cast<u32>(player_n);
    const u32 pick_n = starts.n;
    if (std::fwrite(&magic, sizeof(magic), 1, fp) != 1
        || std::fwrite(&ver, sizeof(ver), 1, fp) != 1
        || std::fwrite(&pn, sizeof(pn), 1, fp) != 1
        || std::fwrite(&pick_n, sizeof(pick_n), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    for (u32 i = 0; i < pick_n; ++i) {
        const u16 x = starts.pts[i].x;
        const u16 y = starts.pts[i].y;
        if (std::fwrite(&x, sizeof(x), 1, fp) != 1
            || std::fwrite(&y, sizeof(y), 1, fp) != 1) {
            std::fclose(fp);
            return false;
        }
    }
    std::fclose(fp);
    return true;
}

bool GameLoopCache::load_starts (cstr path, SpgPickCoords* out, u16 player_n) {
    if (path == nullptr || out == nullptr || player_n == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    u32 magic = 0;
    u32 ver = 0;
    u32 pn = 0;
    u32 pick_n = 0;
    if (std::fread(&magic, sizeof(magic), 1, fp) != 1
        || std::fread(&ver, sizeof(ver), 1, fp) != 1
        || std::fread(&pn, sizeof(pn), 1, fp) != 1
        || std::fread(&pick_n, sizeof(pick_n), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    if (magic != k_starts_magic || ver != k_cache_ver || pn != static_cast<u32>(player_n)) {
        std::fclose(fp);
        return false;
    }
    if (pick_n == 0 || pick_n > SPG_MAX_PICK_PTS || pick_n != static_cast<u32>(player_n)) {
        std::fclose(fp);
        return false;
    }
    out->n = 0;
    for (u32 i = 0; i < pick_n; ++i) {
        u16 x = 0;
        u16 y = 0;
        if (std::fread(&x, sizeof(x), 1, fp) != 1
            || std::fread(&y, sizeof(y), 1, fp) != 1) {
            out->n = 0;
            std::fclose(fp);
            return false;
        }
        out->pts[i].x = x;
        out->pts[i].y = y;
    }
    out->n = pick_n;
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
