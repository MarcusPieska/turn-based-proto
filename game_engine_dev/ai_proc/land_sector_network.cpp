//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "land_sector_network.h"

//================================================================================================================================
//=> - LandSectorNetwork -
//================================================================================================================================

LandSectorNetwork::LandSectorNetwork () :
    m_links(nullptr),
    m_n(0u),
    m_ok(false) {
}

LandSectorNetwork::~LandSectorNetwork () {
    clr();
}

void LandSectorNetwork::clr () {
    delete[] m_links;
    m_links = nullptr;
    m_n = 0u;
    m_ok = false;
}

bool LandSectorNetwork::ok () const {
    return m_ok;
}

u16 LandSectorNetwork::link_n () const {
    return m_n;
}

const LandSectorLink* LandSectorNetwork::get (u16 i) const {
    if (!m_ok || m_links == nullptr || i >= m_n) {
        return nullptr;
    }
    return &m_links[i];
}

bool LandSectorNetwork::take (LandSectorLink* links, u16 n) {
    clr();
    if (links == nullptr && n != 0u) {
        return false;
    }
    m_links = links;
    m_n = n;
    m_ok = true;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
