//
// Created by Kristoffer on 29/07/2023.
//

#ifndef ADA_CPP_TESSELLATEFACTORY_H
#define ADA_CPP_TESSELLATEFACTORY_H

#include <iostream>
#include "../binding_core.h"


// Enum for different tessellation algorithms
enum class TessellationAlgorithm : uint32_t {
    OCCT_DEFAULT = 0, // Converts to OCC TopoDS_Shape and uses BRepMesh_IncrementalMesh
    CGAL_DEFAULT = 1, // Uses CGAL's polyhedron_3

};

class TessellateFactory {
public:
    explicit TessellateFactory(TessellationAlgorithm algorithm = static_cast<TessellationAlgorithm>(0));

    TessellationAlgorithm algorithm;

    void tessellate(){
        // print to console which algorithm is used
        std::cout << "Tessellating using algorithm: " << static_cast<int>(algorithm) << std::endl;
    }
};

void tess_module(nb::module_ &m);


#endif //ADA_CPP_TESSELLATEFACTORY_H
