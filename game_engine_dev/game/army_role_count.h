//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef ARMY_ROLE_COUNT_H
#define ARMY_ROLE_COUNT_H

#include "game_primitives.h"

//================================================================================================================================
//=> - ArmyRoleCount -
//================================================================================================================================
//
//  Per-seat built+building tallies for land defend / attack / barrage roles. Delta updates are
//  clamped at zero; tot() is the sum used against free support * factor.
//
//================================================================================================================================

class ArmyRoleCount {
public:
    ArmyRoleCount ();

    void add_def (i32 d);
    void add_atk (i32 d);
    void add_brg (i32 d);

    u16 def () const;
    u16 atk () const;
    u16 brg () const;
    u32 tot () const;

private:
    static void add_one (u16* n, i32 d);

    u16 m_def;
    u16 m_atk;
    u16 m_brg;
};

//================================================================================================================================
//=> - Inline -
//================================================================================================================================

inline ArmyRoleCount::ArmyRoleCount () : m_def(0u), m_atk(0u), m_brg(0u) {
}

inline void ArmyRoleCount::add_one (u16* n, i32 d) {
    if (n == nullptr || d == 0) {
        return;
    }
    if (d > 0) {
        const u32 s = static_cast<u32>(*n) + static_cast<u32>(d);
        *n = s > 65535u ? 65535u : static_cast<u16>(s);
        return;
    }
    const u32 sub = static_cast<u32>(-d);
    if (sub >= static_cast<u32>(*n)) {
        *n = 0u;
    } else {
        *n = static_cast<u16>(static_cast<u32>(*n) - sub);
    }
}

inline void ArmyRoleCount::add_def (i32 d) {
    add_one(&m_def, d);
}

inline void ArmyRoleCount::add_atk (i32 d) {
    add_one(&m_atk, d);
}

inline void ArmyRoleCount::add_brg (i32 d) {
    add_one(&m_brg, d);
}

inline u16 ArmyRoleCount::def () const {
    return m_def;
}

inline u16 ArmyRoleCount::atk () const {
    return m_atk;
}

inline u16 ArmyRoleCount::brg () const {
    return m_brg;
}

inline u32 ArmyRoleCount::tot () const {
    return static_cast<u32>(m_def) + static_cast<u32>(m_atk) + static_cast<u32>(m_brg);
}

#endif // ARMY_ROLE_COUNT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
