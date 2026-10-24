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

int SubGrid::srows () const { return m_srows; }
int SubGrid::scols () const { return m_scols; }
int SubGrid::get (int sr, int sc) const { return m_ter[sr * m_scols + sc]; }
void SubGrid::set (int sr, int sc, int ter) { m_ter[sr * m_scols + sc] = ter; }
int SubGrid::tile_r (int sr) const { return sr / SUBTILE_DIV; }
int SubGrid::tile_c (int sc) const { return sc / SUBTILE_DIV; }

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
