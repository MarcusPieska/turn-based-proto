//================================================================================================================================
// DiaGrid places a GRID_N x GRID_N lattice of isometric diamonds (100x50, 101px draw span).
// Layout #2: each step along a row is +TILE_HALF_W x and -TILE_HALF_H y (rise 1/2, up-right);
// each step down a column is +TILE_HALF_W x and +TILE_HALF_H y. The grid forms a meta-diamond.
//================================================================================================================================

#ifndef DIA_GRID_H
#define DIA_GRID_H

#include <vector>

#include "dia_cfg.h"

//================================================================================================================================
//=> - Class: DiaGrid -
//================================================================================================================================

class DiaGrid {
public:
    DiaGrid ();
    ~DiaGrid ();

    void build (int rows, int cols, int margin);
    int  rows () const;
    int  cols () const;
    int  img_w () const;
    int  img_h () const;

    Vx   center (int r, int c) const;
    void diamond (int r, int c, Vx out[4]) const;
    void sub_vx (int r, int c, std::vector<Vx> &out) const;
    int  n_sub () const;

private:
    int m_rows;                                                                  // tile rows
    int m_cols;                                                                  // tile cols
    int m_margin;                                                                // canvas margin px
    int m_img_w;                                                                 // canvas width
    int m_img_h;                                                                 // canvas height
    int m_x0;                                                                    // origin x for first center
    int m_y0;                                                                    // origin y for first center
};

#endif // DIA_GRID_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
