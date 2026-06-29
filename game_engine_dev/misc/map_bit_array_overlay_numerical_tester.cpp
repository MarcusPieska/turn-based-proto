//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "map_bit_array_overlay.h"

//================================================================================================================================
//=> - Tester_MapBitArrayOverlayNumerical -
//================================================================================================================================

class Tester_MapBitArrayOverlayNumerical : public MapBitArrayOverlay {
public:
    Tester_MapBitArrayOverlayNumerical () : MapBitArrayOverlay(4, 4, 12) {}

    void reset_all () {
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < width(); ++x) {
                set_uint(x, y, 0u);
            }
        }
    }

    bool all_other_zero (u16 x0, u16 y0) const {
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < width(); ++x) {
                if (x == x0 && y == y0) {
                    continue;
                }
                if (get_uint(x, y) != 0u) {
                    return false;
                }
            }
        }
        return true;
    }

    void test_uint_roundtrip () {
        const u32 vmax = max_val();
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < width(); ++x) {
                for (u32 val = 0; val <= vmax; ++val) {
                    reset_all();
                    set_uint(x, y, val);
                    const u32 got = get_uint(x, y);
                    if (got != val) {
                        std::printf("*** FAILED uint roundtrip at (%u,%u) val %u: got %u\n",
                            static_cast<unsigned>(x),
                            static_cast<unsigned>(y),
                            val,
                            got);
                        return;
                    }
                    if (!all_other_zero(x, y)) {
                        std::printf("*** FAILED uint roundtrip at (%u,%u) val %u: other tile non-zero\n",
                            static_cast<unsigned>(x),
                            static_cast<unsigned>(y),
                            val);
                        return;
                    }
                }
            }
        }
        std::printf("uint roundtrip: bpv %u max_val %u tiles %u\n",
            static_cast<unsigned>(bits_per_val()),
            vmax,
            static_cast<unsigned>(width() * height()));
        std::printf("bytes allocated: %u\n", byte_count());
        std::printf("*** PASSED uint roundtrip\n");
    }
};

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    Tester_MapBitArrayOverlayNumerical tester;
    tester.test_uint_roundtrip();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
