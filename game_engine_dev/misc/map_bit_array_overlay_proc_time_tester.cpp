//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>

#include "map_bit_array_overlay.h"

typedef std::chrono::high_resolution_clock Clock;

//================================================================================================================================
//=> - Tester_MapBitArrayOverlayProcTime -
//================================================================================================================================

class Tester_MapBitArrayOverlayProcTime : public MapBitArrayOverlay {
public:
    Tester_MapBitArrayOverlayProcTime () : MapBitArrayOverlay(20, 20, 7) {}

    u32 run_overlay_rw () {
        const u16 w = width();
        const u32 vmax = max_val();
        u32 acc = 0;
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < w; ++x) {
                const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
                set_uint(x, y, i % (vmax + 1u));
            }
        }
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < w; ++x) {
                acc += get_uint(x, y);
            }
        }
        return acc;
    }

    static u32 run_byte_rw (u16 w, u16 h, u32 vmax) {
        const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
        u8* arr = new u8[n];
        u32 acc = 0;
        for (u16 y = 0; y < h; ++y) {
            for (u16 x = 0; x < w; ++x) {
                const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
                arr[i] = static_cast<u8>(i % (vmax + 1u));
            }
        }
        for (u32 i = 0; i < n; ++i) {
            acc += static_cast<u32>(arr[i]);
        }
        delete[] arr;
        return acc;
    }

    void test_proc_time () {
        const u32 n = static_cast<u32>(width()) * static_cast<u32>(height());
        const Clock::time_point t0 = Clock::now();
        const u32 ov_acc = run_overlay_rw();
        const Clock::time_point t1 = Clock::now();
        const u32 ba_acc = run_byte_rw(width(), height(), max_val());
        const Clock::time_point t2 = Clock::now();
        const double ov_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        const double ba_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
        std::printf("proc time: %ux%u bpv %u max_val %u tiles %u\n",
            static_cast<unsigned>(width()),
            static_cast<unsigned>(height()),
            static_cast<unsigned>(bits_per_val()),
            max_val(),
            static_cast<unsigned>(n));
        std::printf("bit array overlay: %.3f ms checksum %u\n", ov_ms, ov_acc);
        std::printf("byte array:        %.3f ms checksum %u\n", ba_ms, ba_acc);
        if (ba_ms > 0.0) {
            std::printf("ratio overlay/byte: %.3fx\n", ov_ms / ba_ms);
        }
        std::printf("overlay bytes: %u  byte array bytes: %u\n",
            byte_count(),
            n);
        std::printf("*** PASSED proc time\n");
    }
};

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    Tester_MapBitArrayOverlayProcTime tester;
    tester.test_proc_time();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
