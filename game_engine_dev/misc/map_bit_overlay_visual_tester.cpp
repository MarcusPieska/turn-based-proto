//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "map_bit_overlay.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Visual helpers -
//================================================================================================================================

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
//=> - Tester_MapBitOverlayVisual -
//================================================================================================================================

class Tester_MapBitOverlayVisual : public MapBitOverlay {
public:
    Tester_MapBitOverlayVisual () : MapBitOverlay(95, 95) {}

    void i_to_xy (u32 i, u16& x, u16& y) const {
        const u16 w = width();
        y = static_cast<u16>(i / static_cast<u32>(w));
        x = static_cast<u16>(i % static_cast<u32>(w));
    }

    void reset_all () {
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < width(); ++x) {
                clr(x, y);
            }
        }
    }

    void print_grid (cstr title) const {
        const u16 w = width();
        const u16 h = height();
        const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
        std::printf("%s\n", title);
        for (u16 y = 0; y < h; ++y) {
            for (u16 x = 0; x < w; ++x) {
                const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
                pr_bit(get(x, y));
                if (((i + 1u) % 8u) == 0u && (i + 1u) < n) {
                    std::printf(" ");
                }
            }
            std::printf("|\n");
        }
        std::printf("\n");
    }

    void set_byte_bit (u32 bi, u32 sh) {
        const u32 i = bi * 8u + sh;
        const u32 n = static_cast<u32>(width()) * static_cast<u32>(height());
        if (i >= n) {
            return;
        }
        u16 x = 0;
        u16 y = 0;
        i_to_xy(i, x, y);
        set(x, y);
    }

    void set_row_x (u16 y, u16 x) {
        set(x, y);
    }

    void run_case (cstr title, void (Tester_MapBitOverlayVisual::*fn) ()) {
        reset_all();
        (this->*fn)();
        print_grid(title);
        wait_ln();
    }

    void case_byte_first_bits () {
        for (u32 b = 0; b < byte_count(); ++b) {
            set_byte_bit(b, 0u);
        }
    }

    void case_byte_last_bits () {
        for (u32 b = 0; b < byte_count(); ++b) {
            set_byte_bit(b, 7u);
        }
    }

    void case_row_first_bits () {
        for (u16 y = 0; y < height(); ++y) {
            set_row_x(y, 0u);
        }
    }

    void case_row_last_bits () {
        const u16 x = static_cast<u16>(width() - 1u);
        for (u16 y = 0; y < height(); ++y) {
            set_row_x(y, x);
        }
    }

    void case_row_nibble_alternate () {
        for (u16 y = 0; y < height(); ++y) {
            if ((y & 1u) == 0u) {
                for (u16 x = 0; x < 4u; ++x) {
                    set_row_x(y, x);
                }
            } else {
                const u16 x0 = static_cast<u16>(width() - 4u);
                for (u16 x = x0; x < width(); ++x) {
                    set_row_x(y, x);
                }
            }
        }
    }

    void run_all () {
        run_case("1) first bit of every byte", &Tester_MapBitOverlayVisual::case_byte_first_bits);
        run_case("2) last bit of every byte", &Tester_MapBitOverlayVisual::case_byte_last_bits);
        run_case("3) first bit of every row", &Tester_MapBitOverlayVisual::case_row_first_bits);
        run_case("4) last bit of every row", &Tester_MapBitOverlayVisual::case_row_last_bits);
        run_case("5) even rows first 4 bits, odd rows last 4 bits",
            &Tester_MapBitOverlayVisual::case_row_nibble_alternate);
    }
};

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    Tester_MapBitOverlayVisual tester;
    tester.run_all();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
