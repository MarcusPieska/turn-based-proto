//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GAME_IO_CMP_H
#define GAME_IO_CMP_H

#include "game_primitives.h"

class GameArraySimple;
class UnitAddVector;
class CityArray;
class PlayerState;

//================================================================================================================================
//=> - GameIoCmp -
//================================================================================================================================

//
//  Item-by-item equality for GameIo hot-path blobs (map, units, cities, players).
//

//================================================================================================================================

class GeneralBitBank;
class BitArrayCL;
struct UnitAddStruct;

class GameIoCmp {
public:
    GameIoCmp () = delete;

    static bool map (const GameArraySimple& a, const GameArraySimple& b);
    static bool units (const UnitAddVector& a, const UnitAddVector& b);
    static bool cities (const CityArray& a, const CityArray& b);
    static bool players (const PlayerState* a, u16 a_n, const PlayerState* b, u16 b_n);

private:
    static bool bank (const GeneralBitBank* a, const GeneralBitBank* b);
    static bool bit_cl (const BitArrayCL* a, const BitArrayCL* b);
    static bool unit (const UnitAddStruct& a, const UnitAddStruct& b);
};

#endif // GAME_IO_CMP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
