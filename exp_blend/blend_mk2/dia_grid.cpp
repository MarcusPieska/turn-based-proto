//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "dia_grid.h"

#include <algorithm>

//================================================================================================================================
//=> - Class: DiaGrid -
//================================================================================================================================

DiaGrid::DiaGrid () :
    m_rows(0), m_cols(0), m_margin(0), m_img_w(0), m_img_h(0), m_x0(0), m_y0(0) {}

DiaGrid::~DiaGrid () {}

void DiaGrid::build (int rows, int cols, int margin) {
    m_rows = rows;
    m_cols = cols;
    m_margin = margin;
    m_x0 = margin + TILE_HALF_W;
    m_y0 = margin + TILE_HALF_H;
    int max_x = 0, max_y = 0, min_x = 0, min_y = 0;
    bool first = true;
    for (int r = 0; r < m_rows; r++) {
        for (int c = 0; c < m_cols; c++) {
            Vx d[4];
            diamond (r, c, d);
            for (int i = 0; i < 4; i++) {
                if (first || d[i].x < min_x) min_x = d[i].x;
                if (first || d[i].y < min_y) min_y = d[i].y;
                if (first || d[i].x > max_x) max_x = d[i].x;
                if (first || d[i].y > max_y) max_y = d[i].y;
                first = false;
            }
        }
    }
    int pad = margin;
    m_img_w = (max_x - min_x + 1) + pad * 2;
    m_img_h = (max_y - min_y + 1) + pad * 2;
    int dx = pad - min_x;
    int dy = pad - min_y;
    m_x0 += dx;
    m_y0 += dy;
}

int DiaGrid::rows () const { return m_rows; }
int DiaGrid::cols () const { return m_cols; }
int DiaGrid::img_w () const { return m_img_w; }
int DiaGrid::img_h () const { return m_img_h; }
int DiaGrid::n_sub () const { return SUBTILE_DIV + 1; }

Vx DiaGrid::center (int r, int c) const {
    int x = m_x0 + (c + r) * TILE_HALF_W;
    int y = m_y0 + (r - c) * TILE_HALF_H;
    return Vx (x, y);
}

void DiaGrid::diamond (int r, int c, Vx out[4]) const {
    Vx ctr = center (r, c);
    int hw = TILE_HALF_W;
    int hh = TILE_HALF_H;
    out[0] = Vx (ctr.x, ctr.y - hh);
    out[1] = Vx (ctr.x + hw, ctr.y);
    out[2] = Vx (ctr.x, ctr.y + hh);
    out[3] = Vx (ctr.x - hw, ctr.y);
}

void DiaGrid::sub_vx (int r, int c, std::vector<Vx> &out) const {
    Vx ctr = center (r, c);
    int n = SUBTILE_DIV;
    out.resize ((size_t)(n + 1) * (n + 1));
    for (int sj = 0; sj <= n; sj++) {
        for (int si = 0; si <= n; si++) {
            int px = ctr.x + (si - sj) * TILE_HALF_W / n;
            int py = ctr.y + (si + sj) * TILE_HALF_H / n - TILE_HALF_H;
            out[sj * (n + 1) + si] = Vx (px, py);
        }
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
