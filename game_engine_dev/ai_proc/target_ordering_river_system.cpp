//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "target_ordering_river_system.h"

#include <cstring>

#include "build_adds_array.h"
#include "city.h"
#include "city_array.h"
#include "game_array_simple.h"
#include "game_state.h"
#include "gen_watershed.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static const i8 k_dx8[9] = {0, -1, 0, 1, -1, 1, -1, 0, 1};
static const i8 k_dy8[9] = {0, -1, -1, -1, 0, 0, 1, 1, 1};

//================================================================================================================================
//=> - TargetOrderingRiverSystem -
//================================================================================================================================

TargetOrderingRiverSystem::TargetOrderingRiverSystem () {
}

u16 TargetOrderingRiverSystem::fill (GameState& st, u16 sx, u16 sy, u8 enemy, u16* out, u16 cap) {
    if (out == nullptr || cap == 0u) {
        return 0;
    }
    const u16 w = st.m_map.width();
    const u16 h = st.m_map.height();
    if (sx >= w || sy >= h) {
        return 0;
    }
    if (WhiteboardMng::width() != w || WhiteboardMng::height() != h) {
        return 0;
    }
    GenWatershed ws;
    if (!ws.begin(st.m_map) || ws.fill_rivers(sx, sy) == 0u) {
        return 0;
    }
    const Whiteboard_1B& riv = ws.overlay();
    Whiteboard_1B taken("TargetOrderingRiverSystem", "taken", 0u);
    if (!taken.ok()) {
        return 0;
    }
    std::memset(taken.raw(), 0, static_cast<size_t>(WhiteboardMng::tile_n()));
    const u16 cn = st.m_cities.get_city_count();
    u16 n = 0;
    for (u16 y = 0; y < h && n < cap; ++y) {
        for (u16 x = 0; x < w && n < cap; ++x) {
            if (riv.rd(x, y) == 0u) {
                continue;
            }
            for (u8 d = 0; d < 9u && n < cap; ++d) {
                const i32 nx = static_cast<i32>(x) + static_cast<i32>(k_dx8[d]);
                const i32 ny = static_cast<i32>(y) + static_cast<i32>(k_dy8[d]);
                if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                    continue;
                }
                const u16 ux = static_cast<u16>(nx);
                const u16 uy = static_cast<u16>(ny);
                if (taken.rd(ux, uy) != 0u) {
                    continue;
                }
                if (st.m_map.get_add_typ(ux, uy) != BUILD_ADD_CITY) {
                    continue;
                }
                const u16 cidx = st.m_map.get_add_idx(ux, uy);
                if (cidx >= cn) {
                    continue;
                }
                City* c = st.m_cities.get_city(cidx);
                if (c == nullptr || c->get_owner() != enemy) {
                    continue;
                }
                taken.wr(ux, uy, 1u);
                out[n] = cidx;
                n = static_cast<u16>(n + 1u);
            }
        }
    }
    return n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
