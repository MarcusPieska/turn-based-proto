//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "gen_land_sector_network.h"

#include <cstring>

#include "gen_land_sectors.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};

//================================================================================================================================
//=> - GenLandSectorNetwork -
//================================================================================================================================

bool GenLandSectorNetwork::build (const Whiteboard_2B& sec, u16 sec_n, LandSectorNetwork* out) {
    if (out == nullptr || !sec.ok() || sec_n == 0u) {
        return false;
    }
    out->clr();
    const u16 w = sec.w();
    const u16 h = sec.h();
    if (w == 0u || h == 0u || WhiteboardMng::width() != w || WhiteboardMng::height() != h) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 wi = static_cast<u32>(w);
    const u32 pair_n = static_cast<u32>(sec_n) * static_cast<u32>(sec_n);
    u8* seen = new u8[pair_n];
    std::memset(seen, 0, static_cast<size_t>(pair_n));
    u32* sum_x = new u32[pair_n];
    u32* sum_y = new u32[pair_n];
    u32* touch_n = new u32[pair_n];
    std::memset(sum_x, 0, static_cast<size_t>(pair_n) * sizeof(u32));
    std::memset(sum_y, 0, static_cast<size_t>(pair_n) * sizeof(u32));
    std::memset(touch_n, 0, static_cast<size_t>(pair_n) * sizeof(u32));
    u16 link_n = 0u;
    for (u32 ti = 0; ti < n; ++ti) {
        const u16 ta = sec.rd_i(ti);
        if (ta == GLS_IDX_NONE || ta > sec_n) {
            continue;
        }
        const u32 py = ti / wi;
        const u32 px = ti - py * wi;
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(px) + k_dx4[d];
            const i32 ny = static_cast<i32>(py) + k_dy4[d];
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u32 ni = static_cast<u32>(ny) * wi + static_cast<u32>(nx);
            const u16 tb = sec.rd_i(ni);
            if (tb == GLS_IDX_NONE || tb > sec_n || tb == ta) {
                continue;
            }
            if (ta > tb) {
                continue;
            }
            const u16 a = static_cast<u16>(ta - 1u);
            const u16 b = static_cast<u16>(tb - 1u);
            const u32 pi = static_cast<u32>(a) * static_cast<u32>(sec_n) + static_cast<u32>(b);
            if (seen[pi] == 0u) {
                seen[pi] = 1u;
                ++link_n;
            }
            const u32 mx = (px + static_cast<u32>(nx)) / 2u;
            const u32 my = (py + static_cast<u32>(ny)) / 2u;
            sum_x[pi] += mx;
            sum_y[pi] += my;
            ++touch_n[pi];
        }
    }
    if (link_n == 0u) {
        delete[] seen;
        delete[] sum_x;
        delete[] sum_y;
        delete[] touch_n;
        return out->take(nullptr, 0u);
    }
    LandSectorLink* links = new LandSectorLink[link_n];
    u16 wr = 0u;
    for (u16 a = 0; a < sec_n; ++a) {
        for (u16 b = static_cast<u16>(a + 1u); b < sec_n; ++b) {
            const u32 pi = static_cast<u32>(a) * static_cast<u32>(sec_n) + static_cast<u32>(b);
            if (seen[pi] == 0u || touch_n[pi] == 0u) {
                continue;
            }
            if (wr >= link_n) {
                delete[] links;
                delete[] seen;
                delete[] sum_x;
                delete[] sum_y;
                delete[] touch_n;
                return false;
            }
            links[wr].m_a = a;
            links[wr].m_b = b;
            links[wr].m_x = static_cast<u16>(sum_x[pi] / touch_n[pi]);
            links[wr].m_y = static_cast<u16>(sum_y[pi] / touch_n[pi]);
            links[wr].m_len = static_cast<u16>(touch_n[pi] > 0xffffu ? 0xffffu : touch_n[pi]);
            ++wr;
        }
    }
    delete[] seen;
    delete[] sum_x;
    delete[] sum_y;
    delete[] touch_n;
    return out->take(links, wr);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
