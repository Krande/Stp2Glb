#include <Interface_InterfaceModel.hxx>
#include <Interface_Graph.hxx>
#include <Interface_EntityIterator.hxx>

// STEP entity classes
#include <StepBasic_Product.hxx>
#include <StepBasic_ProductDefinition.hxx>
#include <StepBasic_ProductDefinitionFormation.hxx>
#include <StepRepr_NextAssemblyUsageOccurrence.hxx>
#include <StepRepr_ProductDefinitionShape.hxx>
#include <TCollection_HAsciiString.hxx>
#include <Standard_Type.hxx>
#include <Standard_Transient.hxx>

#include <unordered_map>
#include <unordered_set>
#include <queue>

#include <sstream>
#include "step_tree.h"

#include <gp_Ax1.hxx>
#include <StepGeom_Axis2Placement3D.hxx>
#include <StepGeom_CartesianPoint.hxx>
#include <StepGeom_Direction.hxx>

#include "step_helpers.h"


// Helper: given a Product, find its associated ProductDefinition (if any)
static Handle(StepBasic_ProductDefinition) FindProductDefinition(
    const Handle(StepBasic_Product)& product,
    const Handle(Interface_InterfaceModel)& model, const Interface_Graph& theGraph)
{
    // A Product typically references a ProductDefinitionFormation
    // which references a ProductDefinition. Or in some schemas,
    // the Product's "FrameOfReference()" leads to the PDF.
    // So we walk the entire model to match the formation to the definition.
    //
    // If your schema doesn't use Formation(), adapt accordingly (e.g. FrameOfReference).
    Handle(StepBasic_ProductDefinitionFormation) formation;
    {
        Interface_EntityIterator childIter = theGraph.Sharings(product);
        for (childIter.Start(); childIter.More(); childIter.Next()) {
            Handle(Standard_Transient) ent = childIter.Value();
            if (ent->IsKind(STANDARD_TYPE(StepBasic_ProductDefinitionFormation))) {
                formation = Handle(StepBasic_ProductDefinitionFormation)::DownCast(ent);
                break;
            }
        }
    }

    if (formation.IsNull())
    {
        return nullptr;
    }
    {
        Interface_EntityIterator childIter = theGraph.Sharings(formation);
        for (childIter.Start(); childIter.More(); childIter.Next())
        {
            Handle(Standard_Transient) ent = childIter.Value();
            if (ent->IsKind(STANDARD_TYPE(StepBasic_ProductDefinition)))
            {
                return Handle(StepBasic_ProductDefinition)::DownCast(ent);
            }
        }

    }
    // The ProductDefinition we want references this formation
    for (Standard_Integer i = 1; i <= model->NbEntities(); i++)
    {
        Handle(Standard_Transient) ent = model->Value(i);
        if (ent->IsKind(STANDARD_TYPE(StepBasic_ProductDefinition)))
        {
            auto pd = Handle(StepBasic_ProductDefinition)::DownCast(ent);
            if (!pd.IsNull() && pd->Formation() == formation)
            {
                return pd;
            }
        }
    }
    return nullptr;
}

// Build a map from StepBasic_Product -> entity index, for quick lookup
static std::unordered_map<Handle(StepBasic_Product), int>
BuildProductIndexMap(const Handle(Interface_InterfaceModel)& model)
{
    std::unordered_map<Handle(StepBasic_Product), int> productToIndex;

    for (Standard_Integer i = 1; i <= model->NbEntities(); i++)
    {
        Handle(Standard_Transient) entity = model->Value(i);
        if (entity->IsKind(STANDARD_TYPE(StepBasic_Product)))
        {
            auto product = Handle(StepBasic_Product)::DownCast(entity);
            if (!product.IsNull())
            {
                productToIndex[product] = i; // store 1-based index
            }
        }
    }
    return productToIndex;
}

