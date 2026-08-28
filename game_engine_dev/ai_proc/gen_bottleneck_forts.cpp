//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "gen_bottleneck_forts.h"

#include <cstring>

#include "assert_log.h"
#include "game_array_simple.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

#define GBF_ZOC_5X5 1 // 1 => 5x5 ZoC / 25 tiles per fort; 0 => 3x3 / 9

#if GBF_ZOC_5X5
static const i32 GBF_ZOC_R = 2;
static const u32 GBF_TILES_PER = 25u;
#else
static const i32 GBF_ZOC_R = 1;
static const u32 GBF_TILES_PER = 9u;
#endif

static u32 ceil_div_per (u32 n) {
    if (n == 0u) {
        return 0u;
    }
    return (n + GBF_TILES_PER - 1u) / GBF_TILES_PER;
}

//================================================================================================================================
//=> - GenBottleneckForts -
//================================================================================================================================

GenBottleneckForts::GenBottleneckForts () :
    m_map(nullptr),
    m_net(),
    m_walk(),
    m_mark(),
    m_fort("GenBottleneckForts", "fort", 0u),
    m_fort_n(0),
    m_ok(false) {
}

GenBottleneckForts::~GenBottleneckForts () {
}

bool GenBottleneckForts::begin (const GameArraySimple& map) {
    m_map = &map;
    m_ok = m_fort.ok() && map.width() == m_fort.w() && map.height() == m_fort.h();
    GAME_EXPECT(m_ok, "GenBottleneckForts begin whiteboard checkout failed");
    clr();
    return m_ok;
}

void GenBottleneckForts::clr () {
    m_fort_n = 0;
    if (!m_fort.ok()) {
        m_ok = false;
        return;
    }
    std::memset(m_fort.get_iter_ptr(), 0, static_cast<size_t>(WhiteboardMng::tile_n()) * sizeof(u8));
}

bool GenBottleneckForts::ok () const {
    return m_ok;
}

u32 GenBottleneckForts::mark_n () const {
    return m_mark.mark_n();
}

u32 GenBottleneckForts::fort_n () const {
    return m_fort_n;
}

i32 GenBottleneckForts::zoc_r () {
    return GBF_ZOC_R;
}

const Whiteboard_1B& GenBottleneckForts::marks () const {
    return m_mark.marks();
}

const Whiteboard_1B& GenBottleneckForts::forts () const {
    return m_fort;
}

const Whiteboard_2B& GenBottleneckForts::walk_secs () const {
    return m_walk.sectors();
}

bool GenBottleneckForts::build () {
    GAME_EXPECT_RET(m_ok && m_map != nullptr, false, "GenBottleneckForts build not begun");
    clr();
    const u16 w = m_map->width();
    const u16 h = m_map->height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* terr = new u8[n];
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            terr[static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)] = m_map->get_terrain(x, y);
        }
    }
    if (!m_net.begin(w, h, terr)) {
        delete[] terr;
        return false;
    }
    delete[] terr;
    if (!m_walk.begin(m_net, *m_map) || !m_walk.build()) {
        return false;
    }
    if (!m_mark.begin(m_net, m_walk, *m_map) || !m_mark.mark()) {
        return false;
    }
    place_all();
    return true;
}

