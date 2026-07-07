//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "test_support.h"

#include <cstdio>
#include <cstring>

//================================================================================================================================
//=> - ExploredRiverOverlay -
//================================================================================================================================

ExploredRiverOverlay::ExploredRiverOverlay (u16 w, u16 h) :
    m_ov(w, h) {}

u16 ExploredRiverOverlay::width () const {
    return m_ov.width();
}

u16 ExploredRiverOverlay::height () const {
    return m_ov.height();
}

u32 ExploredRiverOverlay::get (u16 x, u16 y) const {
    return m_ov.get(x, y);
}

bool ExploredRiverOverlay::set (u16 x, u16 y) {
    return m_ov.set(x, y);
}

void ExploredRiverOverlay::pull (
    const GameArraySimple& map,
    const MapBitOverlay& exp) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_river(x, y) == 0u) {
                continue;
            }
            if (exp.get(x, y) == 0u) {
                continue;
            }
            m_ov.set(x, y);
        }
    }
}

u32 ExploredRiverOverlay::cnt () const {
    u32 n = 0;
    for (u16 y = 0; y < m_ov.height(); ++y) {
        for (u16 x = 0; x < m_ov.width(); ++x) {
            n += m_ov.get(x, y);
        }
    }
    return n;
}

//================================================================================================================================
//=> - RiverSystemFinder -
//================================================================================================================================

RiverSystemFinder::RiverSystemFinder (
    const GameArraySimple& map,
    const ExploredRiverOverlay& riv_exp) :
    m_map(map),
    m_re(riv_exp) {}

bool RiverSystemFinder::find_next (u16& ox, u16& oy) const {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (m_map.get_river(x, y) == 0u) {
                continue;
            }
            if (m_re.get(x, y) != 0u) {
                continue;
            }
            ox = x;
            oy = y;
            return true;
        }
    }
    return false;
}

u32 RiverSystemFinder::riv_tot () const {
    u32 n = 0;
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (m_map.get_river(x, y) != 0u) {
                ++n;
            }
        }
    }
    return n;
}

//================================================================================================================================
//=> - RivSysImageAudit -
//================================================================================================================================

RivSysImageAudit::RivSysImageAudit () :
    m_rgb(nullptr),
    m_sid(nullptr),
    m_riv(nullptr),
    m_stats(nullptr),
    m_w(0),
    m_h(0),
    m_sys_n(0) {
    m_clrs.m_riv_b[0] = 0u;
    m_clrs.m_riv_b[1] = 0u;
    m_clrs.m_riv_b[2] = 255u;
    m_clrs.m_riv_w[0] = 255u;
    m_clrs.m_riv_w[1] = 255u;
    m_clrs.m_riv_w[2] = 255u;
    m_clrs.m_red[0] = 255u;
    m_clrs.m_red[1] = 0u;
    m_clrs.m_red[2] = 0u;
    m_clrs.m_grn[0] = 0u;
    m_clrs.m_grn[1] = 255u;
    m_clrs.m_grn[2] = 0u;
    m_clrs.m_gry[0] = 128u;
    m_clrs.m_gry[1] = 128u;
    m_clrs.m_gry[2] = 128u;
}

RivSysImageAudit::~RivSysImageAudit () {
    clr_buf();
}

void RivSysImageAudit::set_clrs (const RivSysClrs& clrs) {
    m_clrs = clrs;
}

void RivSysImageAudit::clr_buf () {
    delete[] m_rgb;
    delete[] m_sid;
    delete[] m_riv;
    delete[] m_stats;
    m_rgb = nullptr;
    m_sid = nullptr;
    m_riv = nullptr;
    m_stats = nullptr;
    m_w = 0;
    m_h = 0;
    m_sys_n = 0;
}

