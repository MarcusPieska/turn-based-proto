//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDABLE_ASSESSOR_H
#define BUILDABLE_ASSESSOR_H

class BitArrayCL;
class BuiltBuildings;
class BuildableBuildings;

//================================================================================================================================
//=> - BuildingAssessor class -
//================================================================================================================================

class BuildingAssessor {
public:
    static BuildableBuildings* assess (
        const BitArrayCL* techs,
        const BuiltBuildings* buildings,
        const BitArrayCL* resources,
        const BitArrayCL* flags
    );

private:
    BuildingAssessor () = delete;
    BuildingAssessor (const BuildingAssessor& other) = delete;
    BuildingAssessor (BuildingAssessor&& other) = delete;
};

#endif // BUILDABLE_ASSESSOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
