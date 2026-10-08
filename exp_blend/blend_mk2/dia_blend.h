//================================================================================================================================
// Junctions-first diamond edge threading. Shared NE/SE edges walk between pulled 4-way corners; seam compose uses side-of-path.
//================================================================================================================================

#ifndef DIA_BLEND_H
#define DIA_BLEND_H

#include <vector>
#include <cstdint>

#include "dia_cfg.h"
#include "dia_grid.h"

//================================================================================================================================
//=> - Edge path record -
//================================================================================================================================

struct EdgePath {
    EdgeKind kind;                                                               // NE or SE shared edge
    int r;                                                                       // anchor tile row
    int c;                                                                       // anchor tile col
    int ta;                                                                      // terrain on left-of-directed / SW
    int tb;                                                                      // terrain on right-of-directed / NE
    Vx a;                                                                        // true diamond-edge start
    Vx b;                                                                        // true diamond-edge end
    std::vector<Vx> pts;                                                         // jagged polyline (junctions + drunk)
};

//================================================================================================================================
//=> - Class: BlendMap -
//================================================================================================================================

class BlendMap {
public:
    BlendMap ();
    ~BlendMap ();

    bool from_win (const uint8_t *full, int full_w, int full_h, int r0, int c0, int win);
    int  rows () const;
    int  cols () const;
    int  get (int r, int c) const;

private:
    int m_rows;                                                                  // tile rows
    int m_cols;                                                                  // tile cols
    std::vector<int> m_ter;                                                      // row-major terrain ids
};

//================================================================================================================================
//=> - Class: EdgeThread -
//================================================================================================================================

class EdgeThread {
public:
    EdgeThread ();
    ~EdgeThread ();

    void run (const BlendMap &map, const DiaGrid &grid);
    int  n_path () const;
    const EdgePath& path (int i) const;

private:
    uint32_t seed (int a, int b, int c, int d) const;
    int  pick (uint32_t &s, int n) const;
    void calc_juncs (const BlendMap &map, const DiaGrid &grid);
    void drunk (std::vector<int> &off, uint32_t s) const;
    void walk_ne (const BlendMap &map, const DiaGrid &grid, int r, int c);
    void walk_se (const BlendMap &map, const DiaGrid &grid, int r, int c);
    Vx   junc (int jr, int jc) const;

    int m_rows;                                                                  // tile rows
    int m_cols;                                                                  // tile cols
    int m_jrows;                                                                 // junction table rows
    int m_jcols;                                                                 // junction table cols
    std::vector<Vx> m_junc;                                                      // right-corner junctions, indexed (jr+1,jc+1)
    std::vector<EdgePath> m_paths;                                               // jagged border polylines
};

#endif // DIA_BLEND_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
