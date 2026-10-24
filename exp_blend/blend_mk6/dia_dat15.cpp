//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "dia_dat15.h"

#include <cstdio>
#include <cstring>

struct Dat15Top {
    uint16_t num_rows;
    uint16_t num_cols;
};

struct Dat15Hdr {
    uint32_t bits; // matches game_view_mk1 sizeof(dat15_hdr)==4
    bool has_next () const { return (bits & 1u) != 0; }
    int size () const { return (int)((bits >> 1) & 0x7fffu); }
};

//================================================================================================================================
//=> - Class: DiaDat15 -
//================================================================================================================================

DiaDat15::DiaDat15 () : m_rows(0), m_cols(0), m_tw(0), m_th(0) {}
DiaDat15::~DiaDat15 () {}

bool DiaDat15::ld (const std::string &path, int tw, int th) {
    m_rgb.clear ();
    m_rows = m_cols = 0;
    m_tw = tw;
    m_th = th;
    FILE *f = fopen (path.c_str (), "rb");
    if (!f) return false;
    Dat15Top top;
    if (fread (&top, sizeof (top), 1, f) != 1) { fclose (f); return false; }
    m_rows = (int)top.num_rows;
    m_cols = (int)top.num_cols;
    int need = tw * th * 3;
    while (1) {
        Dat15Hdr hdr;
        if (fread (&hdr.bits, sizeof (hdr.bits), 1, f) != 1) break;
        int sz = hdr.size ();
        if (sz <= 0) break;
        std::vector<uint8_t> rgb ((size_t)sz);
        if (fread (rgb.data (), 1, (size_t)sz, f) != (size_t)sz) { fclose (f); return false; }
        if (sz != need) { fclose (f); return false; }
        m_rgb.push_back (std::move (rgb));
        if (!hdr.has_next ()) break;
    }
    fclose (f);
    return (int)m_rgb.size () == m_rows * m_cols && m_rows > 0 && m_cols > 0;
}

int DiaDat15::n_rows () const { return m_rows; }
int DiaDat15::n_cols () const { return m_cols; }
int DiaDat15::tw () const { return m_tw; }
int DiaDat15::th () const { return m_th; }

const uint8_t *DiaDat15::item (int row, int col) const {
    if (m_rows <= 0 || m_cols <= 0 || m_rgb.empty ()) return nullptr;
    row %= m_rows;
    if (row < 0) row += m_rows;
    col %= m_cols;
    if (col < 0) col += m_cols;
    return m_rgb[(size_t)row * m_cols + col].data ();
}

Rgb DiaDat15::sample (int row, int col, int lx, int ly) const {
    const uint8_t *px = item (row, col);
    if (!px || m_tw <= 0 || m_th <= 0) return Rgb (255, 0, 255);
    if (lx < 0) lx = 0;
    if (ly < 0) ly = 0;
    if (lx >= m_tw) lx = m_tw - 1;
    if (ly >= m_th) ly = m_th - 1;
    int i = (ly * m_tw + lx) * 3;
    uint8_t r = px[i], g = px[i + 1], b = px[i + 2];
    if (r == 255 && g == 0 && b == 255) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int x2 = lx + dx, y2 = ly + dy;
                if (x2 < 0 || y2 < 0 || x2 >= m_tw || y2 >= m_th) continue;
                int j = (y2 * m_tw + x2) * 3;
                if (!(px[j] == 255 && px[j + 1] == 0 && px[j + 2] == 255))
                    return Rgb (px[j], px[j + 1], px[j + 2]);
            }
        }
        return Rgb (0, 0, 0);
    }
    return Rgb (r, g, b);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
