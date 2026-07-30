//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "civ_trait_affinity.h"

//================================================================================================================================
//=> - Circular trait ring (edit order here) -
//================================================================================================================================
//
//  Neighbors on the ring are friendly; distance-2 neutral; distance-3 hostile.
//  Preference unroll from owner: self, +1, -1, +2, -2, +3, -3 (clockwise then counter-clockwise).
//
//================================================================================================================================

static const CivTrait k_ring[CivTraitAffinity::k_n] = {
    CivTrait::Commercial,
    CivTrait::Industrious,
    CivTrait::Scientific,
    CivTrait::Militaristic,
    CivTrait::Expansionist,
    CivTrait::Religious,
    CivTrait::Agricultural,
};

/*
static const CivTrait k_ring[CivTraitAffinity::k_n] = {
    CivTrait::Commercial,
    CivTrait::Expansionist,
    CivTrait::Agricultural,
    CivTrait::Religious,
    CivTrait::Industrious,
    CivTrait::Scientific,
    CivTrait::Militaristic,
};
*/

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u16 ring_idx (CivTrait trait) {
    for (u16 i = 0; i < CivTraitAffinity::k_n; ++i) {
        if (k_ring[i] == trait) {
            return i;
        }
    }
    return 0;
}

static u16 circ_dist (u16 a, u16 b) {
    const u16 n = CivTraitAffinity::k_n;
    u16 d = (a > b) ? static_cast<u16>(a - b) : static_cast<u16>(b - a);
    const u16 other = static_cast<u16>(n - d);
    if (other < d) {
        d = other;
    }
    return d;
}

static CivTrait ring_step (u16 from, i16 step) {
    const u16 n = CivTraitAffinity::k_n;
    i32 idx = static_cast<i32>(from) + static_cast<i32>(step);
    while (idx < 0) {
        idx += static_cast<i32>(n);
    }
    while (idx >= static_cast<i32>(n)) {
        idx -= static_cast<i32>(n);
    }
    return k_ring[static_cast<u16>(idx)];
}

//================================================================================================================================
//=> - CivTraitAffinity -
//================================================================================================================================

CivTrait CivTraitAffinity::ring_at (u16 idx) {
    if (idx >= k_n) {
        return k_ring[0];
    }
    return k_ring[idx];
}

CivTrait CivTraitAffinity::pref (CivTrait owner, u16 rank) {
    if (rank >= k_n) {
        return owner;
    }
    if (rank == 0) {
        return owner;
    }
    const u16 base = ring_idx(owner);
    const u16 dist = static_cast<u16>((rank + 1u) / 2u);
    const i16 step = ((rank % 2u) != 0u) ? static_cast<i16>(dist) : static_cast<i16>(-static_cast<i16>(dist));
    return ring_step(base, step);
}

u16 CivTraitAffinity::rank_of (CivTrait owner, CivTrait target) {
    for (u16 r = 0; r < k_n; ++r) {
        if (pref(owner, r) == target) {
            return r;
        }
    }
    return static_cast<u16>(k_n - 1u);
}

i8 CivTraitAffinity::affinity (CivTrait owner, CivTrait target) {
    const u16 d = circ_dist(ring_idx(owner), ring_idx(target));
    if (d == 0) {
        return 6;
    }
    if (d == 1) {
        return 4;
    }
    if (d == 2) {
        return 2;
    }
    return 0;
}

i32 CivTraitAffinity::received_score (CivTrait target) {
    i32 sum = 0;
    for (u16 o = 0; o < k_n; ++o) {
        sum += static_cast<i32>(affinity(static_cast<CivTrait>(o), target));
    }
    return sum;
}

void CivTraitAffinity::received_scores (i32* out_scores) {
    if (out_scores == nullptr) {
        return;
    }
    for (u16 t = 0; t < k_n; ++t) {
        out_scores[t] = received_score(static_cast<CivTrait>(t));
    }
}

bool CivTraitAffinity::row_is_permutation (CivTrait owner) {
    u8 seen = 0;
    for (u16 r = 0; r < k_n; ++r) {
        const u16 ix = static_cast<u16>(pref(owner, r));
        if (ix >= k_n) {
            return false;
        }
        const u8 bit = static_cast<u8>(1u << ix);
        if ((seen & bit) != 0) {
            return false;
        }
        seen = static_cast<u8>(seen | bit);
    }
    return seen == static_cast<u8>((1u << k_n) - 1u);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
