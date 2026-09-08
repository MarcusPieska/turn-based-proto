//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "select_on_continent.h"

#include <cmath>
#include <vector>

//================================================================================================================================
//=> - Tunables -
//================================================================================================================================

#define SEL_SCORE_SCALE 1000
#define SEL_DIST_SCALE 500

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static f32 avg_dist (SpgCoordPair pt, const SpgCoordPair* sels, u16 sn) {
    if (sels == nullptr || sn == 0u) {
        return 0.0f;
    }
    f32 sum = 0.0f;
    for (u16 i = 0; i < sn; ++i) {
        const f32 dx = static_cast<f32>(pt.x) - static_cast<f32>(sels[i].x);
        const f32 dy = static_cast<f32>(pt.y) - static_cast<f32>(sels[i].y);
        sum += std::sqrt(dx * dx + dy * dy);
    }
    return sum / static_cast<f32>(sn);
}

//================================================================================================================================
//=> - SelectOnContinent -
//================================================================================================================================

bool SelectOnContinent::pick (
    const SpgCoordPair* cands,
    const i32* scores,
    u16 cand_n,
    const SpgCoordPair* selected,
    u16 sel_n,
    u16* out_idx)
{
    if (cands == nullptr || scores == nullptr || out_idx == nullptr || cand_n == 0u) {
        return false;
    }
    i32 sc_lo = scores[0];
    i32 sc_hi = scores[0];
    for (u16 i = 1; i < cand_n; ++i) {
        if (scores[i] < sc_lo) {
            sc_lo = scores[i];
        }
        if (scores[i] > sc_hi) {
            sc_hi = scores[i];
        }
    }
    std::vector<f32> dist(cand_n, 0.0f);
    f32 d_lo = 0.0f;
    f32 d_hi = 0.0f;
    for (u16 i = 0; i < cand_n; ++i) {
        dist[i] = avg_dist(cands[i], selected, sel_n);
        if (i == 0u || dist[i] < d_lo) {
            d_lo = dist[i];
        }
        if (i == 0u || dist[i] > d_hi) {
            d_hi = dist[i];
        }
    }
    i32 best_j = -1;
    u16 best_i = 0u;
    for (u16 i = 0; i < cand_n; ++i) {
        i32 sn = SEL_SCORE_SCALE / 2;
        if (sc_hi > sc_lo) {
            sn = static_cast<i32>(
                (static_cast<i64>(scores[i] - sc_lo) * static_cast<i64>(SEL_SCORE_SCALE))
                / static_cast<i64>(sc_hi - sc_lo));
        } else {
            sn = SEL_SCORE_SCALE;
        }
        i32 dn = SEL_DIST_SCALE / 2;
        if (sel_n == 0u) {
            dn = SEL_DIST_SCALE;
        } else if (d_hi > d_lo) {
            dn = static_cast<i32>(((dist[i] - d_lo) / (d_hi - d_lo)) * static_cast<f32>(SEL_DIST_SCALE) + 0.5f);
        } else {
            dn = SEL_DIST_SCALE;
        }
        const i32 joint = sn + dn;
        if (best_j < 0 || joint > best_j) {
            best_j = joint;
            best_i = i;
        }
    }
    *out_idx = best_i;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
