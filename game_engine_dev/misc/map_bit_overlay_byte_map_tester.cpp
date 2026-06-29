//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "map_bit_overlay.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Tester_MapBitOverlayByteMap -
//================================================================================================================================

class Tester_MapBitOverlayByteMap : public MapBitOverlay {
public:
    Tester_MapBitOverlayByteMap () : MapBitOverlay(95, 95) {}

    static void upd_row (u8 vis[8][8], u32 row, u8 byte_val) {
        for (u32 b = 0; b < 8u; ++b) {
            vis[row][b] = static_cast<u8>((byte_val >> b) & 1u);
        }
    }

    static void print_vis (const u8 vis[8][8], cstr label) {
        static const char* RED = "\033[31m";
        static const char* GREEN = "\033[32m";
        static const char* RESET = "\033[0m";
        std::printf("%s\n", label);
        for (u32 r = 0; r < 8u; ++r) {
            for (u32 c = 0; c < 8u; ++c) {
                if (vis[r][c]) {
                    std::printf("%s1%s", GREEN, RESET);
                } else {
                    std::printf("%s0%s", RED, RESET);
                }
            }
            std::printf("\n");
        }
    }

    void test_byte_mapping () {
        const u32 nbytes = byte_count();
        u8* ref = new u8[nbytes];
        u8 vis[8][8];
        std::memset(ref, 0, static_cast<size_t>(nbytes));
        u32 byte_idx = 0;
        u32 cycle = 0;
        bool pr_first = false;
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < width(); ++x) {
                const u32 i = tidx(x, y);
                byte_idx = i / 8u;
                cycle = i % 8u;
                if (cycle == 0u) {
                    std::memset(vis, 0, sizeof(vis));
                }
                const u8 prev = ref[byte_idx];
                set(x, y);
                ref[byte_idx] = static_cast<u8>(ref[byte_idx] | static_cast<u8>(1u << cycle));
                upd_row(vis, cycle, ref[byte_idx]);
                if (rd_byte(byte_idx) != ref[byte_idx]) {
                    std::printf("*** FAILED byte map at (%u,%u): byte %u got %u expected %u\n",
                        static_cast<unsigned>(x),
                        static_cast<unsigned>(y),
                        static_cast<unsigned>(byte_idx),
                        static_cast<unsigned>(rd_byte(byte_idx)),
                        static_cast<unsigned>(ref[byte_idx]));
                    print_vis(vis, "byte map vis (error):");
                    delete[] ref;
                    return;
                }
                if (ref[byte_idx] == prev) {
                    std::printf("*** FAILED byte map at (%u,%u): byte %u unchanged\n",
                        static_cast<unsigned>(x),
                        static_cast<unsigned>(y),
                        static_cast<unsigned>(byte_idx));
                    print_vis(vis, "byte map vis (error):");
                    delete[] ref;
                    return;
                }
                for (u32 j = 0; j < nbytes; ++j) {
                    if (j == byte_idx) {
                        continue;
                    }
                    if (rd_byte(j) != ref[j]) {
                        std::printf("*** FAILED byte map at (%u,%u): byte %u changed to %u expected %u\n",
                            static_cast<unsigned>(x),
                            static_cast<unsigned>(y),
                            static_cast<unsigned>(j),
                            static_cast<unsigned>(rd_byte(j)),
                            static_cast<unsigned>(ref[j]));
                        print_vis(vis, "byte map vis (error):");
                        delete[] ref;
                        return;
                    }
                }
                if (cycle == 7u && byte_idx == 0u && !pr_first) {
                    print_vis(vis, "byte map vis (first byte):");
                    pr_first = true;
                }
                if (cycle == 7u && byte_idx == nbytes - 1u) {
                    print_vis(vis, "byte map vis (last byte):");
                }
            }
        }
        std::printf("byte map: tiles %u bytes %u\n", static_cast<unsigned>(width() * height()), static_cast<unsigned>(nbytes));
        std::printf("last byte index: %u cycle bit: %u\n", static_cast<unsigned>(byte_idx), static_cast<unsigned>(cycle));
        std::printf("bytes allocated: %u\n", byte_count());
        std::printf("*** PASSED byte map\n");
        delete[] ref;
    }
};

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    Tester_MapBitOverlayByteMap tester;
    tester.test_byte_mapping();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
