//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "factory_game_array_simple.h"
#include "game_state.h"
#include "unit_chain_validation.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"

//================================================================================================================================
//=> - Meta logging -
//================================================================================================================================

static int g_fail = 0;
static int g_pass = 0;

static void meta (cstr msg) {
    std::printf("[meta] %s\n", msg);
}

static void meta_f (cstr fmt, u32 a) {
    std::printf("[meta] ");
    std::printf(fmt, a);
    std::printf("\n");
}

static void meta_case (cstr name) {
    std::printf("\n");
    std::printf("================================================================================\n");
    std::printf("[meta] SPOOF CASE: %s\n", name);
    std::printf("================================================================================\n");
}

static void chain_begin (cstr expect) {
    std::printf("[meta] >>> calling TestHlpUnitChainValidation (expect %s)\n", expect);
    std::printf("[chain] ---------- begin ----------\n");
}

static void chain_end () {
    std::printf("[chain] ---------- end ----------\n");
}

//================================================================================================================================
//=> - Toy world -
//================================================================================================================================

struct ToyKeys {
    UnitAddKey solo;
    UnitAddKey head;
    UnitAddKey tail;
};

static bool reset_world (GameState& s) {
    s.clear();
    return Factory_GameArraySimple::init_test_grid(&s.m_map, 4u, 4u);
}

static UnitAddKey mk_unit (GameState& s) {
    const UnitAddKey k = s.m_units.get_next_new_unit_add_key();
    UnitAddStruct* u = s.m_units.get_unit_add(k);
    if (u == nullptr) {
        return UnitAddKey::None();
    }
    u->m_x = U16_KEY_NULL;
    u->m_y = U16_KEY_NULL;
    u->m_unit_typ_idx = 0;
    u->m_next_unit_on_tile = U16_KEY_NULL;
    u->m_next_unit_in_group = U16_KEY_NULL;
    u->m_mvt_points = 0;
    u->m_player_idx = 0;
    u->m_health = UNIT_HEALTH;
    u->m_level = 0;
    u->m_misc = 0;
    return k;
}

static void put_on_map (GameState& s, UnitAddKey k, u16 x, u16 y) {
    UnitAddStruct* u = s.m_units.get_unit_add(k);
    if (u == nullptr) {
        return;
    }
    u->m_x = x;
    u->m_y = y;
    u->m_next_unit_on_tile = s.m_map.get_unit_hd(x, y);
    s.m_map.set_unit_hd(x, y, k.value());
}

static bool build_clean (GameState& s, ToyKeys* k) {
    if (k == nullptr || !reset_world(s)) {
        return false;
    }
    k->solo = mk_unit(s);
    k->head = mk_unit(s);
    k->tail = mk_unit(s);
    if (!k->solo.is_valid() || !k->head.is_valid() || !k->tail.is_valid()) {
        return false;
    }
    put_on_map(s, k->solo, 0u, 0u);
    put_on_map(s, k->head, 1u, 0u);
    UnitAddStruct* t = s.m_units.get_unit_add(k->tail);
    UnitAddStruct* h = s.m_units.get_unit_add(k->head);
    if (t == nullptr || h == nullptr) {
        return false;
    }
    t->m_x = U16_KEY_NULL;
    t->m_y = U16_KEY_NULL;
    t->m_next_unit_on_tile = U16_KEY_NULL;
    t->m_next_unit_in_group = U16_KEY_NULL;
    h->m_next_unit_in_group = k->tail.value();
    return true;
}

static bool expect_pass (GameState& s, cstr label) {
    meta(label);
    chain_begin("PASS");
    UnitChainScan scan;
    const bool ok = TestHlpUnitChainValidation::run(s, false, &scan);
    chain_end();
    if (ok && scan.err_n == 0u) {
        meta("validator PASS (as expected)");
        return true;
    }
    meta("validator did NOT pass — meta FAIL");
    g_fail++;
    return false;
}

static bool expect_fail (GameState& s, cstr label) {
    meta(label);
    chain_begin("FAIL");
    UnitChainScan scan;
    const bool ok = TestHlpUnitChainValidation::run(s, false, &scan);
    chain_end();
    if (!ok && scan.err_n > 0u) {
        meta_f("validator FAIL err_n=%u (as expected)", scan.err_n);
        g_pass++;
        return true;
    }
    meta("validator did NOT fail — meta FAIL");
    g_fail++;
    return false;
}

