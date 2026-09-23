//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "eval_paths.h"

#include <cstdio>
#include <cstring>

#include <dirent.h>

//================================================================================================================================
//=> - EvalPaths -
//================================================================================================================================

EvalPaths::EvalPaths () : m_seed(0), m_players(0), m_ok(false), m_save_turn_n(0) {
    m_out[0] = 0;
    m_saves[0] = 0;
    m_trace[0] = 0;
    for (u16 i = 0; i < SAVE_TURN_MAX; ++i) {
        m_save_turns[i] = 0;
    }
}

bool EvalPaths::load (cstr path) {
    m_ok = false;
    m_seed = 0;
    m_players = 0;
    m_out[0] = 0;
    m_saves[0] = 0;
    m_trace[0] = 0;
    m_save_turn_n = 0;
    for (u16 i = 0; i < SAVE_TURN_MAX; ++i) {
        m_save_turns[i] = 0;
    }
    if (path == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "r");
    if (fp == nullptr) {
        std::printf("EvalPaths: cannot open %s\n", path);
        return false;
    }
    char line[1024];
    while (std::fgets(line, sizeof(line), fp) != nullptr) {
        char* p = line;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }
        if (*p == 0 || *p == '\n' || *p == '#') {
            continue;
        }
        char key[64];
        char val[512];
        if (std::sscanf(p, "%63s == %511[^\n]", key, val) != 2) {
            std::printf("EvalPaths: bad line in %s: %s", path, line);
            std::fclose(fp);
            return false;
        }
        char* v = val;
        while (*v == ' ' || *v == '\t') {
            ++v;
        }
        u32 n = static_cast<u32>(std::strlen(v));
        while (n > 0 && (v[n - 1] == ' ' || v[n - 1] == '\t' || v[n - 1] == '\r')) {
            v[--n] = 0;
        }
        if (std::strcmp(key, "OUT_ROOT") == 0) {
            std::snprintf(m_out, sizeof(m_out), "%s", v);
        } else if (std::strcmp(key, "SAVES_ROOT") == 0) {
            std::snprintf(m_saves, sizeof(m_saves), "%s", v);
        } else if (std::strcmp(key, "TRACE_PATH") == 0) {
            std::snprintf(m_trace, sizeof(m_trace), "%s", v);
        } else if (std::strcmp(key, "SEED") == 0) {
            unsigned long s = 0;
            if (std::sscanf(v, "%lu", &s) != 1 || s == 0) {
                std::printf("EvalPaths: bad SEED '%s'\n", v);
                std::fclose(fp);
                return false;
            }
            m_seed = static_cast<u32>(s);
        } else if (std::strcmp(key, "PLAYERS") == 0) {
            unsigned long pl = 0;
            if (std::sscanf(v, "%lu", &pl) != 1 || pl == 0 || pl > 0xfffful) {
                std::printf("EvalPaths: bad PLAYERS '%s'\n", v);
                std::fclose(fp);
                return false;
            }
            m_players = static_cast<u16>(pl);
        } else {
            std::printf("EvalPaths: unknown key %s\n", key);
            std::fclose(fp);
            return false;
        }
    }
    std::fclose(fp);
    if (m_out[0] == 0 || m_saves[0] == 0 || m_trace[0] == 0 || m_seed == 0 || m_players == 0) {
        std::printf("EvalPaths: missing OUT_ROOT/SAVES_ROOT/TRACE_PATH/SEED/PLAYERS in %s\n", path);
        return false;
    }
    m_ok = true;
    return true;
}

bool EvalPaths::ok () const {
    return m_ok;
}

cstr EvalPaths::out_root () const {
    return m_out;
}

cstr EvalPaths::saves_root () const {
    return m_saves;
}

cstr EvalPaths::trace_path () const {
    return m_trace;
}

u32 EvalPaths::seed () const {
    return m_seed;
}

u16 EvalPaths::players () const {
    return m_players;
}

bool EvalPaths::eval_dir (char* buf, u32 cap) const {
    if (!m_ok || buf == nullptr || cap == 0) {
        return false;
    }
    return std::snprintf(buf, cap, "%s/game-eval", m_out) > 0;
}

