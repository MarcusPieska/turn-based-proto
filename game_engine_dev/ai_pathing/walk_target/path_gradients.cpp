//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "path_gradients.h"

//================================================================================================================================
//=> - PathGradients -
//================================================================================================================================

PathGradients::PathGradients () {
    for (u16 i = 0; i < k_cap; ++i) {
        m_g[i] = nullptr;
    }
}

PathGradients::~PathGradients () {
    for (u16 i = 0; i < k_cap; ++i) {
        delete m_g[i];
        m_g[i] = nullptr;
    }
}

bool PathGradients::ens (u16 i) {
    if (i >= k_cap) {
        return false;
    }
    if (m_g[i] == nullptr) {
        m_g[i] = new WalkP2P();
    }
    return m_g[i] != nullptr && m_g[i]->ok();
}

WalkP2P& PathGradients::at (u16 i) {
    if (i >= k_cap) {
        i = 0u;
    }
    if (m_g[i] == nullptr) {
        m_g[i] = new WalkP2P();
    }
    return *m_g[i];
}

const WalkP2P& PathGradients::at (u16 i) const {
    return *m_g[(i < k_cap && m_g[i] != nullptr) ? i : 0u];
}

bool PathGradients::ok (u16 i) const {
    return i < k_cap && m_g[i] != nullptr && m_g[i]->ok();
}

u16 PathGradients::w (u16 i) const {
    return ok(i) ? m_g[i]->w() : 0u;
}

u16 PathGradients::h (u16 i) const {
    return ok(i) ? m_g[i]->h() : 0u;
}

u16* PathGradients::turn (u16 i) {
    return ens(i) ? m_g[i]->turn() : nullptr;
}

const u16* PathGradients::turn (u16 i) const {
    return ok(i) ? m_g[i]->turn() : nullptr;
}

u16* PathGradients::rem (u16 i) {
    return ens(i) ? m_g[i]->rem() : nullptr;
}

const u16* PathGradients::rem (u16 i) const {
    return ok(i) ? m_g[i]->rem() : nullptr;
}

PathGradients::StepRes PathGradients::peek (u16 i, const GameState& s, u16 x, u16 y) const {
    if (!ok(i)) {
        StepRes r;
        r.nx = 0u;
        r.ny = 0u;
        r.cost = 0u;
        r.have = false;
        r.reason = PeekFail::NotReady;
        return r;
    }
    return m_g[i]->peek(s, x, y);
}

void PathGradients::peek_opts (u16 i, const GameState& s, u16 x, u16 y, u16* ox, u16* oy) const {
    if (!ok(i) || ox == nullptr || oy == nullptr) {
        return;
    }
    m_g[i]->peek_opts(s, x, y, ox, oy);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