static bool run_case (GameState& s, cstr name, bool (*inject)(GameState&, ToyKeys&)) {
    meta_case(name);
    ToyKeys k;
    if (!build_clean(s, &k)) {
        meta("could not build clean baseline");
        g_fail++;
        return false;
    }
    if (!expect_pass(s, "baseline before inject")) {
        return false;
    }
    meta("injecting spoof...");
    if (!inject(s, k)) {
        meta("inject helper failed");
        g_fail++;
        return false;
    }
    return expect_fail(s, "after inject");
}

//================================================================================================================================
//=> - Injectors (one per detectable fault family) -
//================================================================================================================================

static bool inj_mixed_coords (GameState& s, ToyKeys& k) {
    meta("spoof: unit with mixed null/non-null coordinates");
    UnitAddStruct* u = s.m_units.get_unit_add(k.solo);
    if (u == nullptr) {
        return false;
    }
    u->m_x = 0u;
    u->m_y = U16_KEY_NULL;
    return true;
}

static bool inj_stack_key_oor (GameState& s, ToyKeys& k) {
    (void)k;
    meta("spoof: map stack head key out of range (>= unit pool head)");
    const u16 bad = static_cast<u16>(s.m_units.get_head_unit_add_idx() + 10u);
    s.m_map.set_unit_hd(2u, 2u, bad);
    return true;
}

static bool inj_unique_map_head (GameState& s, ToyKeys& k) {
    meta("spoof: same unit head registered on two map tiles");
    s.m_map.set_unit_hd(2u, 0u, k.solo.value());
    return true;
}

static bool inj_stack_cycle (GameState& s, ToyKeys& k) {
    meta("spoof: cycle in m_next_unit_on_tile on one tile");
    UnitAddStruct* a = s.m_units.get_unit_add(k.solo);
    UnitAddKey b = mk_unit(s);
    UnitAddStruct* bu = s.m_units.get_unit_add(b);
    if (a == nullptr || bu == nullptr) {
        return false;
    }
    s.m_map.set_unit_hd(0u, 0u, k.solo.value());
    a->m_x = 0u;
    a->m_y = 0u;
    a->m_next_unit_on_tile = b.value();
    bu->m_x = 0u;
    bu->m_y = 0u;
    bu->m_next_unit_on_tile = k.solo.value();
    return true;
}

static bool inj_dead_stack (GameState& s, ToyKeys& k) {
    meta("spoof: map stack points at a recycled (dead) unit");
    const u16 raw = k.solo.value();
    s.m_map.set_unit_hd(0u, 0u, raw);
    s.m_units.return_unit_add(k.solo);
    k.solo = UnitAddKey::None();
    return true;
}

static bool inj_stack_xy_mismatch (GameState& s, ToyKeys& k) {
    meta("spoof: stack unit xy does not match its map tile");
    UnitAddStruct* u = s.m_units.get_unit_add(k.solo);
    if (u == nullptr) {
        return false;
    }
    u->m_x = 3u;
    u->m_y = 3u;
    return true;
}

static bool inj_group_key_oor (GameState& s, ToyKeys& k) {
    meta("spoof: group chain key out of range");
    UnitAddStruct* h = s.m_units.get_unit_add(k.head);
    if (h == nullptr) {
        return false;
    }
    h->m_next_unit_in_group = static_cast<u16>(s.m_units.get_head_unit_add_idx() + 10u);
    return true;
}

static bool inj_group_cycle (GameState& s, ToyKeys& k) {
    meta("spoof: cycle in m_next_unit_in_group");
    UnitAddStruct* h = s.m_units.get_unit_add(k.head);
    UnitAddStruct* t = s.m_units.get_unit_add(k.tail);
    if (h == nullptr || t == nullptr) {
        return false;
    }
    h->m_next_unit_in_group = k.tail.value();
    t->m_next_unit_in_group = k.head.value();
    return true;
}

static bool inj_dup_group (GameState& s, ToyKeys& k) {
    meta("spoof: same tail appears in two group chains");
    UnitAddKey h2 = mk_unit(s);
    if (!h2.is_valid()) {
        return false;
    }
    put_on_map(s, h2, 2u, 0u);
    UnitAddStruct* a = s.m_units.get_unit_add(k.head);
    UnitAddStruct* b = s.m_units.get_unit_add(h2);
    if (a == nullptr || b == nullptr) {
        return false;
    }
    a->m_next_unit_in_group = k.tail.value();
    b->m_next_unit_in_group = k.tail.value();
    return true;
}