bool EvalPaths::data_dir (cstr name, char* buf, u32 cap) const {
    if (!m_ok || name == nullptr || buf == nullptr || cap == 0 || name[0] == 0) {
        return false;
    }
    return std::snprintf(buf, cap, "%s/game-eval/data/%s", m_out, name) > 0;
}

bool EvalPaths::fill (char* buf, u32 cap, cstr suffix, u32 turn) const {
    if (!m_ok || buf == nullptr || suffix == nullptr || cap == 0) {
        return false;
    }
    return std::snprintf(buf, cap, "%s/game-loop-seed-%u-p%u-%04u%s",
        m_saves, m_seed, m_players, turn, suffix) > 0;
}

bool EvalPaths::map_path (u32 turn, char* buf, u32 cap) const {
    return fill(buf, cap, ".bin", turn);
}

bool EvalPaths::units_path (u32 turn, char* buf, u32 cap) const {
    return fill(buf, cap, "-units.bin", turn);
}

bool EvalPaths::cities_path (u32 turn, char* buf, u32 cap) const {
    return fill(buf, cap, "-cities.bin", turn);
}

bool EvalPaths::players_path (u32 turn, char* buf, u32 cap) const {
    return fill(buf, cap, "-players.bin", turn);
}

bool EvalPaths::quartet_ok (u32 turn) const {
    char map_p[512];
    char units_p[512];
    char cities_p[512];
    char players_p[512];
    if (!map_path(turn, map_p, sizeof(map_p))
        || !units_path(turn, units_p, sizeof(units_p))
        || !cities_path(turn, cities_p, sizeof(cities_p))
        || !players_path(turn, players_p, sizeof(players_p))) {
        return false;
    }
    std::FILE* fp = std::fopen(map_p, "rb");
    if (fp == nullptr) {
        return false;
    }
    std::fclose(fp);
    fp = std::fopen(units_p, "rb");
    if (fp == nullptr) {
        return false;
    }
    std::fclose(fp);
    fp = std::fopen(cities_p, "rb");
    if (fp == nullptr) {
        return false;
    }
    std::fclose(fp);
    fp = std::fopen(players_p, "rb");
    if (fp == nullptr) {
        return false;
    }
    std::fclose(fp);
    return true;
}

bool EvalPaths::scan_saves () {
    m_save_turn_n = 0;
    if (!m_ok) {
        return false;
    }
    DIR* d = ::opendir(m_saves);
    if (d == nullptr) {
        return false;
    }
    while (const dirent* e = ::readdir(d)) {
        if (e->d_name[0] == '.') {
            continue;
        }
        unsigned seed = 0;
        unsigned pn = 0;
        unsigned turn = 0;
        if (std::sscanf(e->d_name, "game-loop-seed-%u-p%u-%u.bin", &seed, &pn, &turn) != 3) {
            continue;
        }
        if (seed != m_seed || pn != m_players) {
            continue;
        }
        char expect[256];
        if (std::snprintf(expect, sizeof(expect), "game-loop-seed-%u-p%u-%04u.bin", seed, pn, turn) <= 0
            || std::strcmp(e->d_name, expect) != 0) {
            continue;
        }
        if (!quartet_ok(turn)) {
            continue;
        }
        if (m_save_turn_n >= SAVE_TURN_MAX) {
            ::closedir(d);
            return false;
        }
        m_save_turns[m_save_turn_n] = turn;
        m_save_turn_n = static_cast<u16>(m_save_turn_n + 1u);
    }
    ::closedir(d);
    for (u16 i = 1; i < m_save_turn_n; ++i) {
        const u32 key = m_save_turns[i];
        u16 j = i;
        while (j > 0 && m_save_turns[j - 1u] > key) {
            m_save_turns[j] = m_save_turns[j - 1u];
            j = static_cast<u16>(j - 1u);
        }
        m_save_turns[j] = key;
    }
    return m_save_turn_n != 0;
}

u16 EvalPaths::save_turn_n () const {
    return m_save_turn_n;
}

u32 EvalPaths::save_turn_at (u16 i) const {
    if (i >= m_save_turn_n) {
        return 0;
    }
    return m_save_turns[i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
