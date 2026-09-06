//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "target_sort_by_core.h"

#include "city.h"
#include "city_array.h"
#include "game_state.h"

//================================================================================================================================
//=> - TargetSortByCore -
//================================================================================================================================

i32 TargetSortByCore::part (u16* tgts, u32* keys, i32 lo, i32 hi) {
    const u32 piv = keys[hi];
    i32 i = lo;
    for (i32 j = lo; j < hi; ++j) {
        if (keys[j] <= piv) {
            const u16 tt = tgts[i];
            tgts[i] = tgts[j];
            tgts[j] = tt;
            const u32 kt = keys[i];
            keys[i] = keys[j];
            keys[j] = kt;
            ++i;
        }
    }
    const u16 tt = tgts[i];
    tgts[i] = tgts[hi];
    tgts[hi] = tt;
    const u32 kt = keys[i];
    keys[i] = keys[hi];
    keys[hi] = kt;
    return i;
}

void TargetSortByCore::qsort (u16* tgts, u32* keys, i32 lo, i32 hi) {
    if (lo >= hi) {
        return;
    }
    const i32 p = part(tgts, keys, lo, hi);
    qsort(tgts, keys, lo, p - 1);
    qsort(tgts, keys, p + 1, hi);
}

void TargetSortByCore::sort (const GameState& st, u8 attacker, u16* tgts, u16 n) {
    if (tgts == nullptr || n <= 1u) {
        return;
    }
    if (n > TARGET_SORT_BY_CORE_CAP) {
        n = static_cast<u16>(TARGET_SORT_BY_CORE_CAP);
    }
    u64 sx = 0;
    u64 sy = 0;
    u32 an = 0;
    const u16 cn = st.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = st.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != attacker) {
            continue;
        }
        sx += c->get_x();
        sy += c->get_y();
        ++an;
    }
    if (an == 0u) {
        return;
    }
    const u16 ax = static_cast<u16>(sx / an);
    const u16 ay = static_cast<u16>(sy / an);
    u32 keys[TARGET_SORT_BY_CORE_CAP];
    for (u16 i = 0; i < n; ++i) {
        const City* c = st.m_cities.get_city(tgts[i]);
        if (c == nullptr) {
            keys[i] = 0xFFFFFFFFu;
            continue;
        }
        const u16 cx = c->get_x();
        const u16 cy = c->get_y();
        const u32 adx = cx > ax ? static_cast<u32>(cx - ax) : static_cast<u32>(ax - cx);
        const u32 ady = cy > ay ? static_cast<u32>(cy - ay) : static_cast<u32>(ay - cy);
        keys[i] = adx + ady;
    }
    qsort(tgts, keys, 0, static_cast<i32>(n) - 1);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
