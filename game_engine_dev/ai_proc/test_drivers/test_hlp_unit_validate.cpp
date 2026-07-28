//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "test_hlp_unit_validate.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#include "city.h"
#include "city_array.h"
#include "game_primitives.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "test_hlp_tile_ownership.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static void join_path (char* dst, size_t n, cstr base, cstr leaf) {
    if (base == nullptr || leaf == nullptr || n == 0u) {
        if (n > 0u) {
            dst[0] = '\0';
        }
        return;
    }
    const size_t bl = std::strlen(base);
    if (bl > 0u && base[bl - 1u] == '/') {
        std::snprintf(dst, n, "%s%s", base, leaf);
    } else {
        std::snprintf(dst, n, "%s/%s", base, leaf);
    }
}

static bool mkdir_p (cstr path) {
    return mkdir(path, 0755) == 0 || errno == EEXIST;
}

static void put_px (u8* rgb, u16 w, u16 h, i32 x, i32 y, u8 r, u8 g, u8 b) {
    if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i] = r;
    rgb[i + 1u] = g;
    rgb[i + 2u] = b;
}

static void draw_plus (u8* rgb, u16 w, u16 h, u16 cx, u16 cy, u8 r, u8 g, u8 b) {
    const i32 x0 = static_cast<i32>(cx);
    const i32 y0 = static_cast<i32>(cy);
    for (i32 dx = -2; dx <= 2; ++dx) {
        for (i32 t = -1; t <= 1; ++t) {
            put_px(rgb, w, h, x0 + dx, y0 + t, r, g, b);
        }
    }
    for (i32 dy = -2; dy <= 2; ++dy) {
        for (i32 t = -1; t <= 1; ++t) {
            put_px(rgb, w, h, x0 + t, y0 + dy, r, g, b);
        }
    }
}

static void paint_cities (u8* rgb, u16 w, u16 h, const GameState& s, u16 stx, u16 sty) {
    const u16 n = s.m_cities.get_city_count();
    for (u16 i = 0; i < n; ++i) {
        const City* c = s.m_cities.get_city(i);
        if (c == nullptr) {
            continue;
        }
        draw_plus(rgb, w, h, c->get_x(), c->get_y(), 20, 20, 20);
    }
    draw_plus(rgb, w, h, stx, sty, 40, 90, 255);
}

static void sec_rgb (u16 sid, u16 pa, u16 pb, u8* r, u8* g, u8* b) {
    if (sid == TestHlpTileOwnership::k_sec_none) {
        *r = 36;
        *g = 36;
        *b = 40;
        return;
    }
    if (sid == pa) {
        *r = 40;
        *g = 200;
        *b = 70;
        return;
    }
    if (sid == pb) {
        *r = 70;
        *g = 120;
        *b = 255;
        return;
    }
    const u32 s = static_cast<u32>(sid) + 1u;
    *r = static_cast<u8>(70u + ((s * 97u) % 120u));
    *g = static_cast<u8>(50u + ((s * 57u) % 100u));
    *b = static_cast<u8>(50u + ((s * 31u) % 100u));
}

static bool save_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static bool load_terr_ppm (cstr in_base, u16 w, u16 h, u8* rgb) {
    char path[512];
    join_path(path, sizeof(path), in_base, "terrain.ppm");
    FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    char mag[3] = {};
    if (std::fscanf(fp, "%2s", mag) != 1 || std::strcmp(mag, "P6") != 0) {
        std::fclose(fp);
        return false;
    }
    unsigned tw = 0;
    unsigned th = 0;
    unsigned maxv = 0;
    int ch = 0;
    do {
        ch = std::fgetc(fp);
    } while (ch == '#' || ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t');
    if (ch == EOF) {
        std::fclose(fp);
        return false;
    }
    std::ungetc(ch, fp);
    if (std::fscanf(fp, "%u %u %u", &tw, &th, &maxv) != 3 || maxv != 255u) {
        std::fclose(fp);
        return false;
    }
    if (tw != static_cast<unsigned>(w) || th != static_cast<unsigned>(h)) {
        std::fclose(fp);
        return false;
    }
    std::fgetc(fp);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fread(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static void blend_sec (u8* rgb, u16 w, u16 h, const u16* ov, u16 pa, u16 pb) {
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 sr = 0;
            u8 sg = 0;
            u8 sb = 0;
            sec_rgb(ov[tidx(w, x, y)], pa, pb, &sr, &sg, &sb);
            const u32 i = tidx(w, x, y) * 3u;
            rgb[i] = static_cast<u8>((static_cast<u16>(rgb[i]) + static_cast<u16>(sr)) / 2u);
            rgb[i + 1u] = static_cast<u8>((static_cast<u16>(rgb[i + 1u]) + static_cast<u16>(sg)) / 2u);
            rgb[i + 2u] = static_cast<u8>((static_cast<u16>(rgb[i + 2u]) + static_cast<u16>(sb)) / 2u);
        }
    }
}

static void fill_sec (u8* rgb, u16 w, u16 h, const u16* ov, u16 pa, u16 pb) {
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            sec_rgb(ov[tidx(w, x, y)], pa, pb, &r, &g, &b);
            const u32 i = tidx(w, x, y) * 3u;
            rgb[i] = r;
            rgb[i + 1u] = g;
            rgb[i + 2u] = b;
        }
    }
}