// Build a map: parentIndex -> list of child indices
static std::unordered_map<int, std::vector<int>>
BuildAssemblyLinks(const Handle(Interface_InterfaceModel)& model, const Interface_Graph& theGraph)
{
    // We'll track: Parent's entity index -> [Child's entity index...]
    std::unordered_map<int, std::vector<int>> parentToChildren;

    // Build a quick map from ProductDefinition -> (Product, index)
    // so that given a PD, we know which Product it belongs to
    std::unordered_map<Handle(StepBasic_ProductDefinition), int> pdToProductIndex;

    // 1) find all products, map each to its product definition
    auto productIndexMap = BuildProductIndexMap(model);
    for (auto& kv : productIndexMap)
    {
        Handle(StepBasic_Product) product = kv.first;
        int productIdx = kv.second;

        Handle(StepBasic_ProductDefinition) pd = FindProductDefinition(product, model, theGraph);
        if (!pd.IsNull())
        {
            pdToProductIndex[pd] = productIdx;
        }
    }

    // 2) Traverse NextAssemblyUsageOccurrence to see parent-child relationships
    //    Each NAUO references two ProductDefinitions: "RelatingProductDefinition"
    //    (the parent) and "RelatedProductDefinition" (the child).
    for (Standard_Integer i = 1; i <= model->NbEntities(); i++)
    {
        Handle(Standard_Transient) ent = model->Value(i);
        if (ent->IsKind(STANDARD_TYPE(StepRepr_NextAssemblyUsageOccurrence)))
        {
            auto nauo = Handle(StepRepr_NextAssemblyUsageOccurrence)::DownCast(ent);
            if (!nauo.IsNull())
            {
                Handle(StepBasic_ProductDefinition) pdParent = nauo->RelatingProductDefinition();
                Handle(StepBasic_ProductDefinition) pdChild = nauo->RelatedProductDefinition();

                auto itParent = pdToProductIndex.find(pdParent);
                auto itChild = pdToProductIndex.find(pdChild);
                if (itParent != pdToProductIndex.end() && itChild != pdToProductIndex.end())
                {
                    int parentIdx = itParent->second;
                    int childIdx = itChild->second;

                    parentToChildren[parentIdx].push_back(childIdx);
                }
            }
        }
    }
    return parentToChildren;
}

// Recursive function that builds a ProductNode tree
static ProductNode BuildProductNode(
    int productIndex,
    const std::unordered_map<int, std::vector<int>>& parentToChildren,
    const Handle(Interface_InterfaceModel)& model)
{
    ProductNode node;
    node.entityIndex = productIndex;

    Handle(Standard_Transient) ent = model->Value(productIndex);
    auto product = Handle(StepBasic_Product)::DownCast(ent);
    if (!product.IsNull() && !product->Name().IsNull())
    {
        node.name = product->Name()->ToCString();
    }
    else
    {
        node.name = "(unnamed product)";
    }

    // Recurse for children
    auto it = parentToChildren.find(productIndex);
    if (it != parentToChildren.end())
    {
        for (int childIdx : it->second)
        {
            node.children.push_back(
                BuildProductNode(childIdx, parentToChildren, model)
            );
        }
    }
    return node;
}

// Main function: extracts top-level ProductNode trees
std::vector<ProductNode> ExtractProductHierarchy(const Handle(Interface_InterfaceModel)& model, const Interface_Graph& theGraph)
{
    // 1) Build the map of parent->children relationships
    auto parentToChildren = BuildAssemblyLinks(model, theGraph);

    // 2) We want to find "root" products (those that never appear as a child)
    std::unordered_set<int> allChildren;
    for (auto& kv : parentToChildren)
    {
        for (int child : kv.second)
        {
            allChildren.insert(child);
        }
    }

    // 3) Gather all product indices
    std::vector<int> allProducts;
    auto productIndexMap = BuildProductIndexMap(model);
    for (auto& kv : productIndexMap)
    {
        allProducts.push_back(kv.second);
    }

    // 4) For each product, if it’s NOT in allChildren => it’s a root
    std::vector<ProductNode> roots;
    for (int idx : allProducts)
    {
        if (allChildren.find(idx) == allChildren.end())
        {
            // This is a root product
            roots.push_back(BuildProductNode(idx, parentToChildren, model));
        }
    }
    return roots;
}


// Simple recursive JSON builder
static void ProductNodeToJson(const ProductNode& node, std::ostream& os, int indentLevel)
{
    // Utility lambda to insert some indentation spaces
    auto indent = [&](int level)
    {
        for (int i = 0; i < level; i++) os << "  ";
    };

    indent(indentLevel);
    os << "{\n";

    indent(indentLevel + 1);
    os << "\"entityIndex\": " << node.entityIndex << ",\n";

    indent(indentLevel + 1);
    os << "\"name\": \"" << node.name << "\",\n";

    indent(indentLevel + 1);
    os << "\"geometryIndices\": [";
    for (size_t i = 0; i < node.geometryIndices.size(); i++)
    {
        os << node.geometryIndices[i];
        if (i + 1 < node.geometryIndices.size())
        {
            os << ", ";
        }
    }
    os << "],\n";

    indent(indentLevel + 1);
    os << "\"children\": [\n";
    for (size_t i = 0; i < node.children.size(); i++)
    {
        ProductNodeToJson(node.children[i], os, indentLevel + 2);
        if (i + 1 < node.children.size())
        {
            os << ",";
        }
        os << "\n";
    }
    indent(indentLevel + 1);
    os << "]\n";

    indent(indentLevel);
    os << "}";
}

