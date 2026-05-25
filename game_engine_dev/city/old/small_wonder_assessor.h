//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SMALL_WONDER_ASSESSOR_H
#define SMALL_WONDER_ASSESSOR_H

class BitArrayCL;
class BuiltBuildings;
class BuildableSmallWonders;

//================================================================================================================================
//=> - SmallWonderAssessor class -
//================================================================================================================================

class SmallWonderAssessor {
    public:
        static BuildableSmallWonders* assess (
            const BitArrayCL* techs, 
            const BuiltBuildings* buildings, 
            const BitArrayCL* resources, 
            const BitArrayCL* flags,
            const BuiltSmallWonders* built
        );
    
    private:
        SmallWonderAssessor () = delete;
        SmallWonderAssessor (const SmallWonderAssessor& other) = delete;
        SmallWonderAssessor (SmallWonderAssessor&& other) = delete;
    };
#endif // SMALL_WONDER_ASSESSOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
