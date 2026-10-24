//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "dia_pull.h"

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
    static const int dxs[4] = { -1, 0, 0, 1 };
    static const int dys[4] = { 0, -1, 1, 0 };
    int sx = dxs[which], sy = dys[which];
    for (int i = 0; i < CORNER_PULL_N; i++) {
        int u = CORNER_PULL_U[i];
        add (sx * u * PULL_UX, sy * u * PULL_UY);
    }
}

void CornPull::fill_h () {
    for (int i = 0; i < CORNER_PULL_N; i++) {
        int m = CORNER_PULL_U[i] * PULL_UY;
        add (0, -m);
        add (0, m);
    }
}

void CornPull::fill_v () {
    for (int i = 0; i < CORNER_PULL_N; i++) {
        int m = CORNER_PULL_U[i] * PULL_UX;
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
    m_n = CORNER_PULL_N;
}

PullKind CornPull::kind () const { return m_kind; }
int CornPull::n_off () const { return m_n; }
int CornPull::solo () const { return m_solo; }

Vx CornPull::off (int i) const {
    if (i < 0 || i >= m_n) return Vx (0, 0);
    return m_off[i];
}

Vx CornPull::into (int which, int i) const {
    static const int dxs[4] = { -1, 0, 0, 1 };
    static const int dys[4] = { 0, -1, 1, 0 };
    if (which < 0 || which > 3) return Vx (0, 0);
    if (i < 0) i = 0;
    if (i >= CORNER_PULL_N) i = CORNER_PULL_N - 1;
    int u = CORNER_PULL_U[i];
    return Vx (dxs[which] * u * PULL_UX, dys[which] * u * PULL_UY);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
