//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>

#include "tile_array.h"

//================================================================================================================================
//=> - Addendum internals -
//================================================================================================================================

struct Addendum {
    u16 x;
    u16 y;
    u16 add_count;
    u16 allocation_count;
    BuildAddItem* adds;
};

//================================================================================================================================
//=> - TileArray internals -
//================================================================================================================================

static Tile* s_tiles = nullptr;
static u16 s_width = 0;
static u16 s_height = 0;

static Addendum* s_adds = nullptr;
static u16 s_adds_width = 0;
static u16 s_adds_height = 0;

//================================================================================================================================
//=> - TileArray implementation -
//================================================================================================================================

static Addendum* get_addendum_for_tile(u16 x, u16 y) {
    if (s_adds == nullptr || x >= s_width || y >= s_height) {
        return nullptr;
    }
    const u16 adds_x = static_cast<u16>(x / TileArray::ADDS_BLOCK_SIZE);
    const u16 adds_y = static_cast<u16>(y / TileArray::ADDS_BLOCK_SIZE);
    if (adds_x >= s_adds_width || adds_y >= s_adds_height) {
        return nullptr;
    }
    const u32 adds_idx = static_cast<u32>(adds_y) * static_cast<u32>(s_adds_width) + static_cast<u32>(adds_x);
    return &s_adds[adds_idx];
}

static BuildAddItem* upsert_addendum_item(u16 x, u16 y) {
    Addendum* addendum = get_addendum_for_tile(x, y);
    if (addendum == nullptr) {
        return nullptr;
    }

    const u8 x_offset = static_cast<u8>(x % TileArray::ADDS_BLOCK_SIZE);
    const u8 y_offset = static_cast<u8>(y % TileArray::ADDS_BLOCK_SIZE);

    for (u16 i = 0; i < addendum->add_count; ++i) {
        BuildAddItem* item = &addendum->adds[i];
        if (item->x_offset == x_offset && item->y_offset == y_offset) {
            return item;
        }
    }

    if (addendum->add_count >= addendum->allocation_count) {
        const u16 new_allocation_count = static_cast<u16>(addendum->allocation_count + TileArray::ALLOCATION_BLOCK_SIZE);
        BuildAddItem* temp = new BuildAddItem[new_allocation_count]();
        for (u16 i = 0; i < addendum->add_count; ++i) {
            temp[i] = addendum->adds[i];
        }
        delete[] addendum->adds;
        addendum->adds = temp;
        addendum->allocation_count = new_allocation_count;
    }

    BuildAddItem* item = &addendum->adds[addendum->add_count];
    item->x_offset = x_offset;
    item->y_offset = y_offset;
    item->build_add_type = BUILD_ADD_NONE;
    item->build_add_idx = 0;
    item->resource_idx = 0;
    item->next_add_idx = 0;
    addendum->add_count = static_cast<u16>(addendum->add_count + 1);
    return item;
}

static void clear_storage() {
    if (s_tiles != nullptr) {
        delete[] s_tiles;
        s_tiles = nullptr;
    }
    if (s_adds != nullptr) {
        const u32 adds_count = static_cast<u32>(s_adds_width) * static_cast<u32>(s_adds_height);
        for (u32 i = 0; i < adds_count; ++i) {
            if (s_adds[i].adds != nullptr) {
                delete[] s_adds[i].adds;
                s_adds[i].adds = nullptr;
            }
            s_adds[i].add_count = 0;
        }
        delete[] s_adds;
        s_adds = nullptr;
    }
    s_width = 0;
    s_height = 0;
    s_adds_width = 0;
    s_adds_height = 0;
}

