//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "dia_grid.h"

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
    m_x0 += pad - min_x;
    m_y0 += pad - min_y;
}

int DiaGrid::rows () const { return m_rows; }
int DiaGrid::cols () const { return m_cols; }
int DiaGrid::img_w () const { return m_img_w; }
int DiaGrid::img_h () const { return m_img_h; }

Vx DiaGrid::center (int r, int c) const {
    int x = m_x0 + (c + r) * TILE_HALF_W;
    int y = m_y0 + (r - c) * TILE_HALF_H;
    return Vx (x, y);
}

void DiaGrid::diamond (int r, int c, Vx out[4]) const {
    Vx ctr = center (r, c);
    out[0] = Vx (ctr.x, ctr.y - TILE_HALF_H);
    out[1] = Vx (ctr.x + TILE_HALF_W, ctr.y);
    out[2] = Vx (ctr.x, ctr.y + TILE_HALF_H);
    out[3] = Vx (ctr.x - TILE_HALF_W, ctr.y);
}

Vx DiaGrid::sub_vx (int r, int c, int si, int sj) const {
    Vx ctr = center (r, c);
    int n = SUBTILE_DIV;
    int px = ctr.x + (si - sj) * TILE_HALF_W / n;
    int py = ctr.y + (si + sj) * TILE_HALF_H / n - TILE_HALF_H;
    return Vx (px, py);
}

void DiaGrid::sub_dia (int r, int c, int si, int sj, Vx out[4]) const {
    out[0] = sub_vx (r, c, si, sj);
    out[1] = sub_vx (r, c, si + 1, sj);
    out[2] = sub_vx (r, c, si + 1, sj + 1);
    out[3] = sub_vx (r, c, si, sj + 1);
}

Vx DiaGrid::sub_ctr (int r, int c, int si, int sj) const {
    Vx ctr = center (r, c);
    int n = SUBTILE_DIV;
    int px = ctr.x + (si - sj) * TILE_HALF_W / n;
    int py = ctr.y + (si + sj + 1) * TILE_HALF_H / n - TILE_HALF_H;
    return Vx (px, py);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
