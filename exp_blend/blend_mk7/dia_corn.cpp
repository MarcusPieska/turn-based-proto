//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "dia_corn.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static int corn_at (const int *grid, int rows, int cols, int r, int c) {
    if (r < 0 || c < 0 || r >= rows || c >= cols) return -1;
    return grid[r * cols + c];
}

static bool corn_triple (int a, int b, int c, int t) {
    if (a < 0 || b < 0 || c < 0) return false;
    return a == b && b == c && a != t;
}

//================================================================================================================================
//=> - Grid eval -
//================================================================================================================================

uint8_t dia_corn_at (const int *grid, int rows, int cols, int r, int c) {
    int t = corn_at (grid, rows, cols, r, c);
    uint8_t b = 0;
    if (corn_triple (corn_at (grid, rows, cols, r - 1, c - 1),
                     corn_at (grid, rows, cols, r - 1, c),
                     corn_at (grid, rows, cols, r, c - 1), t))
        b |= CORN_W;
    if (corn_triple (corn_at (grid, rows, cols, r - 1, c),
                     corn_at (grid, rows, cols, r - 1, c + 1),
                     corn_at (grid, rows, cols, r, c + 1), t))
        b |= CORN_N;
    if (corn_triple (corn_at (grid, rows, cols, r, c + 1),
                     corn_at (grid, rows, cols, r + 1, c + 1),
                     corn_at (grid, rows, cols, r + 1, c), t))
        b |= CORN_E;
    if (corn_triple (corn_at (grid, rows, cols, r, c - 1),
                     corn_at (grid, rows, cols, r + 1, c - 1),
                     corn_at (grid, rows, cols, r + 1, c), t))
        b |= CORN_S;
    return b;
}

void dia_corn_grid (const int *grid, int rows, int cols, uint8_t *out) {
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++)
            out[r * cols + c] = dia_corn_at (grid, rows, cols, r, c);
    }
}

uint8_t dia_tips_si_sj (uint8_t bits) {
    uint8_t o = 0;
    if (bits & CORN_N) o |= CORN_W;
    if (bits & CORN_W) o |= CORN_N;
    if (bits & CORN_E) o |= CORN_S;
    if (bits & CORN_S) o |= CORN_E;
    return o;
}

//================================================================================================================================
//=> - C ABI -
//================================================================================================================================

extern "C" void dia_corn_2x2 (const int *cells, int *out_bits) {
    uint8_t bits[4];
    dia_corn_grid (cells, 2, 2, bits);
    for (int i = 0; i < 4; i++) out_bits[i] = (int)bits[i];
}

extern "C" void dia_corn_grid_c (const int *grid, int rows, int cols, int *out_bits) {
    for (int i = 0; i < rows * cols; i++) {
        int r = i / cols, c = i % cols;
        out_bits[i] = (int)dia_corn_at (grid, rows, cols, r, c);
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
