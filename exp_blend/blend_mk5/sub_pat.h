//================================================================================================================================
// Junction pattern banks and apply: classify 2x2, pick a random template, splat the corner 10x10.
// Beach banks: labels 0-3 = slot; 4-7 = desert if slot (N-4) is ocean/sea/coast and junction has land.
//================================================================================================================================

#ifndef SUB_PAT_H
#define SUB_PAT_H

#include <vector>
#include <string>
#include <cstdint>

#include "sub_cfg.h"
#include "sub_map.h"

//================================================================================================================================
//=> - Kind -
//================================================================================================================================

enum PatKind {
    PAT_NONE = 0, // all four equal
    PAT_CORNER = 1, // 3+1 solo
    PAT_HSPLIT = 2, // AA over BB (or rotated vertical)
    PAT_TCROSS = 3, // one edge-pair equal, three terrains
    PAT_DIAG = 4, // one diagonal equal, three terrains
    PAT_DDIAG = 5, // two textures on both diagonals (checker); uses diag bank
    PAT_QUAD = 6 // four distinct
};

//================================================================================================================================
//=> - Class: PatBank -
//================================================================================================================================

class PatBank {
public:
    PatBank ();
    ~PatBank ();

    bool ld (const std::string &dir, const char *prefix);
    int n () const;
    const uint8_t *get (int i) const; // N*N cells, row-major

private:
    std::vector<std::vector<uint8_t>> m_pats;
};

//================================================================================================================================
//=> - Class: PatSet -
//================================================================================================================================

class PatSet {
public:
    PatSet ();
    ~PatSet ();

    bool ld (const std::string &root);
    const PatBank& bank (PatKind k) const;

private:
    PatBank m_corner;
    PatBank m_hsplit;
    PatBank m_tcross;
    PatBank m_diag;
    PatBank m_quad;
};

//================================================================================================================================
//=> - Class: JuncRun -
//================================================================================================================================

class JuncRun {
public:
    JuncRun ();
    ~JuncRun ();

    void bind (const PatSet *pats);
    void run (const BlendMap &map, SubGrid &sg);
    int n_junc () const;
    double t_min_us () const;
    double t_max_us () const;
    double t_avg_us () const;

private:
    uint32_t seed (int a, int b, int c, int d) const;
    int pick (uint32_t &s, int n) const;
    void xform (int pr, int pc, int rot, bool flip_h, int &qr, int &qc) const;
    void apply (SubGrid &sg, int tr, int tc, const uint8_t *pat, int rot, bool flip_h, const int *ter, bool beach_ok);

    const PatSet *m_pats;
    int m_n;
    double m_tmin;
    double m_tmax;
    double m_tsum;
};

#endif // SUB_PAT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
