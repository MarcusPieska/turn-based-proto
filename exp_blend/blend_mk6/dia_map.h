//================================================================================================================================
// Terrain tile map plus a uniform 10x10 subtile expansion (logical square indices; diamond draw elsewhere).
//================================================================================================================================

#ifndef DIA_MAP_H
#define DIA_MAP_H

#include <vector>
#include <string>

#include "dia_cfg.h"

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
    int srows () const;
    int scols () const;
    int get (int sr, int sc) const;
    void set (int sr, int sc, int ter);
    int tile_r (int sr) const;
    int tile_c (int sc) const;

private:
    int m_srows; // subtile rows
    int m_scols; // subtile cols
    std::vector<int> m_ter; // row-major subtile terrain
};

#endif // DIA_MAP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
