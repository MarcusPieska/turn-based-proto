//================================================================================================================================
//= Includes =
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "eval_driver.h"
#include "eval_need.h"
#include "game_io.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"

//================================================================================================================================
//= Paths =
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const u16 k_key_cap = 32u;

//================================================================================================================================
//= Dump =
//================================================================================================================================

static void wr_u (std::FILE* fp, u32 turn, u16 key, const UnitAddStruct* u) {
    if (u == nullptr) {
        std::fprintf(fp, "t=%u key=%u missing\n", turn, static_cast<u32>(key));
        return;
    }
    std::fprintf(
        fp,
        "t=%u key=%u xy=(%u,%u) seat=%u typ=%u size=%u hp=%u lvl=%u mp=%d camp=%u army=%u ext=%u nxt_grp=%u nxt_tile=%u misc=%u dx=%d dy=%d\n",
        turn,
        static_cast<u32>(key),
        static_cast<u32>(u->m_x),
        static_cast<u32>(u->m_y),
        static_cast<u32>(u->m_player_idx),
        static_cast<u32>(u->m_unit_typ_idx),
        static_cast<u32>(u->m_unit_size),
        static_cast<u32>(u->m_health),
        static_cast<u32>(u->m_level),
        static_cast<int>(u->m_mvt_points),
        static_cast<u32>(u->m_in_campaign),
        static_cast<u32>(u->m_in_army),
        static_cast<u32>(u->m_being_extended),
        static_cast<u32>(u->m_next_unit_in_group),
        static_cast<u32>(u->m_next_unit_on_tile),
        static_cast<u32>(u->m_misc),
        static_cast<int>(u->m_delta_x_dest),
        static_cast<int>(u->m_delta_y_dest));
}

//================================================================================================================================
//= UnitSaveDumpTester =
//================================================================================================================================

class UnitSaveDumpTester : public EvalDriver {
public:
    UnitSaveDumpTester (const u16* keys, u16 n);

protected:
    int run () override;

private:
    u16 m_key[k_key_cap];
    u16 m_n;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

UnitSaveDumpTester::UnitSaveDumpTester (const u16* keys, u16 n) : EvalDriver(make_need()), m_n(0) {
    if (keys == nullptr || n == 0u) {
        return;
    }
    if (n > k_key_cap) {
        n = k_key_cap;
    }
    for (u16 i = 0; i < n; ++i) {
        m_key[i] = keys[i];
    }
    m_n = n;
}

int UnitSaveDumpTester::run () {
    if (m_n == 0u) {
        std::printf("unit_save_dump: need unit key args\n");
        return 1;
    }
    const u16 save_n = paths().save_turn_n();
    std::printf("unit_save_dump keys=%u saves=%u\n", static_cast<u32>(m_n), static_cast<u32>(save_n));
    for (u16 si = 0; si < save_n; ++si) {
        const u32 turn = paths().save_turn_at(si);
        char units_p[512];
        if (!paths().units_path(turn, units_p, sizeof(units_p))) {
            std::printf("unit_save_dump: units_path fail turn=%u\n", turn);
            return 1;
        }
        UnitAddVector units;
        if (!GameIo::load_units(units_p, units)) {
            std::printf("unit_save_dump: load fail %s\n", units_p);
            return 1;
        }
        const u16 head = units.get_head_unit_add_idx();
        for (u16 i = 0; i < m_n; ++i) {
            const u16 key = m_key[i];
            const UnitAddStruct* u = (key < head) ? units.get_unit_add(UnitAddKey::from_raw(key)) : nullptr;
            wr_u(stdout, turn, key, u);
        }
    }
    return 0;
}

//================================================================================================================================
//= Main =
//================================================================================================================================

int main (int argc, char** argv) {
    u16 keys[k_key_cap];
    u16 n = 0;
    for (int i = 1; i < argc && n < k_key_cap; ++i) {
        char* end = nullptr;
        const unsigned long v = std::strtoul(argv[i], &end, 10);
        if (end == argv[i] || *end != 0 || v > 65535ul) {
            std::printf("unit_save_dump: bad key '%s'\n", argv[i]);
            return 1;
        }
        keys[n++] = static_cast<u16>(v);
    }
    if (n == 0u) {
        std::printf("usage: unit_save_dump_tester <unit_key> [unit_key...]\n");
        return 1;
    }
    UnitSaveDumpTester t(keys, n);
    return t.go(G_PATHS);
}

//================================================================================================================================
//= End of file =
//================================================================================================================================
