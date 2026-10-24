//================================================================================================================================
// DiaGrid places a lattice of isometric diamonds (100x50). Layout #2: row step +half_w/-half_h; col step +half_w/+half_h.
//================================================================================================================================

#ifndef DIA_GRID_H
#define DIA_GRID_H

#include "dia_cfg.h"

//================================================================================================================================
//=> - Class: DiaGrid -
//================================================================================================================================

class DiaGrid {
public:
    DiaGrid ();
    ~DiaGrid ();

    void build (int rows, int cols, int margin);
    int rows () const;
    int cols () const;
    int img_w () const;
    int img_h () const;
    Vx center (int r, int c) const;
    void diamond (int r, int c, Vx out[4]) const;

private:
    int m_rows; // tile rows
    int m_cols; // tile cols
    int m_margin; // canvas margin px
    int m_img_w; // canvas width
    int m_img_h; // canvas height
    int m_x0; // origin x for first center
    int m_y0; // origin y for first center
};

#endif // DIA_GRID_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
