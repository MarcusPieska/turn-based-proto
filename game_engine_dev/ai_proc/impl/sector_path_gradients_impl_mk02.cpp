//================================================================================================================================
//=> - SectorPathGradients mk02 -
//================================================================================================================================
//
//  Retained as exclusive full-map fill (mk03 supersedes shared-layer approach).
//
//================================================================================================================================

#include "generate_distance_p2p.h"
#include "runtime_statics.h"
#include "whiteboard_mng.h"

bool SectorPathGradients::setup (const Whiteboard_2B& sec, u16 sec_n) {
    (void)sec;
    (void)sec_n;
    s_ok = true;
    s_map_w = WhiteboardMng::width();
    s_map_h = WhiteboardMng::height();
    s_tile_n = WhiteboardMng::tile_n();
    return s_ok;
}

void SectorPathGradients::teardown () {
    s_ok = false;
}

bool SectorPathGradients::bind (const Whiteboard_2B* sec) {
    m_sec = sec;
    return true;
}

void SectorPathGradients::clr () {
    for (u16 i = 0; i < k_cap; ++i) {
        release(i);
        delete m_full[i];
        m_full[i] = nullptr;
    }
    m_sec = nullptr;
}

void SectorPathGradients::release (u16 slot) {
    if (slot >= k_cap) {
        return;
    }
    m_on[slot] = 0u;
    m_mode[slot] = k_off;
}

bool SectorPathGradients::take_full (u16 slot) {
    if (slot >= k_cap) {
        return false;
    }
    release(slot);
    if (m_full[slot] == nullptr) {
        m_full[slot] = new WalkP2P();
    }
    if (m_full[slot] == nullptr || !m_full[slot]->ok()) {
        return false;
    }
    m_mode[slot] = k_full;
    m_on[slot] = 1u;
    return true;
}

bool SectorPathGradients::take_chunk (u8 kind, u16 slot, u16 ox, u16 oy, u16 cw, u16 ch) {
    (void)kind;
    (void)ox;
    (void)oy;
    (void)cw;
    (void)ch;
    return take_full(slot);
}

bool SectorPathGradients::lease (u16 slot, u16 sector) {
    (void)sector;
    return take_full(slot);
}

bool SectorPathGradients::lease_multi (u16 slot, const u16* sectors, u16 n) {
    (void)sectors;
    (void)n;
    return take_full(slot);
}

bool SectorPathGradients::fill (
    u16 slot,
    GameState& st,
    const RuntimeStatics& rst,
    u16 sx,
    u16 sy,
    u16 dx,
    u16 dy,
    u16 seat,
    u8 acc) {
    if (!slot_ok(slot) || m_full[slot] == nullptr) {
        return false;
    }
    u16 dmax = 0u;
    return GenerateDistanceP2P::generate(st, rst, sx, sy, dx, dy, *m_full[slot], seat, acc, &dmax);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
