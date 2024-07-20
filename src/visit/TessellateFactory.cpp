#include "TessellateFactory.h"

// a geometry store that can batch tessellate shapes to meshes using different tessellation algorithms
TessellateFactory::TessellateFactory(TessellationAlgorithm algorithm) {
    this->algorithm = algorithm;
}


void tess_module(nb::module_ &m) {

    nb::enum_<TessellationAlgorithm>(m, "TessellationAlgorithm")
            .value("OCCT_DEFAULT", TessellationAlgorithm::OCCT_DEFAULT,
                   "Converts to OCC TopoDS_Shape and uses BRepMesh_IncrementalMesh")
            .value("CGAL_DEFAULT", TessellationAlgorithm::CGAL_DEFAULT, "Uses CGAL's polyhedron_3");

    nb::class_<TessellateFactory>(m, "TessellateFactory")
            .def(nb::init<>())
            .def(nb::init<const TessellationAlgorithm &>())
            .def("tessellate", &TessellateFactory::tessellate)
            .def_rw("algorithm", &TessellateFactory::algorithm, "Tessellation algorithm");

}