bool RivSysImageAudit::load (const char* path) {
    clr_buf();
    FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    char magic[3];
    if (std::fread(magic, 1, 3, fp) != 3 || magic[0] != 'P' || magic[1] != '6') {
        std::fclose(fp);
        return false;
    }
    int iw = 0;
    int ih = 0;
    int imax = 0;
    if (std::fscanf(fp, " %d %d %d", &iw, &ih, &imax) != 3 || imax != 255) {
        std::fclose(fp);
        return false;
    }
    if (iw <= 0 || ih <= 0 || iw > 65535 || ih > 65535) {
        std::fclose(fp);
        return false;
    }
    m_w = static_cast<u16>(iw);
    m_h = static_cast<u16>(ih);
    const u32 n = static_cast<u32>(m_w) * static_cast<u32>(m_h);
    m_rgb = new u8[static_cast<size_t>(n) * 3u];
    m_sid = new u16[static_cast<size_t>(n)];
    m_riv = new u8[static_cast<size_t>(n)];
    if (m_rgb == nullptr || m_sid == nullptr || m_riv == nullptr) {
        clr_buf();
        std::fclose(fp);
        return false;
    }
    std::fgetc(fp);
    const size_t nbytes = static_cast<size_t>(n) * 3u;
    if (std::fread(m_rgb, 1, nbytes, fp) != nbytes) {
        clr_buf();
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    for (u32 i = 0; i < n; ++i) {
        const u16 x = static_cast<u16>(i % static_cast<u32>(m_w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(m_w));
        const bool rv = is_riv_px(x, y);
        m_riv[i] = rv ? 1u : 0u;
        m_sid[i] = rv ? k_unm : 0u;
    }
    return true;
}

bool RivSysImageAudit::rgb_eq (u16 x, u16 y, const u8 c[3]) const {
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(m_w) + static_cast<u32>(x)) * 3u;
    return m_rgb[i + 0] == c[0] && m_rgb[i + 1] == c[1] && m_rgb[i + 2] == c[2];
}

bool RivSysImageAudit::is_riv_px (u16 x, u16 y) const {
    return rgb_eq(x, y, m_clrs.m_riv_b)
        || rgb_eq(x, y, m_clrs.m_riv_w)
        || rgb_eq(x, y, m_clrs.m_red)
        || rgb_eq(x, y, m_clrs.m_grn)
        || rgb_eq(x, y, m_clrs.m_gry);
}

bool RivSysImageAudit::is_red_px (u16 x, u16 y) const {
    return rgb_eq(x, y, m_clrs.m_red);
}

bool RivSysImageAudit::is_grn_px (u16 x, u16 y) const {
    return rgb_eq(x, y, m_clrs.m_grn);
}

bool RivSysImageAudit::is_gry_px (u16 x, u16 y) const {
    return rgb_eq(x, y, m_clrs.m_gry);
}

void RivSysImageAudit::flood_sys (u16 sx, u16 sy, u16 id) {
    const u32 cap = static_cast<u32>(m_w) * static_cast<u32>(m_h);
    u16* stk_x = new u16[cap];
    u16* stk_y = new u16[cap];
    if (stk_x == nullptr || stk_y == nullptr) {
        delete[] stk_x;
        delete[] stk_y;
        return;
    }
    u32 sn = 0;
    stk_x[sn] = sx;
    stk_y[sn] = sy;
    ++sn;
    const u32 si0 = static_cast<u32>(sy) * static_cast<u32>(m_w) + static_cast<u32>(sx);
    m_sid[si0] = id;
    static const i32 dx4[4] = {0, 0, -1, 1};
    static const i32 dy4[4] = {-1, 1, 0, 0};
    while (sn > 0u) {
        --sn;
        const u16 cx = stk_x[sn];
        const u16 cy = stk_y[sn];
        for (u32 k = 0; k < 4u; ++k) {
            const i32 nx = static_cast<i32>(cx) + dx4[k];
            const i32 ny = static_cast<i32>(cy) + dy4[k];
            if (nx < 0 || ny < 0) {
                continue;
            }
            const u16 ax = static_cast<u16>(nx);
            const u16 ay = static_cast<u16>(ny);
            if (ax >= m_w || ay >= m_h) {
                continue;
            }
            const u32 si = static_cast<u32>(ay) * static_cast<u32>(m_w) + static_cast<u32>(ax);
            if (m_riv[si] == 0u || m_sid[si] != k_unm) {
                continue;
            }
            m_sid[si] = id;
            stk_x[sn] = ax;
            stk_y[sn] = ay;
            ++sn;
        }
    }
    delete[] stk_x;
    delete[] stk_y;
}

bool RivSysImageAudit::analyze () {
    if (m_rgb == nullptr || m_sid == nullptr || m_riv == nullptr) {
        return false;
    }
    delete[] m_stats;
    m_stats = nullptr;
    m_sys_n = 0;
    for (u16 y = 0; y < m_h; ++y) {
        for (u16 x = 0; x < m_w; ++x) {
            const u32 si = static_cast<u32>(y) * static_cast<u32>(m_w) + static_cast<u32>(x);
            if (m_riv[si] == 0u || m_sid[si] != k_unm) {
                continue;
            }
            flood_sys(x, y, static_cast<u16>(m_sys_n));
            ++m_sys_n;
        }
    }
    if (m_sys_n == 0u) {
        return true;
    }
    m_stats = new RivSysStat[m_sys_n];
    if (m_stats == nullptr) {
        m_sys_n = 0;
        return false;
    }
    for (u32 i = 0; i < m_sys_n; ++i) {
        m_stats[i].m_tiles = 0;
        m_stats[i].m_red = 0;
        m_stats[i].m_grn = 0;
        m_stats[i].m_gry = 0;
    }
    for (u16 y = 0; y < m_h; ++y) {
        for (u16 x = 0; x < m_w; ++x) {
            const u32 si = static_cast<u32>(y) * static_cast<u32>(m_w) + static_cast<u32>(x);
            if (m_riv[si] == 0u) {
                continue;
            }
            const u16 id = m_sid[si];
            if (id >= m_sys_n) {
                continue;
            }
            RivSysStat& st = m_stats[id];
            ++st.m_tiles;
            if (is_red_px(x, y)) {
                ++st.m_red;
            }
            if (is_grn_px(x, y)) {
                ++st.m_grn;
            }
            if (is_gry_px(x, y)) {
                ++st.m_gry;
            }
        }
    }
    return true;
}

void RivSysImageAudit::print_stats () const {
    for (u32 i = 0; i < m_sys_n; ++i) {
        const RivSysStat& st = m_stats[i];
        std::printf("system %u: tiles=%u green=%u gray=%u ",
            static_cast<unsigned>(i),
            static_cast<unsigned>(st.m_tiles),
            static_cast<unsigned>(st.m_grn),
            static_cast<unsigned>(st.m_gry));
        if (st.m_red != 1u) {
            std::printf("\033[31mstarts=%u\033[0m", static_cast<unsigned>(st.m_red));
        } else {
            std::printf("starts=%u", static_cast<unsigned>(st.m_red));
        }
        std::printf("\n");
    }
}

void RivSysImageAudit::pal_rgb (u32 id, u8& r, u8& g, u8& b) {
    const u32 t = id * 97u + 17u;
    r = static_cast<u8>(128u + (t * 53u) % 128u);
    g = static_cast<u8>(128u + (t * 97u) % 128u);
    b = static_cast<u8>(128u + (t * 151u) % 128u);
}

bool RivSysImageAudit::save_sys_map (const char* path) const {
    if (m_rgb == nullptr || m_sid == nullptr || m_riv == nullptr || m_sys_n == 0u) {
        return false;
    }
    const u32 n = static_cast<u32>(m_w) * static_cast<u32>(m_h);
    u8* out = new u8[static_cast<size_t>(n) * 3u];
    if (out == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        out[i * 3u + 0] = 0;
        out[i * 3u + 1] = 0;
        out[i * 3u + 2] = 0;
    }
    for (u16 y = 0; y < m_h; ++y) {
        for (u16 x = 0; x < m_w; ++x) {
            const u32 si = static_cast<u32>(y) * static_cast<u32>(m_w) + static_cast<u32>(x);
            if (m_riv[si] == 0u) {
                continue;
            }
            const u16 id = m_sid[si];
            if (id >= m_sys_n) {
                continue;
            }
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            pal_rgb(id, r, g, b);
            out[si * 3u + 0] = r;
            out[si * 3u + 1] = g;
            out[si * 3u + 2] = b;
        }
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        delete[] out;
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)m_w, (unsigned)m_h);
    const size_t nbytes = static_cast<size_t>(n) * 3u;
    const bool ok = std::fwrite(out, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    delete[] out;
    return ok;
}

bool RivSysImageAudit::save_bad_map (const char* path) const {
    if (m_rgb == nullptr || m_sid == nullptr || m_riv == nullptr
        || m_stats == nullptr || m_sys_n == 0u) {
        return false;
    }
    const u32 n = static_cast<u32>(m_w) * static_cast<u32>(m_h);
    u8* out = new u8[static_cast<size_t>(n) * 3u];
    if (out == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        out[i * 3u + 0] = 0;
        out[i * 3u + 1] = 0;
        out[i * 3u + 2] = 0;
    }
    for (u16 y = 0; y < m_h; ++y) {
        for (u16 x = 0; x < m_w; ++x) {
            const u32 si = static_cast<u32>(y) * static_cast<u32>(m_w) + static_cast<u32>(x);
            if (m_riv[si] == 0u) {
                continue;
            }
            const u16 id = m_sid[si];
            if (id >= m_sys_n || m_stats[id].m_red == 1u) {
                continue;
            }
            out[si * 3u + 0] = m_clrs.m_red[0];
            out[si * 3u + 1] = m_clrs.m_red[1];
            out[si * 3u + 2] = m_clrs.m_red[2];
        }
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        delete[] out;
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)m_w, (unsigned)m_h);
    const size_t nbytes = static_cast<size_t>(n) * 3u;
    const bool ok = std::fwrite(out, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    delete[] out;
    return ok;
}

u32 RivSysImageAudit::bad_st_n () const {
    u32 n = 0;
    for (u32 i = 0; i < m_sys_n; ++i) {
        if (m_stats[i].m_red != 1u) {
            ++n;
        }
    }
    return n;
}

bool re_riv_audit_run (const char* src, const char* sys_map, const char* bad_map) {
    RivSysImageAudit aud;
    if (!aud.load(src)) {
        std::printf("*** FAILED riv audit load %s\n", src);
        return false;
    }
    if (!aud.analyze()) {
        std::printf("*** FAILED riv audit analyze %s\n", src);
        return false;
    }
    if (!aud.save_sys_map(sys_map)) {
        std::printf("*** FAILED riv audit save %s\n", sys_map);
        return false;
    }
    if (!aud.save_bad_map(bad_map)) {
        std::printf("*** FAILED riv audit save %s\n", bad_map);
        return false;
    }
    const u32 bad = aud.bad_st_n();
    std::printf("--- riv audit ---\n");
    std::printf("systems %u bad_starts %u sysmap %s badmap %s\n",
        static_cast<unsigned>(aud.sys_n()),
        static_cast<unsigned>(bad),
        sys_map,
        bad_map);
    aud.print_stats();
    if (bad > 0u) {
        std::printf("*** FAILED riv audit %u systems without exactly one start\n",
            static_cast<unsigned>(bad));
        return false;
    }
    std::printf("*** PASSED riv audit\n");
    return true;
}

u32 RivSysImageAudit::sys_n () const {
    return m_sys_n;
}

const RivSysStat& RivSysImageAudit::stat (u32 i) const {
    static RivSysStat z = {0, 0, 0, 0};
    if (m_stats == nullptr || i >= m_sys_n) {
        return z;
    }
    return m_stats[i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