std::string ExportHierarchyToJson(const std::vector<ProductNode>& roots)
{
    std::ostringstream oss;
    oss << "[\n";
    for (size_t i = 0; i < roots.size(); i++)
    {
        ProductNodeToJson(roots[i], oss, 1);
        if (i + 1 < roots.size())
        {
            oss << ",";
        }
        oss << "\n";
    }
    oss << "]\n";

    return oss.str();
}

void add_geometries_to_nodes(std::vector<ProductNode> &nodes, const Interface_Graph &theGraph) {
    for (auto &node : nodes) {
        auto& product = theGraph.Entity(node.entityIndex);

        Interface_EntityIterator breps =
                Get_Associated_SolidModel_BiDirectional(product,
                                                STANDARD_TYPE(StepShape_SolidModel),
                                                theGraph);
        // get the geometry indices
        while (breps.More()) {
            auto entity = breps.Value();
            auto entityIndex = theGraph.Model()->Number(entity);
            node.geometryIndices.push_back(entityIndex);
            breps.Next();
        }
        if (!node.children.empty()) {
            add_geometries_to_nodes(node.children, theGraph);
        }
    }
}

// Function to compute the transformation matrix for a given assembly instance
gp_Trsf GetTransformationMatrix(
    const Handle(StepRepr_NextAssemblyUsageOccurrence)& nauo,
    const Interface_Graph& theGraph)
{
    gp_Trsf transformation;
    Handle(StepBasic_ProductDefinition) relatedProdDef = nauo->RelatedProductDefinition();

    // Check if the NAUO provides transformation data
    if (!relatedShape.IsNull())
    {
        Interface_EntityIterator refs = theGraph.Sharings(relatedShape);
        for (refs.Start(); refs.More(); refs.Next())
        {
            const Handle(Standard_Transient) refEntity = refs.Value();
            if (refEntity->IsKind(STANDARD_TYPE(StepGeom_Axis2Placement3d)))
            {
                auto axisPlacement = Handle(StepGeom_Axis2Placement3d)::DownCast(refEntity);

                // Extract location, direction, and orientation to create the transformation
                if (!axisPlacement->Location().IsNull())
                {
                    auto location = axisPlacement->Location();
                    transformation.SetTranslation(
                        gp_Vec(location->CoordinatesValue(1), location->CoordinatesValue(2), location->CoordinatesValue(3)));
                }

                if (!axisPlacement->RefDirection().IsNull() &&
                    !axisPlacement->Axis().IsNull())
                {
                    auto refDirection = axisPlacement->RefDirection();
                    auto axis = axisPlacement->Axis();
                    auto angle = refDirection->Angle(axis);

                    transformation.SetRotation(
                        gp_Ax1(gp_Pnt(0, 0, 0),
                               gp_Dir(axis->DirectionRatiosValue(1), axis->DirectionRatiosValue(2), axis->DirectionRatiosValue(3))),
                        angle);
                }
                break;
            }
        }
    }
    return transformation;
}

// Modified BuildProductNode to include transformation data
static ProductNode BuildProductNodeWithTransform(
    int productIndex,
    const std::unordered_map<int, std::vector<int>>& parentToChildren,
    const Handle(Interface_InterfaceModel)& model,
    const gp_Trsf& parentTransform = gp_Trsf())
{
    ProductNode node;
    node.entityIndex = productIndex;

    Handle(Standard_Transient) ent = model->Value(productIndex);
    auto product = Handle(StepBasic_Product)::DownCast(ent);
    if (!product.IsNull() && !product->Name().IsNull())
    {
        node.name = product->Name()->ToCString();
    }
    else
    {
        node.name = "(unnamed product)";
    }

    // Get the transformation for this node
    gp_Trsf currentTransform = parentTransform;

    // Look for NextAssemblyUsageOccurrence related to this product
    for (Standard_Integer i = 1; i <= model->NbEntities(); i++)
    {
        Handle(Standard_Transient) entity = model->Value(i);
        if (entity->IsKind(STANDARD_TYPE(StepRepr_NextAssemblyUsageOccurrence)))
        {
            auto nauo = Handle(StepRepr_NextAssemblyUsageOccurrence)::DownCast(entity);
            if (!nauo.IsNull() && nauo->RelatedProductDefinition() == product)
            {
                gp_Trsf localTransform = GetTransformationMatrix(nauo, model);
                currentTransform.Multiply(localTransform);
                break;
            }
        }
    }

    // Store transformation in the node
    node.transformation = currentTransform; // Assuming ProductNode has a `gp_Trsf` member

    // Recurse for children
    auto it = parentToChildren.find(productIndex);
    if (it != parentToChildren.end())
    {
        for (int childIdx : it->second)
        {
            node.children.push_back(
                BuildProductNodeWithTransform(childIdx, parentToChildren, model, currentTransform));
        }
    }
    return node;
}