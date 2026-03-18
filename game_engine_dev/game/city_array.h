//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_ARRAY_H
#define CITY_ARRAY_H

#include "game_primitives.h"
#include "city.h"

//================================================================================================================================
//=> - CityArray class -
//================================================================================================================================

class CityArray {
public:
    CityArray();
    ~CityArray();

    City* get_city(u16 city_idx);
    const City* get_city(u16 city_idx) const;
    u16 get_next_new_city_idx();
    u16 get_page_count() const;
    u16 get_city_count() const;
    City* get_page(u16 page_idx);
    const City* get_page(u16 page_idx) const;

    static const u16 MAX_PAGES = 256;
    static const u16 CITIES_PER_PAGE = 256;

private:
    City* m_pages[MAX_PAGES];
    u16 m_city_count;
    u16 m_page_count;
};

#endif // CITY_ARRAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
