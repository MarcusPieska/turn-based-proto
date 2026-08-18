//================================================================================================================================
//=> - SettlerMissionManager mk01 -
//================================================================================================================================

bool SettlerMissionManager::wbeg (
    const SectorNetwork& net,
    const SectorNetworkRouter& rt,
    const u8* terr,
    u16 w,
    u16 h)
{
    for (u16 i = 0; i < SMM_SLOT_N; ++i) {
        if (!m_slot[i].m_walk.begin(net, rt, terr, w, h)) {
            return false;
        }
    }
    return true;
}

bool SettlerMissionManager::aim (u16 s, u16 x0, u16 y0, u16 tx, u16 ty) {
    Slot& sl = m_slot[s];
    if (!sl.m_walk.start(x0, y0, tx, ty)) {
        return false;
    }
    sl.m_x = x0;
    sl.m_y = y0;
    sl.m_tx = tx;
    sl.m_ty = ty;
    return true;
}

bool SettlerMissionManager::wgo (u16 s) {
    Slot& sl = m_slot[s];
    const bool ok = sl.m_walk.step();
    sl.m_x = sl.m_walk.x();
    sl.m_y = sl.m_walk.y();
    return ok;
}

bool SettlerMissionManager::wdn (u16 s) const {
    return m_slot[s].m_walk.done();
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
