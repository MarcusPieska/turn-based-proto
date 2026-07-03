//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "map_terrain_validate.h"

#include "generator_constants.h"

//================================================================================================================================
//=> - MapTerrainValidate -
//================================================================================================================================

static const u8* const k_terr_rows[] = {
    TERR_NONE,
    TERR_OCEAN,
    TERR_SEA,
    TERR_COASTAL,
    TERR_PLAINS,
    TERR_HILLS,
    TERR_MOUNTAINS,
    TERR_INLAND_SEA,
    TERR_INLAND_LAKE};

static bool is_valid_class (u8 cls) {
    for (unsigned i = 0; i < sizeof(k_terr_rows) / sizeof(k_terr_rows[0]); ++i) {
        if (k_terr_rows[i][0] == cls) {
            return true;
        }
    }
    return false;
}

u8 MapTerrainValidate::class_from_rgb (u8 r, u8 g, u8 b, bool* matched) {
    if (matched != nullptr) {
        *matched = false;
    }
    for (unsigned i = 0; i < sizeof(k_terr_rows) / sizeof(k_terr_rows[0]); ++i) {
        const u8* row = k_terr_rows[i];
        if (row[1] == r && row[2] == g && row[3] == b) {
            if (matched != nullptr) {
                *matched = true;
            }
            return row[0];
        }
    }
    return TERR_NONE[0];
}

void MapTerrainValidate::rgb_from_class (u8 cls, u8* r, u8* g, u8* b) {
    for (unsigned i = 0; i < sizeof(k_terr_rows) / sizeof(k_terr_rows[0]); ++i) {
        const u8* row = k_terr_rows[i];
        if (row[0] == cls) {
            *r = row[1];
            *g = row[2];
            *b = row[3];
            return;
        }
    }
    *r = TERR_NONE[1];
    *g = TERR_NONE[2];
    *b = TERR_NONE[3];
}

bool MapTerrainValidate::chk_classes (const u8* rows, u32 n) {
    if (rows == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        if (!is_valid_class(rows[i])) {
            return false;
        }
    }
    return true;
}

bool MapTerrainValidate::chk_rgb (const u8* rgb, u32 px_n) {
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < px_n; ++i) {
        const u8* px = rgb + i * 3u;
        bool matched = false;
        (void)class_from_rgb(px[0], px[1], px[2], &matched);
        if (!matched) {
            return false;
        }
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
