//================================================================================================================================
// Pre-determined diamond border threading (mk4): cataloged corners, dup splits, hashed templates morphed c2c.
//================================================================================================================================

#ifndef DIA_THREAD_H
#define DIA_THREAD_H

#include <vector>
#include <string>

#include "dia_cfg.h"
#include "dia_grid.h"

//================================================================================================================================
//=> - Edge path record -
//================================================================================================================================

struct EdgePath {
    EdgeKind kind; // NE, SE, or diagonal connector
    int r; // anchor tile row (or jr for D)
    int c; // anchor tile col (or jc for D)
    int ta; // terrain a
    int tb; // terrain b
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
    void run (const BlendMap &map, const DiaGrid &grid);
    void run_c2c (const BlendMap &map, const DiaGrid &grid, int c2c);
    int n_path () const;
    const EdgePath& path (int i) const;
    Vx junc (int jr, int jc) const;
    Vx junc2 (int jr, int jc) const;
    bool dup (int jr, int jc) const;

private:
    uint32_t seed (int a, int b, int c, int d) const;
    int pick (uint32_t &s, int n) const;
    int idx (int jr, int jc) const;
    void calc_juncs (const BlendMap &map, const DiaGrid &grid);
    Vx end (int jr, int jc, int t0r, int t0c, int t1r, int t1c) const;
    void walk_ne (const BlendMap &map, int r, int c);
    void walk_se (const BlendMap &map, int r, int c);
    void walk_ne_c2c (const BlendMap &map, int r, int c, int c2c);
    void walk_se_c2c (const BlendMap &map, int r, int c, int c2c);
    void link (const BlendMap &map, bool use_bank, int c2c);
    void line (Vx a, Vx b, int n_mid, std::vector<Vx> &pts) const;
    void morph (Vx a, Vx b, const std::vector<Vx> &tpl, std::vector<Vx> &pts) const;
    int hash (int r, int c, int kind) const;

    int m_rows; // tile rows
    int m_cols; // tile cols
    int m_jrows; // junction table rows
    int m_jcols; // junction table cols
    std::vector<Vx> m_junc; // primary junction px
    std::vector<Vx> m_junc2; // secondary clone when dup
    std::vector<uint8_t> m_dup; // 1 if corner was duplicated
    std::vector<uint8_t> m_diag; // 0=AD (NW-SE), 1=BC (NE-SW)
    std::vector<EdgePath> m_paths; // border polylines
    std::vector<std::vector<Vx>> m_bank; // manual threading bank
};

#endif // DIA_THREAD_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