static bool prep_sec_rgb (
    const GameState& s,
    const u16* ov,
    u16 pa,
    u16 pb,
    cstr in_base,
    u8* rgb) {
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    if (load_terr_ppm(in_base, w, h, rgb)) {
        blend_sec(rgb, w, h, ov, pa, pb);
    } else {
        fill_sec(rgb, w, h, ov, pa, pb);
    }
    return true;
}

//================================================================================================================================
//=> - TestHlpUnitValidate -
//================================================================================================================================

void TestHlpUnitValidate::print_counts (const GameState& s) {
    if (s.m_statics == nullptr) {
        std::printf("units: (no statics)\n");
        return;
    }
    const RuntimeStatics& st = *s.m_statics;
    const u16 typ_n = st.unit().get_item_count();
    const u16 pn = s.m_player_n;
    const u32 scan_n = static_cast<u32>(UnitAddVector::MAX_PAGES)
        * static_cast<u32>(UnitAddVector::UNIT_ADD_ITEMS_PER_PAGE);
    u32 total = 0;
    std::printf("units:\n");
    for (u16 p = 0; p < pn; ++p) {
        u32 ptot = 0;
        for (u32 idx = 0; idx < scan_n; ++idx) {
            const UnitAddStruct* u = s.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
            if (u == nullptr || u->m_player_idx != p) {
                continue;
            }
            ptot++;
        }
        if (ptot == 0u) {
            continue;
        }
        total += ptot;
        std::printf("  player %u total=%u\n", (u32)p, ptot);
        for (u16 t = 0; t < typ_n; ++t) {
            u32 ct = 0;
            for (u32 idx = 0; idx < scan_n; ++idx) {
                const UnitAddStruct* u = s.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
                if (u == nullptr || u->m_player_idx != p || u->m_unit_typ_idx != t) {
                    continue;
                }
                ct++;
            }
            if (ct == 0u) {
                continue;
            }
            const char* nm = st.unit().get_name(UnitStaticDataKey::from_raw(t));
            if (nm == nullptr) {
                nm = "?";
            }
            std::printf("    %s: %u\n", nm, ct);
        }
    }
    std::printf("  grand_total=%u\n", total);
}

void TestHlpUnitValidate::print_at_xy (const GameState& s, u16 x, u16 y) {
    if (s.m_statics == nullptr) {
        std::printf("  at (%u,%u): (no statics)\n", (u32)x, (u32)y);
        return;
    }
    const RuntimeStatics& st = *s.m_statics;
    const u16 typ_n = st.unit().get_item_count();
    const u32 scan_n = static_cast<u32>(UnitAddVector::MAX_PAGES)
        * static_cast<u32>(UnitAddVector::UNIT_ADD_ITEMS_PER_PAGE);
    u32 total = 0;
    std::printf("  at (%u,%u):\n", (u32)x, (u32)y);
    for (u16 t = 0; t < typ_n; ++t) {
        u32 ct = 0;
        for (u32 idx = 0; idx < scan_n; ++idx) {
            const UnitAddStruct* u = s.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
            if (u == nullptr || u->m_x != x || u->m_y != y) {
                continue;
            }
            UnitAddKey cur = UnitAddKey::from_raw(static_cast<u16>(idx));
            while (cur.is_valid()) {
                const UnitAddStruct* c = s.m_units.get_unit_add(cur);
                if (c == nullptr) {
                    break;
                }
                if (c->m_unit_typ_idx == t) {
                    ct++;
                }
                if (c->m_next_unit_in_group == U16_KEY_NULL) {
                    break;
                }
                cur = UnitAddKey::from_raw(c->m_next_unit_in_group);
            }
        }
        if (ct == 0u) {
            continue;
        }
        total += ct;
        const char* nm = st.unit().get_name(UnitStaticDataKey::from_raw(t));
        if (nm == nullptr) {
            nm = "?";
        }
        std::printf("    %s: %u\n", nm, ct);
    }
    std::printf("    tile_total=%u\n", total);
}