bool TileArray::init(u16 width, u16 height) {
    clear_storage();
    if (width == 0 || height == 0) {
        printf ("ERROR: Invalid width or height\n");
        exit(1);
    }

    const u32 tile_count = static_cast<u32>(width) * static_cast<u32>(height);
    s_tiles = new Tile[tile_count]();
    if (s_tiles == nullptr) {
        printf ("ERROR: Failed to allocate tiles\n");
        exit(1);
    }

    s_adds_width = static_cast<u16>((width + TileArray::ADDS_BLOCK_SIZE - 1) / TileArray::ADDS_BLOCK_SIZE);
    s_adds_height = static_cast<u16>((height + TileArray::ADDS_BLOCK_SIZE - 1) / TileArray::ADDS_BLOCK_SIZE);
    const u32 adds_count = static_cast<u32>(s_adds_width) * static_cast<u32>(s_adds_height);
    s_adds = new Addendum[adds_count]();
    if (s_adds == nullptr) {
        printf ("ERROR: Failed to allocate adds\n");
        exit(1);
    }

    for (u16 adds_y = 0; adds_y < s_adds_height; ++adds_y) {
        for (u16 adds_x = 0; adds_x < s_adds_width; ++adds_x) {
            const u32 adds_idx = static_cast<u32>(adds_y) * static_cast<u32>(s_adds_width) + static_cast<u32>(adds_x);
            s_adds[adds_idx].x = adds_x;
            s_adds[adds_idx].y = adds_y;
            s_adds[adds_idx].add_count = 0;
            s_adds[adds_idx].allocation_count = 0;
            s_adds[adds_idx].adds = nullptr;
        }
    }

    s_width = width;
    s_height = height;
    return true;
}

Tile* TileArray::get_tile(u16 x, u16 y) {
    if (x >= s_width || y >= s_height || s_tiles == nullptr) {
        return nullptr;
    }
    const u32 idx = static_cast<u32>(y) * static_cast<u32>(s_width) + static_cast<u32>(x);
    return &s_tiles[idx];
}

BuildAddItem TileArray::get_build_adds(u16 x, u16 y) {
    BuildAddItem result = {};
    result.build_add_type = BUILD_ADD_NONE;

    Tile* tile = get_tile(x, y);
    if (tile == nullptr || tile->adds_exists == 0 || s_adds == nullptr) {
        return result;
    }

    const u16 adds_x = static_cast<u16>(x / ADDS_BLOCK_SIZE);
    const u16 adds_y = static_cast<u16>(y / ADDS_BLOCK_SIZE);
    const u32 adds_idx = static_cast<u32>(adds_y) * static_cast<u32>(s_adds_width) + static_cast<u32>(adds_x);
    Addendum* addendum = &s_adds[adds_idx];

    if (addendum->adds == nullptr || addendum->add_count == 0) {
        return result;
    }

    const u8 x_offset = static_cast<u8>(x % ADDS_BLOCK_SIZE);
    const u8 y_offset = static_cast<u8>(y % ADDS_BLOCK_SIZE);

    for (u16 i = 0; i < addendum->add_count; ++i) {
        BuildAddItem* item = &addendum->adds[i];
        if (item->x_offset == x_offset && item->y_offset == y_offset) {
            if (item->build_add_type != BUILD_ADD_NONE || item->resource_idx != 0) {
                result = *item;
            }
            return result;
        }
    }
    return result;
}

u16 TileArray::get_width() {
    return s_width;
}

u16 TileArray::get_height() {
    return s_height;
}

u32 TileArray::get_tile_count() {
    return static_cast<u32>(s_width) * static_cast<u32>(s_height);
}

//================================================================================================================================
//=> - Config interface -
//================================================================================================================================

void TileArray::add_resource(u16 x, u16 y, u16 resource_idx) {
    Tile* tile = get_tile(x, y);
    if (tile == nullptr) {
        printf ("ERROR: Invalid tile\n");
        exit(1);
    }
    BuildAddItem* item = upsert_addendum_item(x, y);
    if (item == nullptr) {
        printf ("ERROR: Failed to upsert addendum item\n");
        exit(1);
    }
    item->resource_idx = resource_idx;
    tile->adds_exists = 1;
}

void TileArray::add_city(u16 x, u16 y, u16 city_idx) {
    Tile* tile = get_tile(x, y);
    if (tile == nullptr) {
        printf ("ERROR: Invalid tile\n");
        exit(1);
    }
    BuildAddItem* item = upsert_addendum_item(x, y);
    if (item == nullptr) {
        printf ("ERROR: Failed to upsert addendum item\n");
        exit(1);
    }
    item->build_add_type = BUILD_ADD_CITY;
    item->build_add_idx = city_idx;
    tile->adds_exists = 1;
}

void TileArray::add_fort(u16 x, u16 y, u16 fort_idx) {
    Tile* tile = get_tile(x, y);
    if (tile == nullptr) {
        printf ("ERROR: Invalid tile\n");
        exit(1);
    }
    BuildAddItem* item = upsert_addendum_item(x, y);
    if (item == nullptr) {
        printf ("ERROR: Failed to upsert addendum item\n");
        exit(1);
    }
    item->build_add_type = BUILD_ADD_FORT;
    item->build_add_idx = fort_idx;
    tile->adds_exists = 1;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================