void GenBottleneckForts::place_all () {
    const u16 w = m_map->width();
    const u16 h = m_map->height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u16 sn = m_net.sector_n();
    const Whiteboard_1B& mk = m_mark.marks();
    const Whiteboard_2B& sec = m_walk.sectors();
    u32* cnt = new u32[sn]();
    for (u32 i = 0; i < n; ++i) {
        if (mk.rd_i(i) == 0u) {
            continue;
        }
        const u16 tag = sec.rd_i(i);
        if (tag == GWS_IDX_NONE) {
            continue;
        }
        const u16 id = static_cast<u16>(tag - 1u);
        if (id < sn) {
            cnt[id] = cnt[id] + 1u;
        }
    }
    u32* off = new u32[static_cast<u32>(sn) + 1u];
    off[0] = 0u;
    for (u16 id = 0; id < sn; ++id) {
        off[id + 1u] = off[id] + cnt[id];
    }
    const u32 tot = off[sn];
    u32* tiles = tot > 0u ? new u32[tot] : nullptr;
    std::memset(cnt, 0, static_cast<size_t>(sn) * sizeof(u32));
    for (u32 i = 0; i < n; ++i) {
        if (mk.rd_i(i) == 0u) {
            continue;
        }
        const u16 tag = sec.rd_i(i);
        if (tag == GWS_IDX_NONE) {
            continue;
        }
        const u16 id = static_cast<u16>(tag - 1u);
        if (id >= sn) {
            continue;
        }
        tiles[off[id] + cnt[id]] = i;
        cnt[id] = cnt[id] + 1u;
    }
    u8* in_sec = new u8[n];
    u8* zoc = new u8[n];
    std::memset(zoc, 0, static_cast<size_t>(n));
    for (u16 id = 0; id < sn; ++id) {
        if (cnt[id] == 0u) {
            continue;
        }
        std::memset(in_sec, 0, static_cast<size_t>(n));
        const u32* st = tiles + off[id];
        for (u32 k = 0; k < cnt[id]; ++k) {
            in_sec[st[k]] = 1u;
        }
        m_fort_n += place_sec(st, cnt[id], in_sec, zoc);
    }
    delete[] zoc;
    delete[] in_sec;
    delete[] tiles;
    delete[] off;
    delete[] cnt;
}

u32 GenBottleneckForts::place_sec (const u32* tiles, u32 tn, u8* in_sec, u8* zoc) {
    const u16 w = m_map->width();
    const u16 h = m_map->height();
    const u32 wi = static_cast<u32>(w);
    const u32 need = ceil_div_per(tn);
    u32 placed = 0;
    for (u32 f = 0; f < need; ++f) {
        bool force_in = (f == 0u);
        i32 best_z = -1;
        i32 best_c = -1;
        u32 best_ti = 0xFFFFFFFFu;
        for (;;) {
            best_z = -1;
            best_c = -1;
            best_ti = 0xFFFFFFFFu;
            for (u32 k = 0; k < tn; ++k) {
                const u32 ti = tiles[k];
                if (m_fort.rd_i(ti) != 0u) {
                    continue;
                }
                const u16 cx = static_cast<u16>(ti % wi);
                const u16 cy = static_cast<u16>(ti / wi);
                if (m_map->get_res(cx, cy) != U16_KEY_NULL) {
                    continue;
                }
                i32 add_z = 0;
                i32 add_c = 0;
                bool ok = true;
                bool contained = true;
                for (i32 dy = -GBF_ZOC_R; dy <= GBF_ZOC_R && ok; ++dy) {
                    for (i32 dx = -GBF_ZOC_R; dx <= GBF_ZOC_R && ok; ++dx) {
                        const i32 nx = static_cast<i32>(cx) + dx;
                        const i32 ny = static_cast<i32>(cy) + dy;
                        if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                            contained = false;
                            continue;
                        }
                        const u32 ni = static_cast<u32>(ny) * wi + static_cast<u32>(nx);
                        if (zoc[ni] != 0u) {
                            ok = false;
                            break;
                        }
                        ++add_z;
                        if (in_sec[ni] != 0u) {
                            ++add_c;
                        } else {
                            contained = false;
                        }
                    }
                }
                if (!ok) {
                    continue;
                }
                if (force_in && !contained) {
                    continue;
                }
                if (add_z > best_z || (add_z == best_z && add_c > best_c)) {
                    best_z = add_z;
                    best_c = add_c;
                    best_ti = ti;
                }
            }
            if (best_ti != 0xFFFFFFFFu || !force_in) {
                break;
            }
            force_in = false;
        }
        if (best_ti == 0xFFFFFFFFu) {
            break;
        }
        const u16 cx = static_cast<u16>(best_ti % wi);
        const u16 cy = static_cast<u16>(best_ti / wi);
        m_fort.wr(cx, cy, 1u);
        for (i32 dy = -GBF_ZOC_R; dy <= GBF_ZOC_R; ++dy) {
            for (i32 dx = -GBF_ZOC_R; dx <= GBF_ZOC_R; ++dx) {
                const i32 nx = static_cast<i32>(cx) + dx;
                const i32 ny = static_cast<i32>(cy) + dy;
                if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                    continue;
                }
                zoc[static_cast<u32>(ny) * wi + static_cast<u32>(nx)] = 1u;
            }
        }
        ++placed;
    }
    return placed;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
