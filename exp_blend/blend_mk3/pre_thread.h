//================================================================================================================================
// Pre-determined border threading (mk3): cataloged corners, dup splits, hashed templates morphed c2c.
//================================================================================================================================

#ifndef PRE_THREAD_H
#define PRE_THREAD_H

#include <vector>
#include <string>

#include "pre_cfg.h"

//================================================================================================================================
//=> - Edge path record -
//================================================================================================================================

struct EdgePath {
    EdgeKind kind; // H, V, or diagonal connector
    int r; // top/left tile row (or vertex row for D)
    int c; // top/left tile col (or vertex col for D)
    int ta; // terrain on top/left / diag a
    int tb; // terrain on bottom/right / diag b
    std::vector<Vx> pts; // polyline in pixel space
};

//================================================================================================================================
//=> - Class: BlendMap -
//================================================================================================================================

class BlendMap {
public:
    BlendMap ();
    ~BlendMap ();

    bool ld (const std::string &path);
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
//=> - Class: EdgeThread -
//================================================================================================================================

class EdgeThread {
public:
    EdgeThread ();
    ~EdgeThread ();

    bool ld_bank (const std::string &dir);
    int n_bank () const;
    void run (const BlendMap &map);
    void run_c2c (const BlendMap &map, int c2c);
    int n_path () const;
    const EdgePath& path (int i) const;
    Vx junc (int vr, int vc) const;
    Vx junc2 (int vr, int vc) const;
    bool dup (int vr, int vc) const;
    int junc_ix (int vr, int vc) const;
    int junc_iy (int vr, int vc) const;

private:
    uint32_t seed (int a, int b, int c, int d) const;
    int pick (uint32_t &s, int n) const;
    void calc_juncs (const BlendMap &map);
    Vx end (int vr, int vc, int t0r, int t0c, int t1r, int t1c) const;
    void walk_h (const BlendMap &map, int r, int c);
    void walk_v (const BlendMap &map, int r, int c);
    void walk_h_c2c (const BlendMap &map, int r, int c, int c2c);
    void walk_v_c2c (const BlendMap &map, int r, int c, int c2c);
    void link (const BlendMap &map, bool use_bank, int c2c);
    void line (Vx a, Vx b, int n_mid, std::vector<Vx> &pts) const;
    void morph (Vx a, Vx b, const std::vector<Vx> &tpl, std::vector<Vx> &pts) const;
    int hash (int r, int c, int kind) const;

    int m_rows; // tile rows
    int m_cols; // tile cols
    std::vector<Vx> m_junc; // primary junction px
    std::vector<Vx> m_junc2; // secondary clone when dup
    std::vector<uint8_t> m_dup; // 1 if corner was duplicated
    std::vector<uint8_t> m_diag; // 0=AD (NW-SE), 1=BC (NE-SW)
    std::vector<uint8_t> m_jx; // catalog axis index x
    std::vector<uint8_t> m_jy; // catalog axis index y
    std::vector<EdgePath> m_paths; // border polylines
    std::vector<std::vector<Vx>> m_bank; // manual threading bank
};

#endif // PRE_THREAD_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
