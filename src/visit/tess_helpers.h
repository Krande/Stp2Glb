//
// Created by Kristoffer on 07/05/2023.
//

#ifndef NANO_OCCT_TESS_HELPERS_H
#define NANO_OCCT_TESS_HELPERS_H

#include <TopoDS_Shape.hxx>
#include "ShapeTesselator.h"
#include <memory>
#include "../geom/Mesh.h"
#include "../binding_core.h"

Mesh tessellate_shape(int id, const TopoDS_Shape &shape, bool compute_edges, float mesh_quality, bool parallel_meshing);

void tess_helper_module(nb::module_ &m);
#endif //NANO_OCCT_TESS_HELPERS_H
