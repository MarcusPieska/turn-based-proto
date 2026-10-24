//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "dia_edge.h"

#include <vector>
#include <cstring>

#ifndef EDGE_RIM_THICK
#define EDGE_RIM_THICK 3
#endif
#ifndef EDGE_WT_DELTA
#define EDGE_WT_DELTA 15
#endif
#ifndef EDGE_AA_FACT
#define EDGE_AA_FACT 5
#endif

//================================================================================================================================
//=> - Class: DiaEdge -
//================================================================================================================================

DiaEdge::DiaEdge () {}
DiaEdge::~DiaEdge () {}

bool DiaEdge::ld (const std::string &bin_path) { return m_rim.ld (bin_path); }
bool DiaEdge::ld_splat (const std::string &root) { return m_rim.ld_splat (root); }
void DiaEdge::set_tips (uint8_t allow) { m_rim.set_tips (allow); }
void DiaEdge::prep () { m_rim.prep (); }

void DiaEdge::apply (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg,
                     const DiaGrid &grid) const {
    std::vector<int> ids;
    m_rim.paint_ids (ids, dw, dh, map, sg, grid);
    std::vector<uint8_t> src ((size_t)dw * dh * 3);
    std::memcpy (src.data (), dst, src.size ());

    auto id_at = [&](int x, int y) -> int {
        if (x < 0 || y < 0 || x >= dw || y >= dh) return -1;
        return ids[(size_t)y * dw + x];
    };
    auto rgb_at = [&](const std::vector<uint8_t> &buf, int x, int y, uint8_t *r, uint8_t *g, uint8_t *b) {
        size_t i = ((size_t)y * dw + x) * 3;
        *r = buf[i]; *g = buf[i + 1]; *b = buf[i + 2];
    };

    static const int dx8[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
    static const int dy8[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
    const int thick = EDGE_RIM_THICK > 0 ? EDGE_RIM_THICK : 1;

    for (int y = 0; y < dh; y++) {
        for (int x = 0; x < dw; x++) {
            int id = id_at (x, y);
            if (id < 0) continue;
            int ox = -1, oy = -1, oid = -1, best = thick + 1;
            for (int dy = -thick; dy <= thick; dy++) {
                for (int dx = -thick; dx <= thick; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = x + dx, ny = y + dy;
                    int nid = id_at (nx, ny);
                    if (nid < 0 || nid == id) continue;
                    int dist = dx < 0 ? -dx : dx;
                    int ady = dy < 0 ? -dy : dy;
                    if (ady > dist) dist = ady;
                    if (dist < best) { best = dist; ox = nx; oy = ny; oid = nid; }
                }
            }
            if (oid < 0) continue;
            int C = 0;
            for (int k = 0; k < 8; k++) {
                int nid = id_at (x + dx8[k], y + dy8[k]);
                if (nid >= 0 && nid != id) C++;
            }
            int d = EDGE_AA_FACT * (C - 3);
            if (d < 0) d = 0;
            int wt = 100 - (thick - best + 1) * EDGE_WT_DELTA - d;
            if (wt < 0) wt = 0;
            if (wt > 100) wt = 100;
            int wo = 100 - wt;
            uint8_t tr, tg, tb, or_, og, ob;
            rgb_at (src, x, y, &tr, &tg, &tb);
            rgb_at (src, ox, oy, &or_, &og, &ob);
            size_t i = ((size_t)y * dw + x) * 3;
            dst[i] = (uint8_t)((tr * wt + or_ * wo) / 100);
            dst[i + 1] = (uint8_t)((tg * wt + og * wo) / 100);
            dst[i + 2] = (uint8_t)((tb * wt + ob * wo) / 100);
        }
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
