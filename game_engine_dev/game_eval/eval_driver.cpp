//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "eval_driver.h"

#include <cstdio>

//================================================================================================================================
//=> - EvalDriver -
//================================================================================================================================

EvalDriver::EvalDriver (const EvalNeed& need) : m_need(need) {
}

EvalDriver::~EvalDriver () {
}

const EvalNeed& EvalDriver::need () const {
    return m_need;
}

EvalPaths& EvalDriver::paths () {
    return m_paths;
}

EvalLog& EvalDriver::log () {
    return m_log;
}

EvalBin& EvalDriver::bin () {
    return m_bin;
}

bool EvalDriver::chk () {
    if (m_need.want_trace ()) {
        if (!m_log.load(m_paths.trace_path())) {
            std::printf("EvalNeed: cannot load TRACE_PATH %s\n", m_paths.trace_path());
            return false;
        }
    }
    if (m_need.want_logs ()) {
        if (!m_need.want_trace ()) {
            std::printf("EvalNeed: log mask requires a loaded trace\n");
            return false;
        }
        const u8* want = m_need.logs().bytes();
        for (u16 i = 0; i < LogNeedMask::n(); ++i) {
            if (want[i] == 0) {
                continue;
            }
            const u32 have = m_log.count_i(i);
            if (have < static_cast<u32>(want[i])) {
                std::printf("EvalNeed: missing log %s (have %u, need >= %u)\n",
                    LogNeedMask::nm(i), have, static_cast<u32>(want[i]));
                return false;
            }
        }
    }
    if (m_need.want_save_seq ()) {
        if (!m_paths.scan_saves () || m_paths.save_turn_n () < 2) {
            std::printf("EvalNeed: save_seq requires >= 2 complete save quartets (have %u)\n",
                static_cast<u32>(m_paths.save_turn_n()));
            return false;
        }
    }
    if (m_need.want_saves ()) {
        m_bin.clr();
        for (u16 i = 0; i < m_need.save_n(); ++i) {
            const u32 turn = m_need.save_turn(i);
            char map_p[512];
            char units_p[512];
            char cities_p[512];
            char players_p[512];
            if (!m_paths.map_path(turn, map_p, sizeof(map_p))
                || !m_paths.units_path(turn, units_p, sizeof(units_p))
                || !m_paths.cities_path(turn, cities_p, sizeof(cities_p))
                || !m_paths.players_path(turn, players_p, sizeof(players_p))) {
                std::printf("EvalNeed: cannot build save paths turn=%u\n", turn);
                return false;
            }
            if (!m_bin.file_ok(map_p) || !m_bin.file_ok(units_p)
                || !m_bin.file_ok(cities_p) || !m_bin.file_ok(players_p)) {
                std::printf("EvalNeed: incomplete save quartet turn=%u\n", turn);
                std::printf("  map=%s\n  units=%s\n  cities=%s\n  players=%s\n",
                    map_p, units_p, cities_p, players_p);
                return false;
            }
            if (!m_bin.load_turn(m_paths, turn)) {
                std::printf("EvalNeed: full unpack failed turn=%u\n", turn);
                return false;
            }
        }
    }
    return true;
}

int EvalDriver::go (cstr paths_file) {
    if (!m_paths.load(paths_file)) {
        return 1;
    }
    if (!chk()) {
        return 1;
    }
    return run();
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
