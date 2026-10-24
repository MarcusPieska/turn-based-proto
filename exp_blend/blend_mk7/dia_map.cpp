//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "dia_map.h"

#include <cstdio>

//================================================================================================================================
//=> - Class: BlendMap -
//================================================================================================================================

BlendMap::BlendMap () : m_rows(0), m_cols(0) {}
BlendMap::~BlendMap () {}

bool BlendMap::ld (const std::string &path) {
    FILE *f = fopen (path.c_str (), "r");
    if (!f) return false;
    int rows = 0, cols = 0;
    if (fscanf (f, "%d %d", &rows, &cols) != 2 || rows <= 0 || cols <= 0) {
        fclose (f);
        return false;
    }
    std::vector<int> ter (rows * cols, 0);
    for (int i = 0; i < rows * cols; i++) {
        int v = 0;
        if (fscanf (f, "%d", &v) != 1 || v < 0 || v >= TERRAIN_ID_MAX) {
            fclose (f);
            return false;
        }
        ter[i] = v;
    }
    fclose (f);
    m_rows = rows;
    m_cols = cols;
    m_ter.swap (ter);
    return true;
}

bool BlendMap::ld_ids (int rows, int cols, const int *ids) {
    if (rows <= 0 || cols <= 0 || !ids) return false;
    std::vector<int> ter ((size_t)rows * cols, 0);
    for (int i = 0; i < rows * cols; i++) {
        int v = ids[i];
        if (v < 0 || v >= TERRAIN_ID_MAX) return false;
        ter[i] = v;
    }
    m_rows = rows;
    m_cols = cols;
    m_ter.swap (ter);
    return true;
}

int BlendMap::rows () const { return m_rows; }
int BlendMap::cols () const { return m_cols; }
int BlendMap::get (int r, int c) const { return m_ter[r * m_cols + c]; }

//================================================================================================================================
//=> - Class: SubGrid -
//================================================================================================================================

SubGrid::SubGrid () : m_srows(0), m_scols(0) {}
SubGrid::~SubGrid () {}

void SubGrid::build (const BlendMap &map) {
    m_srows = map.rows () * SUBTILE_DIV;
    m_scols = map.cols () * SUBTILE_DIV;
    m_ter.assign ((size_t)m_srows * m_scols, 0);
    m_beach.assign ((size_t)m_srows * m_scols, 0);
    m_corn.assign ((size_t)m_srows * m_scols, 0);
    m_oth.assign ((size_t)m_srows * m_scols * 4, -1);
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            int t = map.get (r, c);
            for (int sr = 0; sr < SUBTILE_DIV; sr++) {
                for (int sc = 0; sc < SUBTILE_DIV; sc++) {
                    int y = r * SUBTILE_DIV + sr;
                    int x = c * SUBTILE_DIV + sc;
                    m_ter[y * m_scols + x] = t;
                }
            }
        }
    }
}

void SubGrid::copy_tiles (const SubGrid &src, int tr0, int tc0, int rows, int cols) {
    m_srows = rows * SUBTILE_DIV;
    m_scols = cols * SUBTILE_DIV;
    m_ter.assign ((size_t)m_srows * m_scols, 0);
    m_beach.assign ((size_t)m_srows * m_scols, 0);
    m_corn.assign ((size_t)m_srows * m_scols, 0);
    m_oth.assign ((size_t)m_srows * m_scols * 4, -1);
    const int N = SUBTILE_DIV;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            for (int si = 0; si < N; si++) {
                for (int sj = 0; sj < N; sj++) {
                    int dsr = r * N + si, dsc = c * N + sj;
                    int ssr = (tr0 + r) * N + si, ssc = (tc0 + c) * N + sj;
                    if (ssr < 0 || ssc < 0 || ssr >= src.m_srows || ssc >= src.m_scols) continue;
                    size_t di = (size_t)dsr * m_scols + dsc;
                    size_t si_ = (size_t)ssr * src.m_scols + ssc;
                    m_ter[di] = src.m_ter[si_];
                    m_beach[di] = src.m_beach[si_];
                    m_corn[di] = src.m_corn[si_];
                    size_t do_ = di * 4, so = si_ * 4;
                    m_oth[do_] = src.m_oth[so];
                    m_oth[do_ + 1] = src.m_oth[so + 1];
                    m_oth[do_ + 2] = src.m_oth[so + 2];
                    m_oth[do_ + 3] = src.m_oth[so + 3];
                }
            }
        }
    }
}

int SubGrid::srows () const { return m_srows; }
int SubGrid::scols () const { return m_scols; }
int SubGrid::get (int sr, int sc) const { return m_ter[sr * m_scols + sc]; }
void SubGrid::set (int sr, int sc, int ter) { set (sr, sc, ter, false); }
void SubGrid::set (int sr, int sc, int ter, bool beach) {
    size_t i = (size_t)sr * m_scols + sc;
    m_ter[i] = ter;
    m_beach[i] = beach ? 1 : 0;
}
bool SubGrid::beach (int sr, int sc) const { return m_beach[sr * m_scols + sc] != 0; }
uint8_t SubGrid::corn (int sr, int sc) const { return m_corn[sr * m_scols + sc]; }
void SubGrid::set_corn (int sr, int sc, uint8_t bits) { m_corn[sr * m_scols + sc] = bits; }

int SubGrid::oth (int sr, int sc, uint8_t tip) const {
    size_t i = ((size_t)sr * m_scols + sc) * 4;
    if (tip == CORN_N) return m_oth[i];
    if (tip == CORN_E) return m_oth[i + 1];
    if (tip == CORN_S) return m_oth[i + 2];
    if (tip == CORN_W) return m_oth[i + 3];
    return -1;
}

void SubGrid::set_oth (int sr, int sc, int on, int oe, int os, int ow) {
    size_t i = ((size_t)sr * m_scols + sc) * 4;
    m_oth[i] = on;
    m_oth[i + 1] = oe;
    m_oth[i + 2] = os;
    m_oth[i + 3] = ow;
}

int SubGrid::tile_r (int sr) const { return sr / SUBTILE_DIV; }
int SubGrid::tile_c (int sc) const { return sc / SUBTILE_DIV; }

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
