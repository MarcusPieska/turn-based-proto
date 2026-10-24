//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "pre_pull.h"

//================================================================================================================================
//=> - Class: CornPull -
//================================================================================================================================

CornPull::CornPull () : m_kind(PULL_NONE), m_n(0), m_solo(-1) {}
CornPull::~CornPull () {}

void CornPull::add (int ox, int oy) {
    if (m_n >= 16) return;
    m_off[m_n++] = Vx (ox, oy);
}

void CornPull::fill_solo (int which) {
    static const int dxs[4] = { -1, 1, -1, 1 };
    static const int dys[4] = { -1, -1, 1, 1 };
    int sx = dxs[which], sy = dys[which];
    for (int i = 0; i < CORNER_PULL_N; i++) {
        for (int j = 0; j < CORNER_PULL_N; j++) {
            add (sx * CORNER_PULL_U[i] * CORNER_UNIT, sy * CORNER_PULL_U[j] * CORNER_UNIT);
        }
    }
}

void CornPull::fill_h () {
    for (int i = 0; i < CORNER_PULL_N; i++) {
        int m = CORNER_PULL_U[i] * CORNER_UNIT;
        add (0, -m);
        add (0, m);
    }
}

void CornPull::fill_v () {
    for (int i = 0; i < CORNER_PULL_N; i++) {
        int m = CORNER_PULL_U[i] * CORNER_UNIT;
        add (-m, 0);
        add (m, 0);
    }
}

void CornPull::bind (int nw, int ne, int sw, int se) {
    m_kind = PULL_NONE;
    m_n = 0;
    m_solo = -1;
    if (nw == ne && ne == sw && sw == se) {
        add (0, 0);
        return;
    }
    int t[4] = { nw, ne, sw, se };
    int cnt[TERRAIN_ID_MAX];
    for (int i = 0; i < TERRAIN_ID_MAX; i++) cnt[i] = 0;
    for (int i = 0; i < 4; i++) {
        if (t[i] < 0 || t[i] >= TERRAIN_ID_MAX) continue;
        cnt[t[i]]++;
    }
    int solo_ter = -1, maj_ter = -1;
    for (int i = 0; i < TERRAIN_ID_MAX; i++) {
        if (cnt[i] == 1) solo_ter = i;
        if (cnt[i] == 3) maj_ter = i;
    }
    if (solo_ter >= 0 && maj_ter >= 0) {
        int which = 0;
        for (int i = 0; i < 4; i++) if (t[i] == solo_ter) which = i;
        m_solo = which;
        m_kind = PULL_SOLO;
        fill_solo (which);
        return;
    }
    if (nw == ne && sw == se && nw != sw) {
        m_kind = PULL_H;
        fill_h ();
        return;
    }
    if (nw == sw && ne == se && nw != ne) {
        m_kind = PULL_V;
        fill_v ();
        return;
    }
    m_kind = PULL_DUP;
    m_n = CORNER_PULL_N * CORNER_PULL_N;
}

PullKind CornPull::kind () const { return m_kind; }
int CornPull::n_off () const { return m_n; }
int CornPull::solo () const { return m_solo; }

Vx CornPull::off (int i) const {
    if (i < 0 || i >= m_n) return Vx (0, 0);
    return m_off[i];
}

Vx CornPull::into (int which, int i) const {
    static const int dxs[4] = { -1, 1, -1, 1 };
    static const int dys[4] = { -1, -1, 1, 1 };
    if (which < 0 || which > 3) return Vx (0, 0);
    int n = CORNER_PULL_N * CORNER_PULL_N;
    if (i < 0) i = 0;
    if (i >= n) i = n - 1;
    int ix = i % CORNER_PULL_N;
    int iy = i / CORNER_PULL_N;
    return Vx (dxs[which] * CORNER_PULL_U[ix] * CORNER_UNIT, dys[which] * CORNER_PULL_U[iy] * CORNER_UNIT);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
