//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WONDER_ASSESSOR_H
#define WONDER_ASSESSOR_H

class BitArrayCL;
class WonderBuildableVector;

//================================================================================================================================
//=> - BuildableAssessor class -
//================================================================================================================================

class WonderAssessor {
    public:
        static WonderBuildableVector* assess (
            const BitArrayCL* techs, 
            const BitArrayCL* buildings, 
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
