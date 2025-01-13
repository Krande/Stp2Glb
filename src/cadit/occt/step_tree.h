//
// Created by ofskrand on 09.01.2025.
//

#ifndef STEP_TREE_H
#define STEP_TREE_H

#include <gp_Trsf.hxx>
#include <Interface_InterfaceModel.hxx>
#include <Interface_Graph.hxx>
#include <Standard_Handle.hxx>  // For Handle
#include <StepRepr_NextAssemblyUsageOccurrence.hxx>
#include <coroutine>
#include <string>
#include <TDF_Label.hxx>
#include <vector>
#include <unordered_map>


// Updated struct
struct ProductNode {
    ProductNode* parent = nullptr; // new field to keep track of the node's parent

    int entityIndex;
    std::string name;
    std::vector<std::unique_ptr<ProductNode>> children;
    int instanceIndex;
    std::vector<int> geometryIndices;
    TDF_Label targetIndex;
    gp_Trsf transformation;

    void collectNodesWithGeometry(std::vector<const ProductNode*>& result) const {
        if (!geometryIndices.empty()) {
            result.push_back(this);
        }
        for (const auto& child : children) {
            child->collectNodesWithGeometry(result);
        }
    }
};

std::vector<std::unique_ptr<ProductNode>> ExtractProductHierarchy(const Handle(Interface_InterfaceModel)& model, const Interface_Graph& theGraph);

std::string ExportHierarchyToJson(const std::vector<std::unique_ptr<ProductNode>>& roots);

void add_geometries_to_nodes(std::vector<std::unique_ptr<ProductNode>> &nodes, const Interface_Graph &theGraph);

gp_Trsf GetTransformationMatrix(
    const Handle(StepRepr_NextAssemblyUsageOccurrence)& nauo,
    const Interface_Graph& theGraph);

static std::unique_ptr<ProductNode> BuildProductNodeWithTransform(
    int productIndex,
    const std::unordered_map<int, std::vector<int>>& parentToChildren,
    const Handle(Interface_InterfaceModel)& model,
    const Interface_Graph& theGraph,
    const gp_Trsf& parentTransform);

static std::unique_ptr<ProductNode> BuildProductNodeWithTransformIterative(
    int rootIndex,
    const std::unordered_map<int, std::vector<int>>& parentToChildren,
    const Handle(Interface_InterfaceModel)& model,
    const Interface_Graph& theGraph,
    const gp_Trsf& rootTransform = gp_Trsf());

#endif //STEP_TREE_H
