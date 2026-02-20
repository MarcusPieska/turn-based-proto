//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TRAINABLE_ASSESSOR_H
#define TRAINABLE_ASSESSOR_H

#include <string>

class UnitVector;
class ResourceVector;

//================================================================================================================================
//=> - TrainableAssessor class -
//================================================================================================================================

class TrainableAssessor {
public:

    TrainableAssessor ();
    ~TrainableAssessor ();

    void load_unit_resource_costs (const std::string& filename);
    void determine_trainable_units (UnitVector* units, const ResourceVector& resources);

    void print_unit_resource_costs ();

private:
    TrainableAssessor (const TrainableAssessor& other) = delete;
    TrainableAssessor (TrainableAssessor&& other) = delete;
};

#endif // TRAINABLE_ASSESSOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
