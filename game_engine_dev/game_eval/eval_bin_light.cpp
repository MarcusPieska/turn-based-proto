//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "eval_bin.h"

#include <cstdio>

#include "eval_paths.h"

//================================================================================================================================
//=> - EvalBin light (no GameIo; for log-only drivers) -
//================================================================================================================================

EvalBin::EvalBin () : m_snaps(nullptr), m_n(0), m_cap(0) {
}

EvalBin::~EvalBin () {
    clr();
}

void EvalBin::clr () {
    m_snaps = nullptr;
    m_n = 0;
    m_cap = 0;
}

bool EvalBin::file_ok (cstr path) const {
    if (path == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    std::fclose(fp);
    return true;
}

bool EvalBin::load_turn (const EvalPaths& paths, u32 turn) {
    (void)paths;
    (void)turn;
    std::printf("EvalBin light: load_turn requires full eval_bin.cpp / GameIo link\n");
    return false;
}

u16 EvalBin::snap_n () const {
    return 0;
}

u32 EvalBin::turn_at (u16 i) const {
    (void)i;
    return 0;
}

u16 EvalBin::snap_i (u32 turn) const {
    (void)turn;
    return U16_KEY_NULL;
}

const GameArraySimple* EvalBin::map (u16 i) const {
    (void)i;
    return nullptr;
}

const UnitAddVector* EvalBin::units (u16 i) const {
    (void)i;
    return nullptr;
}

const CityArray* EvalBin::cities (u16 i) const {
    (void)i;
    return nullptr;
}

const PlayerState* EvalBin::seats (u16 i) const {
    (void)i;
    return nullptr;
}

u16 EvalBin::seat_n (u16 i) const {
    (void)i;
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
