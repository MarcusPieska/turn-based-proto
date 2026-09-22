//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TECH_AGE_MNG_H
#define TECH_AGE_MNG_H

#include "game_primitives.h"

class RuntimeStatics;

//================================================================================================================================
//=> - TechAgeMng -
//================================================================================================================================
//
//  Per-player age unlock gate. Static catalog sizes (techs per age); each instance tracks researched
//  counts and head_age (highest unlocked age). log_tech increments the tech's age; at
//  TECH_AGE_UNLOCK_PCT ceil of that age's catalog, head advances (empty ages chain).
//  is_available is age-gate only.
//
//================================================================================================================================

class TechAgeMng {
public:
    static bool setup (const RuntimeStatics& st); // Alloc catalog; read unlock pct from config
    static void clear (); // Free catalog; drop statics
    static bool ready (); // True after successful setup
    static u16 age_n (); // Catalog age count
    static u8 cat (u16 age); // Techs in age; 0 if empty
    static u16 pct (); // Unlock threshold percent from settings

    TechAgeMng ();
    ~TechAgeMng ();

    void reset (); // Zero counts; head_age 0 then chain empty ages
    bool log_tech (u16 tech_idx); // +1 to tech's age; may advance head_age
    bool is_available (u16 tech_idx) const; // True if tech's age <= head_age
    u16 head_age () const; // Highest unlocked age index
    u8 done (u16 age) const; // Researched count for age

private:
    TechAgeMng (const TechAgeMng& o) = delete;
    TechAgeMng& operator= (const TechAgeMng& o) = delete;

    static u8* m_cat; // Catalog size per age
    static u16 m_age_n; // Age count
    static u16 m_pct; // Unlock pct from TECH_AGE_UNLOCK_PCT
    static const RuntimeStatics* m_st; // Bound statics; not owned
    static bool m_ready; // True after setup

    u8* m_done; // Researched count per age
    u16 m_head; // Highest unlocked age

    static u32 need (u8 catalog_n); // Ceil pct of catalog; 0 if empty
    void adv (); // Advance head while current age meets need
};

#endif // TECH_AGE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
