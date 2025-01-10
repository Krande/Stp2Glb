//
// Created by ofskrand on 09.01.2025.
//

#ifndef STEP_TREE_H
#define STEP_TREE_H

#include <Interface_InterfaceModel.hxx>
#include <Interface_Graph.hxx>
#include <Standard_Handle.hxx>  // For Handle

// Our struct from above
struct ProductNode {
    int entityIndex;
    std::string name;
    std::vector<ProductNode> children;
};

std::vector<ProductNode> ExtractProductHierarchy(const Handle(Interface_InterfaceModel)& model, const Interface_Graph& theGraph);

std::string ExportHierarchyToJson(const std::vector<ProductNode>& roots);

#endif //STEP_TREE_H
