//================================================================================================================================
// Terrain id map and junctions-first border threading. Produces edge polylines only; no textures or blit.
//================================================================================================================================

#ifndef BLEND_THREAD_H
#define BLEND_THREAD_H

#include <vector>
#include <string>

#include "blend_cfg.h"

//================================================================================================================================
//=> - Edge path record -
//================================================================================================================================

struct EdgePath {
    EdgeKind kind;                                                               // horizontal or vertical shared edge
    int r;                                                                       // top/left tile row
    int c;                                                                       // top/left tile col
    int ta;                                                                      // terrain on top or left
    int tb;                                                                      // terrain on bottom or right
    std::vector<Vx> pts;                                                         // polyline in pixel space
};

//================================================================================================================================
//=> - Class: BlendMap -
//================================================================================================================================

class BlendMap {
public:
    BlendMap ();
    ~BlendMap ();

    bool ld (const std::string &path);
    bool from_win (const uint8_t *full, int full_w, int full_h, int r0, int c0, int win);
    int  rows () const;
    int  cols () const;
    int  get (int r, int c) const;
    int  px_w () const;
    int  px_h () const;

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

    void run (const BlendMap &map);
    int  n_path () const;
    const EdgePath& path (int i) const;
    Vx   junc (int vr, int vc) const;

private:
    uint32_t seed (int a, int b, int c, int d) const;
    int  pick (uint32_t &s, int n) const;
    void calc_juncs (const BlendMap &map);
    void walk_h (const BlendMap &map, int r, int c);
    void walk_v (const BlendMap &map, int r, int c);
    void drunk (std::vector<int> &off, uint32_t s) const;

    int m_rows;                                                                  // tile rows
    int m_cols;                                                                  // tile cols
    std::vector<Vx> m_junc;                                                      // junction px for each tile vertex
    std::vector<EdgePath> m_paths;                                               // jagged border polylines
};

#endif // BLEND_THREAD_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
