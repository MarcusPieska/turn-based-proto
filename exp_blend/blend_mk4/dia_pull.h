//================================================================================================================================
// Corner pull rules from the 2x2 terrains at a diamond right-corner junction.
//================================================================================================================================

#ifndef DIA_PULL_H
#define DIA_PULL_H

#include "dia_cfg.h"

//================================================================================================================================
//=> - Pull pattern -
//================================================================================================================================

enum PullKind {
    PULL_NONE = 0, // all four equal: fixed origin
    PULL_SOLO = 1, // 3x + 1A: pull into A
    PULL_H = 2, // AA over BB: dent in y
    PULL_V = 3, // AB|AB: dent in x
    PULL_DUP = 4 // 4-way / checker: clone opposite corners
};

//================================================================================================================================
//=> - Class: CornPull -
//================================================================================================================================

class CornPull {
public:
    CornPull ();
    ~CornPull ();

    void bind (int nw, int ne, int sw, int se);
    PullKind kind () const;
    int n_off () const;
    int solo () const;
    Vx off (int i) const;
    Vx into (int which, int i) const;

private:
    void add (int ox, int oy);
    void fill_solo (int which);
    void fill_h ();
    void fill_v ();

    PullKind m_kind; // classified 2x2 pattern
    int m_n; // allowed offset count
    int m_solo; // solo quadrant 0..3, else -1
    Vx m_off[16]; // allowed pixel offsets
};

#endif // DIA_PULL_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