static bool inj_dead_group (GameState& s, ToyKeys& k) {
    meta("spoof: group chain points at a recycled (dead) unit");
    UnitAddStruct* h = s.m_units.get_unit_add(k.head);
    if (h == nullptr) {
        return false;
    }
    h->m_next_unit_in_group = k.tail.value();
    s.m_units.return_unit_add(k.tail);
    k.tail = UnitAddKey::None();
    return true;
}

static bool inj_head_in_tail (GameState& s, ToyKeys& k) {
    meta("spoof: positioned unit appears as a group follower (head in tail)");
    UnitAddStruct* h = s.m_units.get_unit_add(k.head);
    UnitAddStruct* t = s.m_units.get_unit_add(k.tail);
    if (h == nullptr || t == nullptr) {
        return false;
    }
    t->m_x = 1u;
    t->m_y = 0u;
    t->m_next_unit_on_tile = U16_KEY_NULL;
    h->m_next_unit_in_group = k.tail.value();
    return true;
}

static bool inj_tail_on_stack (GameState& s, ToyKeys& k) {
    meta("spoof: stack unit also linked as another head's group follower");
    UnitAddStruct* h = s.m_units.get_unit_add(k.head);
    if (h == nullptr) {
        return false;
    }
    h->m_next_unit_in_group = k.solo.value();
    s.m_units.return_unit_add(k.tail);
    k.tail = UnitAddKey::None();
    return true;
}

static bool inj_pos_missing (GameState& s, ToyKeys& k) {
    meta("spoof: positioned unit missing from all map stacks");
    s.m_map.set_unit_hd(0u, 0u, U16_KEY_NULL);
    UnitAddStruct* u = s.m_units.get_unit_add(k.solo);
    if (u == nullptr) {
        return false;
    }
    u->m_next_unit_on_tile = U16_KEY_NULL;
    return true;
}

static bool inj_orphan_tail (GameState& s, ToyKeys& k) {
    meta("spoof: tail unit not reached by any group chain");
    UnitAddStruct* h = s.m_units.get_unit_add(k.head);
    if (h == nullptr) {
        return false;
    }
    h->m_next_unit_in_group = U16_KEY_NULL;
    return true;
}

static bool inj_tail_as_map_hd (GameState& s, ToyKeys& k) {
    meta("spoof: null-xy tail installed as a map stack head");
    s.m_map.set_unit_hd(2u, 2u, k.tail.value());
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    meta("unit_chain_validation meta-tester");
    meta("prints: [meta]=this driver, [chain]=TestHlpUnitChainValidation");

    GameState s;
    bool all = true;
    all = run_case(s, "mixed coordinates", inj_mixed_coords) && all;
    all = run_case(s, "map stack key out of range", inj_stack_key_oor) && all;
    all = run_case(s, "unique map head (unit on two tiles)", inj_unique_map_head) && all;
    all = run_case(s, "cycle in tile stack", inj_stack_cycle) && all;
    all = run_case(s, "map stack points at dead unit", inj_dead_stack) && all;
    all = run_case(s, "stack xy mismatch vs tile", inj_stack_xy_mismatch) && all;
    all = run_case(s, "group chain key out of range", inj_group_key_oor) && all;
    all = run_case(s, "cycle in group chain", inj_group_cycle) && all;
    all = run_case(s, "unit in two group chains", inj_dup_group) && all;
    all = run_case(s, "group chain points at dead unit", inj_dead_group) && all;
    all = run_case(s, "head in tail (positioned follower)", inj_head_in_tail) && all;
    all = run_case(s, "stack unit also linked as another group's follower", inj_tail_on_stack) && all;
    all = run_case(s, "positioned unit missing from map", inj_pos_missing) && all;
    all = run_case(s, "orphan tail (not in any group)", inj_orphan_tail) && all;
    all = run_case(s, "tail appears as map stack head", inj_tail_as_map_hd) && all;

    std::printf("\n");
    meta_f("cases detected as expected: %u", (u32)g_pass);
    meta_f("meta failures: %u", (u32)g_fail);
    if (all && g_fail == 0) {
        meta("*** PASSED unit_chain_validation meta-tester");
        return 0;
    }
    meta("*** FAILED unit_chain_validation meta-tester");
    return 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
