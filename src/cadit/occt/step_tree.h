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

struct ProcessResult {
    mutable bool added_to_model;
    mutable std::string skip_reason;
    mutable int geometryIndex;
};

struct GeometryInstance {
    int entityIndex{};
    gp_Trsf transformation;
};

// Updated struct
struct ProductNode {
    // Static counter for generating unique instance indices
    static int instanceCounter;


    ProductNode* parent; // new field to keep track of the node's parent

    int entityIndex;
    std::string name;
    std::vector<std::unique_ptr<ProductNode> > children;
    int instanceIndex;
    std::vector<GeometryInstance> geometryInstances;
    TDF_Label targetIndex;
    gp_Trsf transformation;
    ProcessResult processResult = ProcessResult(false, "");

    // Constructor to initialize and assign unique instanceIndex
    ProductNode() : instanceIndex(instanceCounter++) {}

    void collectNodesWithGeometry(std::vector<const ProductNode *> &result) const {
        if (!geometryInstances.empty()) {
            result.push_back(this);
        }
        for (const auto &child: children) {
            child->collectNodesWithGeometry(result);
        }
    }
};

// Define a struct to hold the parent-child relationship and transformation
struct ParentChildRelationship {
    int parentIndex;
    int childIndex;
    gp_Trsf transformation;  // Transformation between parent and child
    int nauoIndex;

    ParentChildRelationship(int pIdx, int cIdx, const gp_Trsf& transform, int nauoIdx)
        : parentIndex(pIdx), childIndex(cIdx), transformation(transform), nauoIndex(nauoIdx) {
    }
};

std::vector<std::unique_ptr<ProductNode> > ExtractProductHierarchy(const Handle(Interface_InterfaceModel) &model,
                                                                   const Interface_Graph &theGraph);

std::string ExportHierarchyToJson(const std::vector<std::unique_ptr<ProductNode> > &roots);

void add_geometries_to_nodes(const std::vector<std::unique_ptr<ProductNode> > &nodes, const Interface_Graph &theGraph);

gp_Trsf GetTransformationMatrix(
    const Handle(StepRepr_NextAssemblyUsageOccurrence) &nauo,
    const Interface_Graph &theGraph);

static std::unique_ptr<ProductNode> BuildProductNodeWithTransform(
    int productIndex,
    const std::unordered_map<int, std::vector<ParentChildRelationship>>& parentToChildrenWTransforms,
    const Handle(Interface_InterfaceModel)& model,
    const Interface_Graph& theGraph,
    const gp_Trsf& parentTransform = gp_Trsf(),
    ProductNode* parent = nullptr);

static std::unique_ptr<ProductNode> BuildProductNodeWithTransformIterative(
    int rootIndex,
    const std::unordered_map<int, std::vector<int> > &parentToChildren,
    const Handle(Interface_InterfaceModel) &model,
    const Interface_Graph &theGraph,
    const gp_Trsf &rootTransform = gp_Trsf());

#endif //STEP_TREE_H
