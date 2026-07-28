//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "unit_chain_validation.h"
#include "game_array_simple.h"
#include "game_state.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void err_kw (cstr kw) {
    std::printf("\033[31m%s\033[0m", kw);
}

static void err_line (UnitChainScan& r, cstr msg) {
    r.err_n++;
    err_kw("ERROR");
    std::printf(": %s\n", msg);
}

static void ok_line (bool print_all, cstr msg) {
    if (print_all) {
        std::printf("ok: %s\n", msg);
    }
}

static bool has_pos (const UnitAddStruct& u) {
    return u.m_x != U16_KEY_NULL && u.m_y != U16_KEY_NULL;
}

static bool is_tail_xy (const UnitAddStruct& u) {
    return u.m_x == U16_KEY_NULL && u.m_y == U16_KEY_NULL;
}

//================================================================================================================================
//=> - TestHlpUnitChainValidation -
//================================================================================================================================

bool TestHlpUnitChainValidation::run (GameState& s, bool print_all, UnitChainScan* out) {
    UnitChainScan r;
    r.vec_n = 0;
    r.vec_pos = 0;
    r.vec_tail = 0;
    r.map_stack = 0;
    r.map_full = 0;
    r.err_n = 0;

    const u32 scan_n = static_cast<u32>(s.m_units.get_head_unit_add_idx());
    u8* mark_pos = nullptr;
    u8* mark_tail = nullptr;
    u8* seen_stack = nullptr;
    u8* seen_grp = nullptr;
    if (scan_n > 0u) {
        mark_pos = new u8[scan_n]();
        mark_tail = new u8[scan_n]();
        seen_stack = new u8[scan_n]();
        seen_grp = new u8[scan_n]();
    }

    for (u32 idx = 0; idx < scan_n; ++idx) {
        const UnitAddKey key = UnitAddKey::from_raw(static_cast<u16>(idx));
        const UnitAddStruct* u = s.m_units.get_unit_add(key);
        if (u == nullptr) {
            continue;
        }
        r.vec_n++;
        if (has_pos(*u)) {
            r.vec_pos++;
            mark_pos[idx] = 1u;
        } else if (is_tail_xy(*u)) {
            r.vec_tail++;
            mark_tail[idx] = 1u;
        } else {
            err_line(r, "unit has mixed null/non-null coordinates");
            if (print_all) {
                std::printf("  key=%u xy=(%u,%u)\n", idx, (u32)u->m_x, (u32)u->m_y);
            }
        }
    }
    if (print_all) {
        std::printf("vector: live=%u pos=%u tail=%u\n", r.vec_n, r.vec_pos, r.vec_tail);
    }

    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u16 cur_raw = s.m_map.get_unit_hd(x, y);
            if (cur_raw == U16_KEY_NULL) {
                continue;
            }
            while (cur_raw != U16_KEY_NULL) {
                if (cur_raw >= scan_n) {
                    err_line(r, "map stack key out of range");
                    break;
                }
                if (seen_stack[cur_raw] != 0u) {
                    err_kw("ERROR");
                    std::printf(": ");
                    err_kw("unique map head");
                    std::printf(" / ");
                    err_kw("cycle");
                    std::printf("; key=%u already on a tile stack (at %u,%u)\n",
                        (u32)cur_raw, (u32)x, (u32)y);
                    r.err_n++;
                    break;
                }
                seen_stack[cur_raw] = 1u;
                r.map_stack++;

                const UnitAddStruct* hu = s.m_units.get_unit_add(UnitAddKey::from_raw(cur_raw));
                if (hu == nullptr) {
                    err_line(r, "map stack points at dead unit");
                    break;
                }
                if (!has_pos(*hu) || hu->m_x != x || hu->m_y != y) {
                    err_kw("ERROR");
                    std::printf(": stack unit key=%u xy mismatch vs tile (%u,%u)\n",
                        (u32)cur_raw, (u32)x, (u32)y);
                    r.err_n++;
                }

                u16 g_raw = cur_raw;
                bool g_first = true;
                bool g_bad = false;
                while (g_raw != U16_KEY_NULL) {
                    if (g_raw >= scan_n) {
                        err_line(r, "group chain key out of range");
                        g_bad = true;
                        break;
                    }
                    if (seen_grp[g_raw] != 0u) {
                        err_kw("ERROR");
                        std::printf(": ");
                        err_kw("cycle");
                        std::printf(" / duplicate group; key=%u already in a group chain (head=%u)\n",
                            (u32)g_raw, (u32)cur_raw);
                        r.err_n++;
                        g_bad = true;
                        break;
                    }
                    seen_grp[g_raw] = 1u;
                    r.map_full++;

                    const UnitAddStruct* gu = s.m_units.get_unit_add(UnitAddKey::from_raw(g_raw));
                    if (gu == nullptr) {
                        err_line(r, "group chain points at dead unit");
                        g_bad = true;
                        break;
                    }
                    if (!g_first) {
                        if (has_pos(*gu)) {
                            err_kw("ERROR");
                            std::printf(": ");
                            err_kw("head in tail");
                            std::printf("; positioned unit key=%u in group after head=%u\n",
                                (u32)g_raw, (u32)cur_raw);
                            r.err_n++;
                        }
                        if (seen_stack[g_raw] != 0u) {
                            err_kw("ERROR");
                            std::printf(": group tail key=%u also on a tile stack\n", (u32)g_raw);
                            r.err_n++;
                        }
                    }
                    g_first = false;
                    g_raw = gu->m_next_unit_in_group;
                }
                if (g_bad) {
                    break;
                }
                cur_raw = hu->m_next_unit_on_tile;
            }
        }
    }
    if (print_all) {
        std::printf("map: stack=%u full=%u\n", r.map_stack, r.map_full);
    }

    if (r.map_stack == r.vec_pos) {
        ok_line(print_all, "map_stack == vec_pos");
    } else {
        err_kw("ERROR");
        std::printf(": map_stack=%u != vec_pos=%u\n", r.map_stack, r.vec_pos);
        r.err_n++;
    }
    if (r.map_full == r.vec_n) {
        ok_line(print_all, "map_full == vec_n");
    } else {
        err_kw("ERROR");
        std::printf(": map_full=%u != vec_n=%u\n", r.map_full, r.vec_n);
        r.err_n++;
    }

    for (u32 idx = 0; idx < scan_n; ++idx) {
        if (mark_pos[idx] != 0u && seen_stack[idx] == 0u) {
            err_kw("ERROR");
            std::printf(": positioned unit key=%u missing from map stacks\n", idx);
            r.err_n++;
        }
        if (mark_tail[idx] != 0u && seen_grp[idx] == 0u) {
            err_kw("ERROR");
            std::printf(": tail unit key=%u not reached by any group chain\n", idx);
            r.err_n++;
        }
        if (mark_tail[idx] != 0u && seen_stack[idx] != 0u) {
            err_kw("ERROR");
            std::printf(": tail unit key=%u appears as map stack head\n", idx);
            r.err_n++;
        }
    }

    if (print_all || r.err_n > 0u) {
        std::printf("unit_chain_validation err_n=%u %s\n",
            r.err_n, (r.err_n == 0u) ? "PASS" : "FAIL");
    }

    delete[] mark_pos;
    delete[] mark_tail;
    delete[] seen_stack;
    delete[] seen_grp;

    if (out != nullptr) {
        *out = r;
    }
    return r.err_n == 0u;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
