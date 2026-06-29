//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <map>

#include "map_bit_overlay.h"

//================================================================================================================================
//=> - Tester_MapBitOverlay -
//================================================================================================================================

class Tester_MapBitOverlay : public MapBitOverlay {
public:
    Tester_MapBitOverlay () : MapBitOverlay(95, 95) {}

    u32 cnt_set () const {
        u32 n = 0;
        const u16 w = width();
        const u16 h = height();
        for (u16 y = 0; y < h; ++y) {
            for (u16 x = 0; x < w; ++x) {
                n += get(x, y);
            }
        }
        return n;
    }

    void test_set_clr_each_bit () {
        std::map<u32, u32> log;
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < width(); ++x) {
                set(x, y);
                const u32 cnt = cnt_set();
                if (cnt != 1u) {
                    std::printf("*** FAILED set/clr each bit at (%u,%u): set count %u\n",
                        static_cast<unsigned>(x),
                        static_cast<unsigned>(y),
                        cnt);
                    return;
                }
                ++log[tidx(x, y)];
                clr(x, y);
            }
        }
        std::map<u32, u32> coc;
        for (std::map<u32, u32>::const_iterator it = log.begin(); it != log.end(); ++it) {
            ++coc[it->second];
        }
        std::printf("set/clr each bit: log entries %zu\n", log.size());
        std::printf("bytes allocated: %u\n", byte_count());
        std::printf("count-of-counts entries: %zu\n", coc.size());
        for (std::map<u32, u32>::const_iterator it = coc.begin(); it != coc.end(); ++it) {
            std::printf("  count %u: %u indices\n",
                static_cast<unsigned>(it->first),
                static_cast<unsigned>(it->second));
        }
        std::printf("*** PASSED set/clr each bit\n");
    }
};

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    Tester_MapBitOverlay tester;
    tester.test_set_clr_each_bit();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