bool TestHlpUnitValidate::ensure_out_dirs (cstr out_base) {
    if (out_base == nullptr) {
        return false;
    }
    char turns[512];
    join_path(turns, sizeof(turns), out_base, "turns");
    return mkdir_p(out_base) && mkdir_p(turns);
}

bool TestHlpUnitValidate::write_sectors (
    const GameState& s,
    const u16* ov,
    u16 pa,
    u16 pb,
    u16 stx,
    u16 sty,
    cstr in_base,
    cstr out_base) {
    if (ov == nullptr || in_base == nullptr || out_base == nullptr) {
        return false;
    }
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    u8* rgb = new u8[static_cast<size_t>(w) * static_cast<size_t>(h) * 3u];
    prep_sec_rgb(s, ov, pa, pb, in_base, rgb);
    paint_cities(rgb, w, h, s, stx, sty);
    char path[512];
    join_path(path, sizeof(path), out_base, "sectors.ppm");
    const bool ok = save_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

bool TestHlpUnitValidate::write_access (
    const GameState& s,
    const Whiteboard_1B& msk,
    u16 view,
    u16 stx,
    u16 sty,
    cstr in_base,
    cstr out_base) {
    if (in_base == nullptr || out_base == nullptr) {
        return false;
    }
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    u8* rgb = new u8[static_cast<size_t>(w) * static_cast<size_t>(h) * 3u];
    if (!load_terr_ppm(in_base, w, h, rgb)) {
        std::memset(rgb, 90, static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 own = s.m_map.get_civ_owner(x, y);
            const u32 i = tidx(w, x, y) * 3u;
            if (own == static_cast<u8>(view)) {
                rgb[i] = 255;
                rgb[i + 1u] = 255;
                rgb[i + 2u] = 255;
            } else if (own != U8_KEY_NULL && msk.rd(x, y) == 0u) {
                rgb[i] = 255;
                rgb[i + 1u] = 140;
                rgb[i + 2u] = 140;
            }
        }
    }
    paint_cities(rgb, w, h, s, stx, sty);
    char path[512];
    join_path(path, sizeof(path), out_base, "access.ppm");
    const bool ok = save_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

bool TestHlpUnitValidate::write_turn (
    const GameState& s,
    const u16* ov,
    u16 pa,
    u16 pb,
    u16 focus_x,
    u16 focus_y,
    u32 turn,
    cstr in_base,
    cstr out_base) {
    if (ov == nullptr || in_base == nullptr || out_base == nullptr) {
        return false;
    }
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    u8* rgb = new u8[static_cast<size_t>(w) * static_cast<size_t>(h) * 3u];
    prep_sec_rgb(s, ov, pa, pb, in_base, rgb);
    paint_cities(rgb, w, h, s, focus_x, focus_y);
    const u32 scan_n = static_cast<u32>(UnitAddVector::MAX_PAGES)
        * static_cast<u32>(UnitAddVector::UNIT_ADD_ITEMS_PER_PAGE);
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const UnitAddStruct* u = s.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            continue;
        }
        const u8 pr = (u->m_player_idx == pa) ? 255 : 40;
        const u8 pg = (u->m_player_idx == pa) ? 40 : 40;
        const u8 pb2 = (u->m_player_idx == pa) ? 40 : 255;
        put_px(rgb, w, h, static_cast<i32>(u->m_x), static_cast<i32>(u->m_y), pr, pg, pb2);
    }
    char turns[512];
    char path[576];
    join_path(turns, sizeof(turns), out_base, "turns");
    if (!mkdir_p(turns)) {
        delete[] rgb;
        return false;
    }
    if (std::snprintf(path, sizeof(path), "%s/turn_%04u.ppm", turns, (unsigned)turn) <= 0) {
        delete[] rgb;
        return false;
    }
    const bool ok = save_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
