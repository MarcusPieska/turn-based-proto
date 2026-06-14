//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>
#include <vector>

#include "generate_global_ocean.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static bool is_wat_cls (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static u32 tidx (u16 w, u32 x, u32 y) {
    return y * static_cast<u32>(w) + x;
}

static void seed_tile (
    const u8* terrain,
    u16 w,
    u16 h,
    u32 x,
    u32 y,
    u8* mask,
    std::vector<u32>* q) 
{
    if (x >= static_cast<u32>(w) || y >= static_cast<u32>(h)) {
        return;
    }
    const u32 i = tidx(w, x, y);
    if (!is_wat_cls(terrain[i]) || mask[i] != 0) {
        return;
    }
    mask[i] = 1;
    q->push_back(i);
}

//================================================================================================================================
//=> - Generate_GlobalOcean -
//================================================================================================================================

u8* Generate_GlobalOcean::build_mask (const u8* terrain, u16 w, u16 h) {
    if (terrain == nullptr || w == 0 || h == 0) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* mask = new u8[n];
    if (mask == nullptr) {
        return nullptr;
    }
    std::memset(mask, 0, n);
    std::vector<u32> q;
    q.reserve(4096);
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    for (u32 x = 0; x < wi; ++x) {
        seed_tile(terrain, w, h, x, 0, mask, &q);
        seed_tile(terrain, w, h, x, hi - 1u, mask, &q);
    }
    for (u32 y = 0; y < hi; ++y) {
        seed_tile(terrain, w, h, 0, y, mask, &q);
        seed_tile(terrain, w, h, wi - 1u, y, mask, &q);
    }
    for (u32 i = 0; i < n; ++i) {
        if (terrain[i] == TERR_OCEAN[0]) {
            seed_tile(terrain, w, h, i % wi, i / wi, mask, &q);
        }
    }
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    for (std::size_t qi = 0; qi < q.size(); ++qi) {
        const u32 i = q[qi];
        const u32 py = i / wi;
        const u32 px = i - py * wi;
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(px) + dx4[d];
            const i32 ny = static_cast<i32>(py) + dy4[d];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                continue;
            }
            const u32 ni = tidx(w, static_cast<u32>(nx), static_cast<u32>(ny));
            if (!is_wat_cls(terrain[ni]) || mask[ni] != 0) {
                continue;
            }
            mask[ni] = 1;
            q.push_back(ni);
        }
    }
    return mask;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
