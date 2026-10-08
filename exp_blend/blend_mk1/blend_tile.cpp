//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "blend_tile.h"

#include <cstring>
#include <algorithm>
#include <cmath>

//================================================================================================================================
//=> - Class: BlendTile -
//================================================================================================================================

BlendTile::BlendTile () {}
BlendTile::~BlendTile () {}

float BlendTile::path_y (const std::vector<Vx> &pts, float x) const {
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

float BlendTile::path_x (const std::vector<Vx> &pts, float y) const {
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

void BlendTile::blit (uint8_t *dst, int dw, int dh, const BlendMap &map, const TileTex *tex, int n_tex, int fb) const {
    (void)dh;
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            int id = map.get (r, c);
            if (id < 0 || id >= n_tex) id = fb;
            const TileTex &t = tex[id];
            int x0 = c * TILE_PX, y0 = r * TILE_PX;
            for (int y = 0; y < TILE_PX; y++) {
                uint8_t *d = &dst[((y0 + y) * dw + x0) * 3];
                const uint8_t *s = &t.px[y * t.w * 3];
                std::memcpy (d, s, (size_t)TILE_PX * 3);
            }
        }
    }
}

void BlendTile::put (uint8_t *dst, int dw, int dh, const TileTex *tex, int n_tex, int id, int x, int y) const {
    if (id < 0 || id >= n_tex || x < 0 || y < 0 || x >= dw || y >= dh) return;
    const TileTex &t = tex[id];
    int lx = x % TILE_PX, ly = y % TILE_PX;
    int si = (ly * t.w + lx) * 3;
    int di = (y * dw + x) * 3;
    dst[di] = t.px[si];
    dst[di + 1] = t.px[si + 1];
    dst[di + 2] = t.px[si + 2];
}

void BlendTile::seam (uint8_t *dst, int dw, int dh, const EdgeThread &th, const TileTex *tex, int n_tex, bool opt) const {
    int pad = (EDGE_MAX_DEV + CORNER_PULL_SUB) * SUBTILE_PX;
    for (int p = 0; p < th.n_path (); p++) {
        const EdgePath &ep = th.path (p);
        if (ep.kind == EDGE_H) {
            int geo_y = (ep.r + 1) * TILE_PX;
            int x0 = ep.c * TILE_PX - pad, x1 = (ep.c + 1) * TILE_PX + pad;
            int y0 = geo_y - pad, y1 = geo_y + pad;
            if (x0 < 0) x0 = 0;
            if (y0 < 0) y0 = 0;
            if (x1 > dw) x1 = dw;
            if (y1 > dh) y1 = dh;
            m_lut.resize ((size_t)(x1 - x0));
            for (int x = x0; x < x1; x++) m_lut[x - x0] = path_y (ep.pts, (float)x + 0.5f);
            if (!opt) {
                for (int y = y0; y < y1; y++) {
                    float cy = (float)y + 0.5f;
                    for (int x = x0; x < x1; x++) {
                        float py = m_lut[x - x0];
                        int id = -1;
                        if (cy < py && cy >= (float)geo_y) id = ep.ta;
                        if (cy >= py && cy < (float)geo_y) id = ep.tb;
                        if (id >= 0) put (dst, dw, dh, tex, n_tex, id, x, y);
                    }
                }
            } else {
                for (int by = y0; by < y1; by += SUBTILE_PX) {
                    int by1 = std::min (by + SUBTILE_PX, y1);
                    for (int bx = x0; bx < x1; bx += SUBTILE_PX) {
                        int bx1 = std::min (bx + SUBTILE_PX, x1);
                        float min_py = 1e30f, max_py = -1e30f;
                        for (int x = bx; x < bx1; x++) {
                            float py = m_lut[x - x0];
                            if (py < min_py) min_py = py;
                            if (py > max_py) max_py = py;
                        }
                        bool skip = false;
                        if (by1 <= geo_y) {
                            if ((float)by1 <= min_py) skip = true;
                        } else if (by >= geo_y) {
                            if ((float)by >= max_py) skip = true;
                        }
                        if (skip) continue;
                        for (int y = by; y < by1; y++) {
                            float cy = (float)y + 0.5f;
                            for (int x = bx; x < bx1; x++) {
                                float py = m_lut[x - x0];
                                int id = -1;
                                if (cy < py && cy >= (float)geo_y) id = ep.ta;
                                if (cy >= py && cy < (float)geo_y) id = ep.tb;
                                if (id >= 0) put (dst, dw, dh, tex, n_tex, id, x, y);
                            }
                        }
                    }
                }
            }
        } else {
            int geo_x = (ep.c + 1) * TILE_PX;
            int y0 = ep.r * TILE_PX - pad, y1 = (ep.r + 1) * TILE_PX + pad;
            int x0 = geo_x - pad, x1 = geo_x + pad;
            if (x0 < 0) x0 = 0;
            if (y0 < 0) y0 = 0;
            if (x1 > dw) x1 = dw;
            if (y1 > dh) y1 = dh;
            m_lut.resize ((size_t)(y1 - y0));
            for (int y = y0; y < y1; y++) m_lut[y - y0] = path_x (ep.pts, (float)y + 0.5f);
            if (!opt) {
                for (int y = y0; y < y1; y++) {
                    float px = m_lut[y - y0];
                    for (int x = x0; x < x1; x++) {
                        float cx = (float)x + 0.5f;
                        int id = -1;
                        if (cx < px && cx >= (float)geo_x) id = ep.ta;
                        if (cx >= px && cx < (float)geo_x) id = ep.tb;
                        if (id >= 0) put (dst, dw, dh, tex, n_tex, id, x, y);
                    }
                }
            } else {
                for (int by = y0; by < y1; by += SUBTILE_PX) {
                    int by1 = std::min (by + SUBTILE_PX, y1);
                    for (int bx = x0; bx < x1; bx += SUBTILE_PX) {
                        int bx1 = std::min (bx + SUBTILE_PX, x1);
                        float min_px = 1e30f, max_px = -1e30f;
                        for (int y = by; y < by1; y++) {
                            float px = m_lut[y - y0];
                            if (px < min_px) min_px = px;
                            if (px > max_px) max_px = px;
                        }
                        bool skip = false;
                        if (bx1 <= geo_x) {
                            if ((float)bx1 <= min_px) skip = true;
                        } else if (bx >= geo_x) {
                            if ((float)bx >= max_px) skip = true;
                        }
                        if (skip) continue;
                        for (int y = by; y < by1; y++) {
                            float px = m_lut[y - y0];
                            for (int x = bx; x < bx1; x++) {
                                float cx = (float)x + 0.5f;
                                int id = -1;
                                if (cx < px && cx >= (float)geo_x) id = ep.ta;
                                if (cx >= px && cx < (float)geo_x) id = ep.tb;
                                if (id >= 0) put (dst, dw, dh, tex, n_tex, id, x, y);
                            }
                        }
                    }
                }
            }
        }
    }
}

void BlendTile::compose (uint8_t *dst, int dw, int dh, const BlendMap &map, const EdgeThread &th,
                         const TileTex *tex, int n_tex, int fb, bool opt) {
    blit (dst, dw, dh, map, tex, n_tex, fb);
    seam (dst, dw, dh, th, tex, n_tex, opt);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
