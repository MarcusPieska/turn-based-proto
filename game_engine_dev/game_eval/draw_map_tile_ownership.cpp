//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "draw_map_tile_ownership.h"

//================================================================================================================================
//=> - Palette -
//================================================================================================================================

const u8 DrawMapTileOwnership::k_pal[][3] = {
    {220, 40, 40},
    {40, 90, 220},
    {40, 170, 70},
    {220, 110, 30},
    {190, 40, 170},
    {30, 170, 170},
    {150, 70, 30},
    {100, 40, 180},
    {200, 160, 40},
    {40, 140, 200},
    {160, 50, 90},
    {90, 140, 40},
    {137, 25, 25},
    {25, 56, 137},
    {25, 106, 44},
    {137, 69, 19},
    {119, 25, 106},
    {19, 106, 106},
    {94, 44, 19},
    {62, 25, 112},
    {125, 100, 25},
    {25, 87, 125},
    {100, 31, 56},
    {56, 87, 25},
};
const u16 DrawMapTileOwnership::k_pal_n =
    static_cast<u16>(sizeof(DrawMapTileOwnership::k_pal) / sizeof(DrawMapTileOwnership::k_pal[0]));

//================================================================================================================================
//=> - DrawMapTileOwnership -
//================================================================================================================================

void DrawMapTileOwnership::sch (u16 seat, u16* a, u16* b, u8* dir) {
    const u16 nn = static_cast<u16>(k_pal_n * k_pal_n);
    const u16 pid = static_cast<u16>(seat % nn);
    *dir = static_cast<u8>(seat & 1u);
    *a = static_cast<u16>(pid % k_pal_n);
    *b = static_cast<u16>(pid / k_pal_n);
}

void DrawMapTileOwnership::lit (const u8* c, u8* out) {
    out[0] = static_cast<u8>((static_cast<u16>(c[0]) + 255u) / 2u);
    out[1] = static_cast<u8>((static_cast<u16>(c[1]) + 255u) / 2u);
    out[2] = static_cast<u8>((static_cast<u16>(c[2]) + 255u) / 2u);
}

void DrawMapTileOwnership::dim (const u8* c, u8* out) {
    out[0] = static_cast<u8>((static_cast<u16>(c[0]) * 5u) / 8u);
    out[1] = static_cast<u8>((static_cast<u16>(c[1]) * 5u) / 8u);
    out[2] = static_cast<u8>((static_cast<u16>(c[2]) * 5u) / 8u);
}

bool DrawMapTileOwnership::edge (const u8* seats, u16 w, u16 h, u16 x, u16 y, u8 seat) {
    const i32 xi = static_cast<i32>(x);
    const i32 yi = static_cast<i32>(y);
    const i32 r = static_cast<i32>(k_border);
    for (i32 dy = -r; dy <= r; ++dy) {
        for (i32 dx = -r; dx <= r; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            const i32 adx = (dx < 0) ? -dx : dx;
            const i32 ady = (dy < 0) ? -dy : dy;
            if (adx > r || ady > r) {
                continue;
            }
            const i32 nx = xi + dx;
            const i32 ny = yi + dy;
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                return true;
            }
            const u8 n = seats[static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx)];
            if (n != seat) {
                return true;
            }
        }
    }
    return false;
}

void DrawMapTileOwnership::rgb (u16 seat, u16 x, u16 y, u8* out) {
    if (out == nullptr) {
        return;
    }
    if (seat == static_cast<u16>(U8_KEY_NULL)) {
        out[0] = k_none_r;
        out[1] = k_none_g;
        out[2] = k_none_b;
        return;
    }
    u16 a = 0;
    u16 b = 0;
    u8 dir = 0;
    sch(seat, &a, &b, &dir);
    const u32 axis = (dir == 0u)
        ? (static_cast<u32>(x) + static_cast<u32>(y))
        : (static_cast<u32>(x) - static_cast<u32>(y));
    const u16 band = static_cast<u16>(axis / static_cast<u32>(k_stripe));
    if ((band & 1u) != 0u) {
        dim(k_pal[b], out);
    } else {
        lit(k_pal[a], out);
    }
}

bool DrawMapTileOwnership::brd (const u8* seats, u16 w, u16 h, u16 x, u16 y) {
    if (seats == nullptr || x >= w || y >= h) {
        return false;
    }
    const u8 seat = seats[static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)];
    if (seat == U8_KEY_NULL) {
        return false;
    }
    return edge(seats, w, h, x, y, seat);
}

void DrawMapTileOwnership::at (const u8* seats, u16 w, u16 h, u16 x, u16 y, u8* out) {
    if (out == nullptr || seats == nullptr || x >= w || y >= h) {
        return;
    }
    const u8 seat = seats[static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)];
    if (seat == U8_KEY_NULL) {
        out[0] = k_none_r;
        out[1] = k_none_g;
        out[2] = k_none_b;
        return;
    }
    if (edge(seats, w, h, x, y, seat)) {
        u16 a = 0;
        u16 b = 0;
        u8 dir = 0;
        sch(static_cast<u16>(seat), &a, &b, &dir);
        lit(k_pal[a], out);
        return;
    }
    rgb(static_cast<u16>(seat), x, y, out);
}

void DrawMapTileOwnership::blend (u8* dst, u16 w, u16 h, u16 x, u16 y, const u8* seats) {
    if (dst == nullptr || seats == nullptr || x >= w || y >= h) {
        return;
    }
    const u8 seat = seats[static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)];
    if (seat == U8_KEY_NULL) {
        return;
    }
    u8 c[3];
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    if (edge(seats, w, h, x, y, seat)) {
        u16 a = 0;
        u16 b = 0;
        u8 dir = 0;
        sch(static_cast<u16>(seat), &a, &b, &dir);
        lit(k_pal[a], c);
        dst[i + 0] = static_cast<u8>((static_cast<u16>(dst[i + 0]) + static_cast<u16>(c[0]) * 7u) / 8u);
        dst[i + 1] = static_cast<u8>((static_cast<u16>(dst[i + 1]) + static_cast<u16>(c[1]) * 7u) / 8u);
        dst[i + 2] = static_cast<u8>((static_cast<u16>(dst[i + 2]) + static_cast<u16>(c[2]) * 7u) / 8u);
        return;
    }
    DrawMapTileOwnership::rgb(static_cast<u16>(seat), x, y, c);
    const u16 cr = (static_cast<u16>(c[0]) * 5u) / 8u;
    const u16 cg = (static_cast<u16>(c[1]) * 5u) / 8u;
    const u16 cb = (static_cast<u16>(c[2]) * 5u) / 8u;
    dst[i + 0] = static_cast<u8>((static_cast<u16>(dst[i + 0]) + cr * 3u) / 4u);
    dst[i + 1] = static_cast<u8>((static_cast<u16>(dst[i + 1]) + cg * 3u) / 4u);
    dst[i + 2] = static_cast<u8>((static_cast<u16>(dst[i + 2]) + cb * 3u) / 4u);
}

bool DrawMapTileOwnership::fill (u8* ov, u16 w, u16 h, const u8* seats) {
    if (ov == nullptr || seats == nullptr || w == 0u || h == 0u) {
        return false;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
            at(seats, w, h, x, y, ov + i);
        }
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
