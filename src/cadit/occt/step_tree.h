//
// Created by ofskrand on 09.01.2025.
//

#ifndef STEP_TREE_H
#define STEP_TREE_H

#include <gp_Trsf.hxx>
#include <Interface_InterfaceModel.hxx>
#include <Interface_Graph.hxx>
#include <Standard_Handle.hxx>  // For Handle

// Our struct from above
struct ProductNode {
    int entityIndex;
    std::string name;
    std::vector<ProductNode> children;
    // references to geometries
    std::vector<int> geometryIndices;
    gp_Trsf transformation;
};

std::vector<ProductNode> ExtractProductHierarchy(const Handle(Interface_InterfaceModel)& model, const Interface_Graph& theGraph);

std::string ExportHierarchyToJson(const std::vector<ProductNode>& roots);

void add_geometries_to_nodes(std::vector<ProductNode> &nodes, const Interface_Graph &theGraph);

gp_Trsf GetTransformationMatrix(
    const Handle(StepRepr_NextAssemblyUsageOccurrence)& nauo,
    const Interface_Graph& theGraph);

#endif //STEP_TREE_H
