//
// Created by ofskrand on 09.01.2025.
//

#ifndef STEP_TREE_H
#define STEP_TREE_H

// Our struct from above
struct ProductNode {
    int entityIndex;
    std::string name;
    std::vector<ProductNode> children;
};

std::vector<ProductNode> ExtractProductHierarchy(const Handle(Interface_InterfaceModel)& model);

std::string ExportHierarchyToJson(const std::vector<ProductNode>& roots);

#endif //STEP_TREE_H
