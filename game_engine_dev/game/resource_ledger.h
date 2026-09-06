//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RESOURCE_LEDGER_H
#define RESOURCE_LEDGER_H

#include "game_primitives.h"

//================================================================================================================================
//=> - ResourceLedger -
//================================================================================================================================
//
//  Per-seat stockpile mirroring the static resource catalog (one u16 slot per catalog index).
//  add clamps each slot to CAP; setup allocates a zeroed row of length res_n.
//
//================================================================================================================================

class ResourceLedger {
public:
    static constexpr u16 CAP = 50000u;

    ResourceLedger ();
    ~ResourceLedger ();

    bool setup (u16 res_n);
    void clear ();

    bool add (u16 res_idx, u16 amt);
    u16 get (u16 res_idx) const;
    u16 count () const;

private:
    u16* m_v; // Dense stock; length m_n
    u16 m_n; // Catalog length; 0 until setup

    ResourceLedger (const ResourceLedger& other) = delete;
    ResourceLedger (ResourceLedger&& other) = delete;
};

#endif // RESOURCE_LEDGER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
