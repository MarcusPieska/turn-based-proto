//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "map_bit_array_overlay.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Tester_MapBitArrayOverlayByteMap -
//================================================================================================================================

class Tester_MapBitArrayOverlayByteMap : public MapBitArrayOverlay {
public:
    Tester_MapBitArrayOverlayByteMap () : MapBitArrayOverlay(95, 95, 3) {}

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
        const u32 bpv = static_cast<u32>(bits_per_val());
        const u32 nbytes = byte_count();
        const u32 n = static_cast<u32>(width()) * static_cast<u32>(height());
        u8* ref = new u8[nbytes];
        u8 vis[8][8];
        std::memset(ref, 0, static_cast<size_t>(nbytes));
        u32 byte_idx = 0;
        u32 cycle = 0;
        u32 gbit = 0;
        bool pr_first = false;
        for (u16 y = 0; y < height(); ++y) {
            for (u16 x = 0; x < width(); ++x) {
                const u32 i = static_cast<u32>(y) * static_cast<u32>(width()) + static_cast<u32>(x);
                const u32 bit_off = i * bpv;
                for (u32 vb = 0; vb < bpv; ++vb) {
                    gbit = bit_off + vb;
                    byte_idx = gbit / 8u;
                    cycle = gbit % 8u;
                    if (cycle == 0u) {
                        std::memset(vis, 0, sizeof(vis));
                    }
                    const u8 prev = ref[byte_idx];
                    const u32 val = get(x, y) | (1u << vb);
                    set(x, y, val);
                    ref[byte_idx] = static_cast<u8>(ref[byte_idx] | static_cast<u8>(1u << cycle));
                    upd_row(vis, cycle, ref[byte_idx]);
                    if (get(x, y) != val) {
                        std::printf("*** FAILED byte map at (%u,%u) vb %u: tile val %u expected %u\n",
                            static_cast<unsigned>(x),
                            static_cast<unsigned>(y),
                            static_cast<unsigned>(vb),
                            get(x, y),
                            val);
                        print_vis(vis, "byte map vis (error):");
                        delete[] ref;
                        return;
                    }
                    if (rd_byte(byte_idx) != ref[byte_idx]) {
                        std::printf("*** FAILED byte map at (%u,%u) vb %u: byte %u got %u expected %u\n",
                            static_cast<unsigned>(x),
                            static_cast<unsigned>(y),
                            static_cast<unsigned>(vb),
                            static_cast<unsigned>(byte_idx),
                            static_cast<unsigned>(rd_byte(byte_idx)),
                            static_cast<unsigned>(ref[byte_idx]));
                        print_vis(vis, "byte map vis (error):");
                        delete[] ref;
                        return;
                    }
                    if (ref[byte_idx] == prev) {
                        std::printf("*** FAILED byte map at (%u,%u) vb %u: byte %u unchanged\n",
                            static_cast<unsigned>(x),
                            static_cast<unsigned>(y),
                            static_cast<unsigned>(vb),
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
                            std::printf("*** FAILED byte map at (%u,%u) vb %u: byte %u changed to %u expected %u\n",
                                static_cast<unsigned>(x),
                                static_cast<unsigned>(y),
                                static_cast<unsigned>(vb),
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
        }
        std::printf("byte map: bpv %u tiles %u storage bits %u bytes %u\n",
            static_cast<unsigned>(bpv),
            static_cast<unsigned>(n),
            static_cast<unsigned>(n * bpv),
            static_cast<unsigned>(nbytes));
        std::printf("last gbit %u byte %u cycle %u\n",
            static_cast<unsigned>(gbit),
            static_cast<unsigned>(byte_idx),
            static_cast<unsigned>(cycle));
        std::printf("bytes allocated: %u\n", byte_count());
        std::printf("*** PASSED byte map\n");
        delete[] ref;
    }
};

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    Tester_MapBitArrayOverlayByteMap tester;
    tester.test_byte_mapping();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
