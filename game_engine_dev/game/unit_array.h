//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_ARRAY_H
#define UNIT_ARRAY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Temp unit stuct until real is hooked up -
//================================================================================================================================

class Unit {
    friend class UnitArray;

public:
    bool do_exist() const { return exists != 0; }
    u32 id;

private:
    u8 exists;
    u16 idx;
};

//================================================================================================================================
//=> - UnitArray class -
//================================================================================================================================

class UnitArray {
public:
    UnitArray();
    ~UnitArray();

    Unit* get_unit(u16 unit_idx);
    const Unit* get_unit(u16 unit_idx) const;
    u16 get_next_new_unit_idx();
    u16 get_page_count() const;
    u16 get_unit_count() const;
    u16 get_head_unit_count() const;
    void return_unit(Unit* unit);
    Unit* get_page(u16 page_idx);
    const Unit* get_page(u16 page_idx) const;

    static const u16 MAX_PAGES = 256;
    static const u16 UNITS_PER_PAGE = 256;

private:
    friend class UnitArrayTester;

    Unit* m_pages[MAX_PAGES];
    u16 m_unit_count;
    u16 m_head_unit_idx;
    u16 m_page_count;

    u16* m_recycled_pages[MAX_PAGES];
    u16 m_recycled_unit_count;
    u16 m_recycled_page_count;
};

#endif // UNIT_ARRAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
