//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GAME_PRIMITIVES_H
#define GAME_PRIMITIVES_H

#include <cstdint>

typedef const char* cstr;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

#define MAX_EFFECTS_PER_ENTITY 4
typedef struct EffectIndices {
    u16 indices[MAX_EFFECTS_PER_ENTITY];
} EffectIndices;

#define MAX_RESOURCES_PER_ENTITY 4
typedef struct ResourceIndices {
    u16 indices[MAX_RESOURCES_PER_ENTITY];
} ResourceIndices;

#define MAX_TECHS_PER_ENTITY 4
typedef struct TechIndices {
    u16 indices[MAX_TECHS_PER_ENTITY];
} TechIndices;

#endif // GAME_PRIMITIVES_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
