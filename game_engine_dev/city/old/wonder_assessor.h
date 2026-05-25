//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WONDER_ASSESSOR_H
#define WONDER_ASSESSOR_H

class BitArrayCL;
class BuiltBuildings;
class BuildableWonders;

//================================================================================================================================
//=> - BuildableAssessor class -
//================================================================================================================================

class WonderAssessor {
    public:
        static BuildableWonders* assess (
            const BitArrayCL* techs, 
            const BuiltBuildings* buildings, 
            const BitArrayCL* resources, 
            const BitArrayCL* flags
        );
    
    private:
        WonderAssessor () = delete;
        WonderAssessor (const WonderAssessor& other) = delete;
        WonderAssessor (WonderAssessor&& other) = delete;
    };
#endif // WONDER_ASSESSOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
