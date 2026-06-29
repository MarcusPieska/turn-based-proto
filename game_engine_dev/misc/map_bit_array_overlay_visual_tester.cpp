//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "map_bit_array_overlay.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Visual helpers -
//================================================================================================================================

static u32 popcnt (u32 v) {
    u32 n = 0;
    while (v != 0u) {
        n += v & 1u;
        v >>= 1u;
    }
    return n;
}

static void pr_cnt (u32 val) {
    const u32 n = popcnt(val);
    if (n > 0u) {
        std::printf("\033[32m%u\033[0m", n);
    } else {
        std::printf("\033[31m0\033[0m");
    }
}

static void pr_bit (u32 v) {
    if (v) {
        std::printf("\033[32m1\033[0m");
    } else {
        std::printf("\033[31m0\033[0m");
    }
}

static void wait_ln () {
    std::printf("press enter to continue...\n");
    std::fflush(stdout);
    std::getchar();
}

//================================================================================================================================
//=> - Tester_MapBitArrayOverlayVisual -
//================================================================================================================================

class Tester_MapBitArrayOverlayVisual : public MapBitArrayOverlay {
public:
    Tester_MapBitArrayOverlayVisual () : MapBitArrayOverlay(95, 95, 9) {}

    u32 tile_n () const {
        return static_cast<u32>(width()) * static_cast<u32>(height());
    }

    void reset_all () {
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < width(); ++x) {
                set(x, y, 0u);
            }
        }
    }

    void print_grid (cstr title) const {
        const u16 w = width();
        const u16 h = height();
        const u32 bpv = static_cast<u32>(bits_per_val());
        const u32 n = tile_n();
        std::printf("%s (bpv %u)\n", title, static_cast<unsigned>(bpv));
        for (u16 y = 0; y < h; ++y) {
            for (u16 x = 0; x < w; ++x) {
                const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
                pr_cnt(get(x, y));
                if (((i + 1u) % 5u) == 0u && (i + 1u) < n) {
                    std::printf(" ");
                }
            }
            std::printf("|\n");
        }
        std::printf("\n");
    }

    void run_case (cstr title, void (Tester_MapBitArrayOverlayVisual::*fn) ()) {
        reset_all();
        (this->*fn)();
        print_grid(title);
        wait_ln();
    }

    void case_alt_one () {
        const u16 w = width();
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < w; ++x) {
                const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
                set(x, y, (i & 1u) == 0u ? 1u : 0u);
            }
        }
    }

    void case_tile_ramp_cycle () {
        const u16 w = width();
        const u32 bpv = static_cast<u32>(bits_per_val());
        const u32 period = bpv + 1u;
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < w; ++x) {
                const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
                const u32 nb = i % period;
                const u32 val = nb == 0u ? 0u : ((1u << nb) - 1u);
                set(x, y, val);
            }
        }
    }

    void run_all () {
        run_case("1) every other tile 1, else 0", &Tester_MapBitArrayOverlayVisual::case_alt_one);
        run_case("2) tile i has i%(bpv+1) bits set", &Tester_MapBitArrayOverlayVisual::case_tile_ramp_cycle);
    }
};

//================================================================================================================================
//=> - Tester_MapBitArrayOverlayVisualSmall -
//================================================================================================================================

class Tester_MapBitArrayOverlayVisualSmall : public MapBitArrayOverlay {
public:
    Tester_MapBitArrayOverlayVisualSmall () : MapBitArrayOverlay(10, 10, 9) {}

    u32 tile_n () const {
        return static_cast<u32>(width()) * static_cast<u32>(height());
    }

    u32 tot_bits () const {
        return tile_n() * static_cast<u32>(bits_per_val());
    }

