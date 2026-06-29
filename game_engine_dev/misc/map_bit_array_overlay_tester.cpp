//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <map>

#include "map_bit_array_overlay.h"

//================================================================================================================================
//=> - Tester_MapBitArrayOverlay -
//================================================================================================================================

class Tester_MapBitArrayOverlay : public MapBitArrayOverlay {
public:
    Tester_MapBitArrayOverlay () : MapBitArrayOverlay(95, 95, 3) {}

    u32 cnt_nz () const {
        u32 n = 0;
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < width(); ++x) {
                if (get(x, y) != 0u) {
                    ++n;
                }
            }
        }
        return n;
    }

    void test_set_clr_each_val_bit () {
        std::map<u32, u32> log;
        const u32 bpv = static_cast<u32>(bits_per_val());
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < width(); ++x) {
                for (u32 vb = 0; vb < bpv; ++vb) {
                    const u32 val = 1u << vb;
                    set(x, y, val);
                    const u32 cnt = cnt_nz();
                    if (cnt != 1u) {
                        std::printf("*** FAILED set/clr each val bit at (%u,%u) vb %u: nz count %u\n",
                            static_cast<unsigned>(x),
                            static_cast<unsigned>(y),
                            static_cast<unsigned>(vb),
                            cnt);
                        return;
                    }
                    if (get(x, y) != val) {
                        std::printf("*** FAILED set/clr each val bit at (%u,%u) vb %u: got %u expected %u\n",
                            static_cast<unsigned>(x),
                            static_cast<unsigned>(y),
                            static_cast<unsigned>(vb),
                            get(x, y),
                            val);
                        return;
                    }
                    ++log[tidx(x, y)];
                    set(x, y, 0u);
                }
            }
        }
        std::map<u32, u32> coc;
        for (std::map<u32, u32>::const_iterator it = log.begin(); it != log.end(); ++it) {
            ++coc[it->second];
        }
        std::printf("set/clr each val bit: bpv %u log entries %zu\n",
            static_cast<unsigned>(bpv),
            log.size());
        std::printf("bytes allocated: %u\n", byte_count());
        std::printf("count-of-counts entries: %zu\n", coc.size());
        for (std::map<u32, u32>::const_iterator it = coc.begin(); it != coc.end(); ++it) {
            std::printf("  count %u: %u indices\n",
                static_cast<unsigned>(it->first),
                static_cast<unsigned>(it->second));
        }
        if (coc.size() != 1u || coc.begin()->first != bpv) {
            std::printf("*** FAILED set/clr each val bit: expected each index %u times\n",
                static_cast<unsigned>(bpv));
            return;
        }
        std::printf("*** PASSED set/clr each val bit\n");
    }
};

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    Tester_MapBitArrayOverlay tester;
    tester.test_set_clr_each_val_bit();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
