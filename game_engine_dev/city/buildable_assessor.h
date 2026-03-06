//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDABLE_ASSESSOR_H
#define BUILDABLE_ASSESSOR_H

#include <string>

class BuildingVector;
class ResourceVector;

//================================================================================================================================
//=> - BuildableAssessor class -
//================================================================================================================================

class BuildableAssessor {
public:

    BuildableAssessor ();
    ~BuildableAssessor ();

    void load_building_resource_costs (const std::string& filename);
    void determine_buildable_buildings (BuildingVector* buildings, const ResourceVector& resources);

    void print_building_resource_costs ();

private:
    BuildableAssessor (const BuildableAssessor& other) = delete;
    BuildableAssessor (BuildableAssessor&& other) = delete;
};

#endif // BUILDABLE_ASSESSOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
