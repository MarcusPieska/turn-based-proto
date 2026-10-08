//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "blend_core.h"

#include <cmath>
#include <algorithm>
#include <queue>

//================================================================================================================================
//=> - Class: SubtileMesh -
//================================================================================================================================

SubtileMesh::SubtileMesh () : m_rows(0), m_cols(0), m_nv(SUBTILE_DIV + 1) {}
SubtileMesh::~SubtileMesh () {}

void SubtileMesh::build (const BlendMap &map) {
    m_rows = map.rows ();
    m_cols = map.cols ();
    m_nv = SUBTILE_DIV + 1;
    m_vx.assign (m_rows * m_cols * m_nv * m_nv, Vx ());
    for (int r = 0; r < m_rows; r++) {
        for (int c = 0; c < m_cols; c++) {
            Vx *dst = &m_vx[(r * m_cols + c) * m_nv * m_nv];
            int ox = c * TILE_PX;
            int oy = r * TILE_PX;
            for (int j = 0; j < m_nv; j++) {
                for (int i = 0; i < m_nv; i++) {
                    dst[j * m_nv + i] = Vx (ox + i * SUBTILE_PX, oy + j * SUBTILE_PX);
                }
            }
        }
    }
}

int SubtileMesh::n_tile () const { return m_rows * m_cols; }
int SubtileMesh::n_vx () const { return m_nv; }
const Vx* SubtileMesh::tile_vx (int r, int c) const {
    return &m_vx[(r * m_cols + c) * m_nv * m_nv];
}

//================================================================================================================================
//=> - Class: BlendClip -
//================================================================================================================================

BlendClip::BlendClip () : m_w(0), m_h(0) {}
BlendClip::~BlendClip () {}

float BlendClip::path_y (const std::vector<Vx> &pts, float x) const {
    if (pts.size () < 2) return 0.f;
    if (x <= (float)pts.front ().x) return (float)pts.front ().y;
    if (x >= (float)pts.back ().x) return (float)pts.back ().y;
    for (size_t i = 1; i < pts.size (); i++) {
        float x0 = (float)pts[i - 1].x, x1 = (float)pts[i].x;
        if (x > x1) continue;
        float t = (x1 > x0) ? (x - x0) / (x1 - x0) : 0.f;
        return (float)pts[i - 1].y + t * (float)(pts[i].y - pts[i - 1].y);
    }
    return (float)pts.back ().y;
}

float BlendClip::path_x (const std::vector<Vx> &pts, float y) const {
    if (pts.size () < 2) return 0.f;
    if (y <= (float)pts.front ().y) return (float)pts.front ().x;
    if (y >= (float)pts.back ().y) return (float)pts.back ().x;
    for (size_t i = 1; i < pts.size (); i++) {
        float y0 = (float)pts[i - 1].y, y1 = (float)pts[i].y;
        if (y > y1) continue;
        float t = (y1 > y0) ? (y - y0) / (y1 - y0) : 0.f;
        return (float)pts[i - 1].x + t * (float)(pts[i].x - pts[i - 1].x);
    }
    return (float)pts.back ().x;
}

void BlendClip::mark_seg (std::vector<uint8_t> &seam, Vx a, Vx b) const {
    int x0 = a.x, y0 = a.y, x1 = b.x, y1 = b.y;
    int dx = std::abs (x1 - x0), dy = std::abs (y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        if (x0 >= 0 && y0 >= 0 && x0 < m_w && y0 < m_h) seam[y0 * m_w + x0] = 1;
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void BlendClip::mark_seams (std::vector<uint8_t> &seam, const EdgeThread &th) const {
    for (int p = 0; p < th.n_path (); p++) {
        const std::vector<Vx> &pts = th.path (p).pts;
        for (size_t i = 1; i < pts.size (); i++) mark_seg (seam, pts[i - 1], pts[i]);
    }
}

void BlendClip::apply_h (const EdgePath &ep) {
    int pad = (EDGE_MAX_DEV + CORNER_PULL_SUB) * SUBTILE_PX;
    int geo_y = (ep.r + 1) * TILE_PX;
    int x0 = ep.c * TILE_PX - pad;
    int x1 = (ep.c + 1) * TILE_PX + pad;
    int y0 = geo_y - pad;
    int y1 = geo_y + pad;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > m_w) x1 = m_w;
    if (y1 > m_h) y1 = m_h;
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            float py = path_y (ep.pts, (float)x + 0.5f);
            float cy = (float)y + 0.5f;
            int &cell = m_ter[y * m_w + x];
            if (cy < py && cy >= (float)geo_y) cell = ep.ta;
            if (cy >= py && cy < (float)geo_y) cell = ep.tb;
        }
    }
}

