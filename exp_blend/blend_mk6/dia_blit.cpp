//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "dia_blit.h"

#include <cmath>
#include <cstring>
#include <vector>

//================================================================================================================================
//=> - Class: DiaBlit -
//================================================================================================================================

DiaBlit::DiaBlit () : m_tex(nullptr), m_banks(nullptr), m_n(0) {}
DiaBlit::~DiaBlit () {}

void DiaBlit::bind (const DiaTex *tex, int n) {
    m_tex = tex;
    m_banks = nullptr;
    m_n = n;
}

void DiaBlit::bind_banks (const DiaDat15 *const *banks, int n) {
    m_banks = banks;
    m_tex = nullptr;
    m_n = n;
}

bool DiaBlit::in_dia (int cx, int cy, int x, int y) const {
    return std::abs (x - cx) * TILE_HALF_H + std::abs (y - cy) * TILE_HALF_W <= TILE_HALF_W * TILE_HALF_H;
}

bool DiaBlit::owns (const BlendMap &map, const DiaGrid &grid, int r, int c, int x, int y) const {
    Vx ctr = grid.center (r, c);
    int dx = x - ctr.x, dy = y - ctr.y;
    int s = std::abs (dx) * TILE_HALF_H + std::abs (dy) * TILE_HALF_W;
    int lim = TILE_HALF_W * TILE_HALF_H;
    if (s > lim) return false;
    if (s < lim) return true;
    int id = map.get (r, c);
    if (dx >= 0 && dy <= 0 && c + 1 < map.cols () && map.get (r, c + 1) != id) return false;
    if (dx >= 0 && dy >= 0 && r + 1 < map.rows () && map.get (r + 1, c) != id) return false;
    if (dx <= 0 && dy <= 0 && r > 0 && map.get (r - 1, c) != id) return false;
    if (dx <= 0 && dy >= 0 && c > 0 && map.get (r, c - 1) != id) return false;
    return dx >= 0;
}

void DiaBlit::home (Vx ctr, int x, int y, int &si, int &sj) const {
    int n = SUBTILE_DIV;
    float ax = (float)TILE_HALF_W / (float)n;
    float ay = (float)TILE_HALF_H / (float)n;
    float dx = (float)(x - ctr.x);
    float dy = (float)(y - (ctr.y - TILE_HALF_H));
    float fsi = 0.5f * (dx / ax + dy / ay);
    float fsj = 0.5f * (dy / ay - dx / ax);
    si = (int)std::floor (fsi);
    sj = (int)std::floor (fsj);
    if (si < 0) si = 0;
    if (sj < 0) sj = 0;
    if (si >= n) si = n - 1;
    if (sj >= n) sj = n - 1;
}

Rgb DiaBlit::sample (int id, int tr, int tc, int lx, int ly) const {
    if (id < 0 || id >= m_n) return Rgb (255, 0, 255);
    if (m_banks) {
        const DiaDat15 *b = m_banks[id];
        if (!b) return Rgb (255, 0, 255);
        int srow = tr - tc;
        int scol = (tr + tc - (srow & 1)) / 2;
        return b->sample (scol, srow, lx, ly);
    }
    if (!m_tex) return Rgb (255, 0, 255);
    const DiaTex &t = m_tex[id];
    if (!t.px || t.w <= 0 || t.h <= 0) return Rgb (255, 0, 255);
    if (lx < 0) lx = 0;
    if (ly < 0) ly = 0;
    if (lx >= t.w) lx = t.w - 1;
    if (ly >= t.h) ly = t.h - 1;
    int i = (ly * t.w + lx) * 3;
    return Rgb (t.px[i], t.px[i + 1], t.px[i + 2]);
}

void DiaBlit::extend_down (uint8_t *dst, int dw, int dh) const {
    if (dh < 2) return;
    std::vector<uint8_t> src (dst, dst + (size_t)dw * dh * 3);
    for (int y = 0; y < dh - 1; y++) {
        for (int x = 0; x < dw; x++) {
            int i = (y * dw + x) * 3;
            int j = i + dw * 3;
            if (src[i] == 0 && src[i + 1] == 0 && src[i + 2] == 0) continue;
            if (dst[j] != 0 || dst[j + 1] != 0 || dst[j + 2] != 0) continue;
            dst[j] = src[i];
            dst[j + 1] = src[i + 1];
            dst[j + 2] = src[i + 2];
        }
    }
}

void DiaBlit::run (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg, const DiaGrid &grid) const {
    std::memset (dst, 0, (size_t)dw * dh * 3);
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            Vx ctr = grid.center (r, c);
            int x0 = ctr.x - TILE_HALF_W, x1 = ctr.x + TILE_HALF_W;
            int y0 = ctr.y - TILE_HALF_H, y1 = ctr.y + TILE_HALF_H;
            if (x0 < 0) x0 = 0;
            if (y0 < 0) y0 = 0;
            if (x1 >= dw) x1 = dw - 1;
            if (y1 >= dh) y1 = dh - 1;
            for (int y = y0; y <= y1; y++) {
                for (int x = x0; x <= x1; x++) {
                    if (!owns (map, grid, r, c, x, y)) continue;
                    int si, sj;
                    home (ctr, x, y, si, sj);
                    int li = SUBTILE_DIV - 1 - sj;
                    int lj = si;
                    int sr = r * SUBTILE_DIV + li;
                    int sc = c * SUBTILE_DIV + lj;
                    int id = sg.get (sr, sc);
                    if (id < 0 || id >= m_n) id = 0;
                    int lx = x - (ctr.x - TILE_HALF_W);
                    int ly = y - (ctr.y - TILE_HALF_H);
                    Rgb col = sample (id, r, c, lx, ly);
                    int i = (y * dw + x) * 3;
                    dst[i] = col.r;
                    dst[i + 1] = col.g;
                    dst[i + 2] = col.b;
                }
            }
        }
    }
    extend_down (dst, dw, dh);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