    void reset_all () {
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < width(); ++x) {
                set(x, y, 0u);
            }
        }
    }

    void print_grid (cstr title) const {
        const u16 w = width();
        const u16 h = height();
        const u32 bpv = static_cast<u32>(bits_per_val());
        const u32 nbits = tot_bits();
        std::printf("%s (%ux%u bpv %u)\n", title, static_cast<unsigned>(w),
            static_cast<unsigned>(h), static_cast<unsigned>(bpv));
        for (u16 y = 0; y < h; ++y) {
            for (u16 x = 0; x < w; ++x) {
                const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
                const u32 val = get(x, y);
                for (u32 vb = 0; vb < bpv; ++vb) {
                    const u32 gbit = i * bpv + vb;
                    pr_bit((val >> vb) & 1u);
                    if (((gbit + 1u) % 8u) == 0u && (gbit + 1u) < nbits) {
                        std::printf(" ");
                    }
                    if (((gbit + 1u) % bpv) == 0u && (gbit + 1u) < nbits) {
                        std::printf(":");
                    }
                }
            }
            std::printf("|\n");
        }
        std::printf("\n");
    }

    void run_case (cstr title, void (Tester_MapBitArrayOverlayVisualSmall::*fn) ()) {
        reset_all();
        (this->*fn)();
        print_grid(title);
        wait_ln();
    }

    void case_alt_one () {
        const u16 w = width();
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < w; ++x) {
                const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
                set(x, y, (i & 1u) == 0u ? 1u : 0u);
            }
        }
    }

    void case_tile_ramp_cycle () {
        const u16 w = width();
        const u32 bpv = static_cast<u32>(bits_per_val());
        const u32 period = bpv + 1u;
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < w; ++x) {
                const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
                const u32 nb = i % period;
                const u32 val = nb == 0u ? 0u : ((1u << nb) - 1u);
                set(x, y, val);
            }
        }
    }

    void run_all () {
        std::printf("--- 10x10 expanded ---\n");
        run_case("1) every other tile 1, else 0", &Tester_MapBitArrayOverlayVisualSmall::case_alt_one);
        run_case("2) tile i has i%(bpv+1) bits set", &Tester_MapBitArrayOverlayVisualSmall::case_tile_ramp_cycle);
    }
};

//================================================================================================================================
//=> - Tester_MapBitArrayOverlayVisualPack -
//================================================================================================================================

class Tester_MapBitArrayOverlayVisualPack : public MapBitArrayOverlay {
public:
    Tester_MapBitArrayOverlayVisualPack () : MapBitArrayOverlay(10, 1, 7) {}

    u32 tile_n () const {
        return static_cast<u32>(width()) * static_cast<u32>(height());
    }

    u32 tot_bits () const {
        return tile_n() * static_cast<u32>(bits_per_val());
    }

    void reset_all () {
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < width(); ++x) {
                set_uint(x, y, 0u);
            }
        }
    }

    void print_grid (cstr title) const {
        const u16 w = width();
        const u16 h = height();
        const u32 bpv = static_cast<u32>(bits_per_val());
        const u32 nbits = tot_bits();
        std::printf("%s (%ux%u bpv %u)\n", title, static_cast<unsigned>(w),
            static_cast<unsigned>(h), static_cast<unsigned>(bpv));
        for (u16 y = 0; y < h; ++y) {
            for (u16 x = 0; x < w; ++x) {
                const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
                const u32 val = get_uint(x, y);
                for (u32 vb = 0; vb < bpv; ++vb) {
                    const u32 gbit = i * bpv + vb;
                    pr_bit((val >> vb) & 1u);
                    if (((gbit + 1u) % 8u) == 0u && (gbit + 1u) < nbits) {
                        std::printf(" ");
                    }
                    if (((gbit + 1u) % bpv) == 0u && (gbit + 1u) < nbits) {
                        std::printf(":");
                    }
                }
            }
            std::printf("|\n");
        }
        std::printf("\n");
    }

    void print_rows () const {
        const u16 w = width();
        const u16 h = height();
        const u32 bpv = static_cast<u32>(bits_per_val());
        const u32 nbits = tot_bits();
        for (u16 y = 0; y < h; ++y) {
            for (u16 x = 0; x < w; ++x) {
                const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
                const u32 val = get_uint(x, y);
                for (u32 vb = 0; vb < bpv; ++vb) {
                    const u32 gbit = i * bpv + vb;
                    pr_bit((val >> vb) & 1u);
                    if (((gbit + 1u) % 8u) == 0u && (gbit + 1u) < nbits) {
                        std::printf(" ");
                    }
                    if (((gbit + 1u) % bpv) == 0u && (gbit + 1u) < nbits) {
                        std::printf(":");
                    }
                }
            }
            std::printf("|\n");
        }
    }

    void case_numerical_pack_walk () {
        static const u16 xs[] = {0, 2, 4, 6, 8};
        const u32 vmax = max_val();
        for (u32 val = 0; val <= vmax; ++val) {
            reset_all();
            for (u32 xi = 0; xi < 5u; ++xi) {
                set_uint(xs[xi], 0u, val);
            }
            print_rows();
        }
    }

    void run_all () {
        case_numerical_pack_walk();
    }
};

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    Tester_MapBitArrayOverlayVisual tester;
    tester.run_all();
    Tester_MapBitArrayOverlayVisualSmall small;
    small.run_all();
    Tester_MapBitArrayOverlayVisualPack pack;
    pack.run_all();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
