//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "distribute_proportional.h"

//================================================================================================================================
//=> - DistributeProportional -
//================================================================================================================================

bool DistributeProportional::run (const DistPropEnt* ents, u16 n, u16 quota, i32* out) {
    if (ents == nullptr || out == nullptr) {
        return false;
    }
    for (u16 i = 0; i < n; ++i) {
        out[i] = 0;
    }
    if (n == 0u || quota == 0u) {
        return true;
    }
    u64 tot = 0u;
    for (u16 i = 0; i < n; ++i) {
        if (ents[i].m_rem > 0u && ents[i].m_tiles > 0u) {
            tot += static_cast<u64>(ents[i].m_tiles);
        }
    }
    if (tot == 0u) {
        return true;
    }
    u64 frac[256];
    if (n > 256u) {
        return false;
    }
    u16 placed = 0u;
    for (u16 i = 0; i < n; ++i) {
        frac[i] = 0u;
        if (ents[i].m_rem == 0u || ents[i].m_tiles == 0u) {
            continue;
        }
        const u64 prod = static_cast<u64>(quota) * static_cast<u64>(ents[i].m_tiles);
        u16 take = static_cast<u16>(prod / tot);
        if (take > ents[i].m_rem) {
            take = ents[i].m_rem;
        }
        out[i] = static_cast<i32>(take);
        frac[i] = prod % tot;
        placed = static_cast<u16>(placed + take);
    }
    u16 left = 0u;
    if (quota > placed) {
        left = static_cast<u16>(quota - placed);
    }
    while (left > 0u) {
        u16 best = U16_KEY_NULL;
        u64 best_f = 0u;
        u32 best_t = 0u;
        for (u16 i = 0; i < n; ++i) {
            if (static_cast<u16>(out[i]) >= ents[i].m_rem) {
                continue;
            }
            if (ents[i].m_tiles == 0u) {
                continue;
            }
            if (best == U16_KEY_NULL
                || frac[i] > best_f
                || (frac[i] == best_f && ents[i].m_tiles > best_t)) {
                best = i;
                best_f = frac[i];
                best_t = ents[i].m_tiles;
            }
        }
        if (best == U16_KEY_NULL) {
            break;
        }
        out[best] += 1;
        frac[best] = 0u;
        --left;
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
