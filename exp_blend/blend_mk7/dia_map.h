//================================================================================================================================
// Terrain tile map plus a uniform 10x10 subtile expansion (logical square indices; diamond draw elsewhere).
//================================================================================================================================

#ifndef DIA_MAP_H
#define DIA_MAP_H

#include <vector>
#include <string>
#include <cstdint>

#include "dia_cfg.h"

//================================================================================================================================
//=> - Corner tip bits (pattern-derived; N E S W) -
//================================================================================================================================

enum CornBit {
    CORN_N = 1,
    CORN_E = 2,
    CORN_S = 4,
    CORN_W = 8
};

//================================================================================================================================
//=> - Class: BlendMap -
//================================================================================================================================

class BlendMap {
public:
    BlendMap ();
    ~BlendMap ();

    bool ld (const std::string &path);
    bool ld_ids (int rows, int cols, const int *ids);
    int rows () const;
    int cols () const;
    int get (int r, int c) const;

private:
    int m_rows; // tile rows
    int m_cols; // tile cols
    std::vector<int> m_ter; // row-major terrain ids
};

//================================================================================================================================
//=> - Class: SubGrid -
//================================================================================================================================

class SubGrid {
public:
    SubGrid ();
    ~SubGrid ();

    void build (const BlendMap &map);
    void copy_tiles (const SubGrid &src, int tr0, int tc0, int rows, int cols);
    int srows () const;
    int scols () const;
    int get (int sr, int sc) const;
    void set (int sr, int sc, int ter);
    void set (int sr, int sc, int ter, bool beach);
    bool beach (int sr, int sc) const;
    uint8_t corn (int sr, int sc) const;
    void set_corn (int sr, int sc, uint8_t bits);
    int oth (int sr, int sc, uint8_t tip) const;
    void set_oth (int sr, int sc, int on, int oe, int os, int ow);
    int tile_r (int sr) const;
    int tile_c (int sc) const;

private:
    int m_srows; // subtile rows
    int m_scols; // subtile cols
    std::vector<int> m_ter; // row-major subtile terrain
    std::vector<uint8_t> m_beach; // pattern beach label (4-7)
    std::vector<uint8_t> m_corn; // tip corner bits from patterns
    std::vector<int> m_oth; // other terrain per tip (N E S W), -1 = none
};

#endif // DIA_MAP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
