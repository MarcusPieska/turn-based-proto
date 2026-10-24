//================================================================================================================================
// Terrain tile map plus a uniform 10x10 subtile expansion (parent terrain copied into each cell).
//================================================================================================================================

#ifndef SUB_MAP_H
#define SUB_MAP_H

#include <vector>
#include <string>

#include "sub_cfg.h"

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
    int px_w () const;
    int px_h () const;

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
    Vx origin (int sr, int sc) const;
    int px_w () const;
    int px_h () const;

private:
    int m_srows; // subtile rows
    int m_scols; // subtile cols
    std::vector<int> m_ter; // row-major subtile terrain
};

#endif // SUB_MAP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