void BlendClip::apply_v (const EdgePath &ep) {
    int pad = (EDGE_MAX_DEV + CORNER_PULL_SUB) * SUBTILE_PX;
    int geo_x = (ep.c + 1) * TILE_PX;
    int y0 = ep.r * TILE_PX - pad;
    int y1 = (ep.r + 1) * TILE_PX + pad;
    int x0 = geo_x - pad;
    int x1 = geo_x + pad;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > m_w) x1 = m_w;
    if (y1 > m_h) y1 = m_h;
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            float px = path_x (ep.pts, (float)y + 0.5f);
            float cx = (float)x + 0.5f;
            int &cell = m_ter[y * m_w + x];
            if (cx < px && cx >= (float)geo_x) cell = ep.ta;
            if (cx >= px && cx < (float)geo_x) cell = ep.tb;
        }
    }
}

void BlendClip::flood (const BlendMap &map, const EdgeThread &th) {
    std::vector<uint8_t> seam (m_w * m_h, 0);
    mark_seams (seam, th);
    m_ter.assign (m_w * m_h, -1);
    std::queue<int> q;
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            int cx = c * TILE_PX + TILE_PX / 2;
            int cy = r * TILE_PX + TILE_PX / 2;
            int idx = cy * m_w + cx;
            if (seam[idx]) {
                bool found = false;
                for (int rad = 1; rad < TILE_PX / 2 && !found; rad++) {
                    for (int dy = -rad; dy <= rad && !found; dy++) {
                        for (int dx = -rad; dx <= rad && !found; dx++) {
                            int x = cx + dx, y = cy + dy;
                            if (x < c * TILE_PX || y < r * TILE_PX) continue;
                            if (x >= (c + 1) * TILE_PX || y >= (r + 1) * TILE_PX) continue;
                            int j = y * m_w + x;
                            if (!seam[j]) { idx = j; found = true; }
                        }
                    }
                }
            }
            if (m_ter[idx] < 0) {
                m_ter[idx] = map.get (r, c);
                q.push (idx);
            }
        }
    }
    static const int dx4[4] = { 1, -1, 0, 0 };
    static const int dy4[4] = { 0, 0, 1, -1 };
    while (!q.empty ()) {
        int idx = q.front ();
        q.pop ();
        int x = idx % m_w, y = idx / m_w;
        int ter = m_ter[idx];
        for (int k = 0; k < 4; k++) {
            int nx = x + dx4[k], ny = y + dy4[k];
            if (nx < 0 || ny < 0 || nx >= m_w || ny >= m_h) continue;
            int ni = ny * m_w + nx;
            if (seam[ni] || m_ter[ni] >= 0) continue;
            m_ter[ni] = ter;
            q.push (ni);
        }
    }
    for (int i = 0; i < m_w * m_h; i++) {
        if (m_ter[i] >= 0) continue;
        int x = i % m_w, y = i / m_w;
        int best = 0;
        for (int k = 0; k < 4; k++) {
            int nx = x + dx4[k], ny = y + dy4[k];
            if (nx < 0 || ny < 0 || nx >= m_w || ny >= m_h) continue;
            int t = m_ter[ny * m_w + nx];
            if (t >= 0) { best = t; break; }
        }
        m_ter[i] = best;
    }
}

void BlendClip::run (const BlendMap &map, const EdgeThread &th, ClipStyle style) {
    m_w = map.px_w ();
    m_h = map.px_h ();
    if (style == CLIP_FLOOD) {
        flood (map, th);
        return;
    }
    m_ter.assign (m_w * m_h, 0);
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            int ter = map.get (r, c);
            int x0 = c * TILE_PX, y0 = r * TILE_PX;
            for (int y = 0; y < TILE_PX; y++) {
                for (int x = 0; x < TILE_PX; x++) m_ter[(y0 + y) * m_w + (x0 + x)] = ter;
            }
        }
    }
    for (int i = 0; i < th.n_path (); i++) {
        const EdgePath &ep = th.path (i);
        if (ep.kind == EDGE_H) apply_h (ep);
        else apply_v (ep);
    }
}

int BlendClip::get (int x, int y) const { return m_ter[y * m_w + x]; }
int BlendClip::w () const { return m_w; }
int BlendClip::h () const { return m_h; }

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
