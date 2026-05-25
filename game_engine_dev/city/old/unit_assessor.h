//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_ASSESSOR_H
#define UNIT_ASSESSOR_H

class BitArrayCL;
class BuiltBuildings;
class BuildableUnits;

//================================================================================================================================
//=> - UnitAssessor class -
//================================================================================================================================

class UnitAssessor {
public:
    static BuildableUnits* assess(
        const BitArrayCL* techs,
        const BuiltBuildings* buildings,
        const BitArrayCL* resources,
        const BitArrayCL* flags
    );

private:
    UnitAssessor () = delete;
    UnitAssessor (const UnitAssessor& other) = delete;
    UnitAssessor (UnitAssessor&& other) = delete;
};

#endif // UNIT_ASSESSOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
