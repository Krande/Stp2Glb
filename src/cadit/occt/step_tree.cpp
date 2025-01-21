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
#include <StepRepr_RepresentationRelationshipWithTransformation.hxx>
#include <StepRepr_ItemDefinedTransformation.hxx>
#include <StepRepr_Transformation.hxx>
#include <StepShape_ContextDependentShapeRepresentation.hxx>
#include <unordered_map>
#include <unordered_set>
#include <queue>

#include <sstream>
#include "step_tree.h"

#include <gp_Ax1.hxx>
#include <gp_Ax3.hxx>
#include <stack>
#include <StepGeom_Axis2Placement3d.hxx>
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
        for (childIter.Start(); childIter.More(); childIter.Next())
        {
            Handle(Standard_Transient) ent = childIter.Value();
            if (ent->IsKind(STANDARD_TYPE(StepBasic_ProductDefinitionFormation)))
            {
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

// Definition of the static counter
int ProductNode::instanceCounter = 0;
// Main function: extracts top-level ProductNode trees with transformations
std::vector<std::unique_ptr<ProductNode>> ExtractProductHierarchy(const Handle(Interface_InterfaceModel)& model,
                                                 const Interface_Graph& theGraph)
{
    // 1) Build the map of parent->children relationships
    const auto parentToChildren = BuildAssemblyLinks(model, theGraph);

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
    std::vector<std::unique_ptr<ProductNode>> roots;
    for (int idx : allProducts)
    {
        if (!allChildren.contains(idx))
        {
            // This is a root product, start with the identity transformation
            roots.push_back(BuildProductNodeWithTransform(idx, parentToChildren, model, theGraph, gp_Trsf()));
        }
    }
    return roots;
}


// Helper function to serialize a gp_Trsf (4x4 transformation matrix) to JSON
static void TransformationToJson(const gp_Trsf& transform, std::ostream& os, int indentLevel)
{
    // Utility lambda to insert some indentation spaces
    auto indent = [&](int level)
    {
        for (int i = 0; i < level; i++) os << "  ";
    };

    indent(indentLevel);
    os << "\"transformation\": [\n";

    // gp_Trsf stores a 3x4 matrix (rotation and translation), so we manually append rows
    for (int i = 1; i <= 3; i++)
    {
        indent(indentLevel + 1);
        os << "[";
        for (int j = 1; j <= 4; j++) // OpenCASCADE uses 1-based indexing
        {
            os << transform.Value(i, j);
            if (j < 4) os << ", ";
        }
        os << "]";
        if (i < 3) os << ",\n";
    }

    // Add the homogeneous row for a 4x4 matrix
    os << ",\n";
    indent(indentLevel + 1);
    os << "[0, 0, 0, 1]\n";

    indent(indentLevel);
    os << "]";
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
    os << "\"InstanceIndex\": " << node.instanceIndex << ",\n";

    indent(indentLevel + 1);
    os << "\"targetIndex\": " << node.targetIndex.Tag() << ",\n";

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

    // Add the transformation matrix to the JSON output
    TransformationToJson(node.transformation, os, indentLevel + 1);
    os << ",\n";

    indent(indentLevel + 1);
    os << "\"children\": [\n";
    for (size_t i = 0; i < node.children.size(); i++)
    {
        ProductNodeToJson(*node.children[i], os, indentLevel + 2);
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


std::string ExportHierarchyToJson(const std::vector<std::unique_ptr<ProductNode>>& roots)
{
    std::ostringstream oss;
    oss << "[\n";
    for (size_t i = 0; i < roots.size(); i++)
    {
        ProductNodeToJson(*roots[i], oss, 1);
        if (i + 1 < roots.size())
        {
            oss << ",";
        }
        oss << "\n";
    }
    oss << "]\n";

    return oss.str();
}

void add_geometries_to_nodes(const std::vector<std::unique_ptr<ProductNode>>& nodes, const Interface_Graph& theGraph)
{
    for (auto& ref_node : nodes)
    {
        // 'node' is a std::unique_ptr<ProductNode>&
        if (!ref_node) {
            // If it's a null pointer, skip it
            continue;
        }
        auto& node = *ref_node;
        auto& product = theGraph.Entity(node.entityIndex);
        Interface_EntityIterator breps =
            Get_Associated_SolidModel_BiDirectional(product,
                                                    STANDARD_TYPE(StepShape_SolidModel),
                                                    theGraph);
        // get the geometry indices
        while (breps.More())
        {
            const auto& entity = breps.Value();
            auto entityIndex = theGraph.Model()->Number(entity);
            node.geometryIndices.push_back(entityIndex);
            breps.Next();
        }
        if (!node.children.empty())
        {
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

    // Get the Related ProductDefinition
    Handle(StepBasic_ProductDefinition) relatedProdDef = nauo->RelatedProductDefinition();
    if (relatedProdDef.IsNull())
    {
        return transformation; // Return identity transform if no related product definition
    }

    // Find the corresponding ProductDefinitionShape
    Handle(StepRepr_ProductDefinitionShape) relatedShape;
    Interface_EntityIterator refs = theGraph.Sharings(relatedProdDef);
    for (refs.Start(); refs.More(); refs.Next())
    {
        const Handle(Standard_Transient) refEntity = refs.Value();
        if (refEntity->IsKind(STANDARD_TYPE(StepRepr_ProductDefinitionShape)))
        {
            relatedShape = Handle(StepRepr_ProductDefinitionShape)::DownCast(refEntity);
            break;
        }
    }

    // If no ProductDefinitionShape is found, return identity transform
    if (relatedShape.IsNull())
    {
        return transformation;
    }

    // Look for an Axis2Placement3D referenced by the ProductDefinitionShape
    Interface_EntityIterator shapeRefs = theGraph.Sharings(relatedShape);
    for (shapeRefs.Start(); shapeRefs.More(); shapeRefs.Next())
    {
        const Handle(Standard_Transient) refEntity = shapeRefs.Value();
        if (refEntity->IsKind(STANDARD_TYPE(StepGeom_Axis2Placement3d)))
        {
            auto axisPlacement = Handle(StepGeom_Axis2Placement3d)::DownCast(refEntity);

            // Extract translation
            if (!axisPlacement->Location().IsNull())
            {
                auto location = axisPlacement->Location();
                transformation.SetTranslation(
                    gp_Vec(location->CoordinatesValue(1), location->CoordinatesValue(2),
                           location->CoordinatesValue(3)));
            }

            // Extract rotation
            if (!axisPlacement->RefDirection().IsNull() && !axisPlacement->Axis().IsNull())
            {
                auto refDirection = axisPlacement->RefDirection();
                auto axis = axisPlacement->Axis();
                auto angle = 0.0;

                transformation.SetRotation(
                    gp_Ax1(gp_Pnt(0, 0, 0),
                           gp_Dir(axis->DirectionRatiosValue(1), axis->DirectionRatiosValue(2),
                                  axis->DirectionRatiosValue(3))),
                    angle);
            }
            break;
        }
    }
    return transformation;
}

gp_Trsf ComputeTransformationFromAxis2Placement(const Handle(StepGeom_Axis2Placement3d)& placement)
{
    gp_Trsf transform;
    if (!placement.IsNull())
    {
        // Extract translation
        if (!placement->Location().IsNull())
        {
            const auto loc = placement->Location();
            transform.SetTranslation(gp_Vec(loc->CoordinatesValue(1), loc->CoordinatesValue(2),
                                            loc->CoordinatesValue(3)));
        }

        // Extract rotation from directions
        if (!placement->RefDirection().IsNull() && !placement->Axis().IsNull())
        {
            gp_Dir xDir(placement->Axis()->DirectionRatiosValue(1),
                        placement->Axis()->DirectionRatiosValue(2),
                        placement->Axis()->DirectionRatiosValue(3));
            gp_Dir zDir(placement->RefDirection()->DirectionRatiosValue(1),
                        placement->RefDirection()->DirectionRatiosValue(2),
                        placement->RefDirection()->DirectionRatiosValue(3));
            gp_Ax3 localAxis(transform.TranslationPart(), zDir, xDir);
            transform.SetTransformation(localAxis);
        }
    }
    return transform;
}

gp_Trsf GetTransformFromShapeRelWithTrans(
    const Handle(StepRepr_RepresentationRelationshipWithTransformation)& relWithTrans)
{
    gp_Trsf transformation;
    auto itemDefinedTrans = relWithTrans->TransformationOperator();
    if (!itemDefinedTrans.IsNull())
    {
        auto itemDefTrans = itemDefinedTrans.ItemDefinedTransformation();
        auto trans1 = itemDefTrans->TransformItem1();
        auto trans2 = itemDefTrans->TransformItem2();
        if (!trans1.IsNull() && !trans2.IsNull())
        {
            // Get AXIS2_PLACEMENT_3D entities for parent and child
            auto childPlacement = Handle(StepGeom_Axis2Placement3d)::DownCast(trans1);
            auto parentPlacement = Handle(StepGeom_Axis2Placement3d)::DownCast(trans2);

            // Compute transformations
            if (!childPlacement.IsNull() && !parentPlacement.IsNull())
            {
                gp_Trsf childTransform = ComputeTransformationFromAxis2Placement(childPlacement);
                gp_Trsf parentTransform = ComputeTransformationFromAxis2Placement(parentPlacement);

                // Combine transformations: parent^-1 * child
                parentTransform.Invert();
                transformation = parentTransform * childTransform;
            }
        }
    }
    return transformation;
}


gp_Trsf GetAssemblyInstanceTransformation(
    const Handle(StepRepr_NextAssemblyUsageOccurrence)& nauo,
    const Interface_Graph& theGraph)
{
    gp_Trsf transformation;

    const Interface_EntityIterator refs = theGraph.Sharings(nauo);
    for (refs.Start(); refs.More(); refs.Next())
    {
        const auto& entity = refs.Value();
        if (entity->IsKind(STANDARD_TYPE(StepRepr_ProductDefinitionShape)))
        {
            auto pds = Handle(StepRepr_ProductDefinitionShape)::DownCast(entity);
            Interface_EntityIterator pdsRefs = theGraph.Sharings(pds);
            for (pdsRefs.Start(); pdsRefs.More(); pdsRefs.Next())
            {
                const auto& pdsRef = pdsRefs.Value();
                if (pdsRef->IsKind(STANDARD_TYPE(StepShape_ContextDependentShapeRepresentation)))
                {
                    auto contextDependentShape =
                        Handle(StepShape_ContextDependentShapeRepresentation)::DownCast(pdsRef);
                    auto r1 = contextDependentShape->RepresentationRelation();
                    auto r2 = contextDependentShape->RepresentedProductRelation();
                    if (!r1.IsNull() && r1->
                        IsKind(STANDARD_TYPE(StepRepr_RepresentationRelationshipWithTransformation)))
                    {
                        auto relWithTrans = Handle(StepRepr_RepresentationRelationshipWithTransformation)::DownCast(r1);
                        return GetTransformFromShapeRelWithTrans(relWithTrans);
                    }
                }
            }
        }
    }

    return transformation;
}

Handle(StepRepr_NextAssemblyUsageOccurrence) Get_NextAssemblyUsageOccurrence(const Handle(StepBasic_Product)& product,
                                                                             const Interface_Graph& theGraph)
{
    Handle(StepRepr_NextAssemblyUsageOccurrence) nauo;
    std::vector<Standard_Integer> nauos;

    Interface_EntityIterator childIter = theGraph.Sharings(product);
    for (childIter.Start(); childIter.More(); childIter.Next())
    {
        if (childIter.Value()->IsKind(STANDARD_TYPE(StepBasic_ProductDefinitionFormation)))
        {
            auto pdf = Handle(StepBasic_ProductDefinitionFormation)::DownCast(childIter.Value());
            Interface_EntityIterator pdfIter = theGraph.Sharings(pdf);
            for (pdfIter.Start(); pdfIter.More(); pdfIter.Next())
            {
                if (pdfIter.Value()->IsKind(STANDARD_TYPE(StepBasic_ProductDefinition)))
                {
                    auto pd = Handle(StepBasic_ProductDefinition)::DownCast(pdfIter.Value());
                    Interface_EntityIterator pdIter = theGraph.Sharings(pd);
                    for (pdIter.Start(); pdIter.More(); pdIter.Next())
                    {
                        if (pdIter.Value()->IsKind(STANDARD_TYPE(StepRepr_NextAssemblyUsageOccurrence)))
                        {
                            nauo = Handle(StepRepr_NextAssemblyUsageOccurrence)::DownCast(pdIter.Value());
                            nauos.push_back(theGraph.EntityNumber(nauo));
                        }
                    }
                }
            }
        }
    }

    return nauo;
}

// Recursive function that builds a ProductNode tree with transformations
static std::unique_ptr<ProductNode> BuildProductNodeWithTransform(
    int productIndex,
    const std::unordered_map<int, std::vector<int>>& parentToChildren,
    const Handle(Interface_InterfaceModel)& model,
    const Interface_Graph& theGraph,
    const gp_Trsf& parentTransform = gp_Trsf(),
    ProductNode* parent)
{
    auto node = std::make_unique<ProductNode>();
    node->entityIndex = productIndex;
    node->parent = parent;

    const Handle(Standard_Transient) ent = model->Value(productIndex);
    const auto product = Handle(StepBasic_Product)::DownCast(ent);

    if (!product.IsNull() && !product->Name().IsNull())
    {
        node->name = product->Name()->ToCString();
    }
    else
    {
        node->name = "(unnamed product)";
    }

    // Compute the transformation for this node
    auto nauo = Get_NextAssemblyUsageOccurrence(product, theGraph);

    // todo: This extracts an arbitrary instance index from the graph. This needs to be fixed.
    gp_Trsf localTransform = GetAssemblyInstanceTransformation(nauo, theGraph);
    // get the entity id of nauo
    // node->instanceIndex = theGraph.Model()->Number(nauo);

    // Combine parent transformation with local transformation
    gp_Trsf absoluteTransform = parentTransform;
    absoluteTransform.Multiply(localTransform);
    node->transformation = absoluteTransform;

    // Recurse for children
    auto it = parentToChildren.find(productIndex);
    if (it != parentToChildren.end())
    {
        for (int childIdx : it->second)
        {
            node->children.push_back(
                BuildProductNodeWithTransform(childIdx, parentToChildren, model, theGraph, absoluteTransform, node.get()));
        }
    }

    return node;
}

// Iterative function
std::unique_ptr<ProductNode> BuildProductNodeWithTransformIterative(
    int rootIndex,
    const std::unordered_map<int, std::vector<int>>& parentToChildren,
    const Handle(Interface_InterfaceModel)& model,
    const Interface_Graph& theGraph,
    const gp_Trsf& rootTransform)
{
     // Create the root node
    auto rootNode = std::make_unique<ProductNode>();
    rootNode->parent = nullptr;
    rootNode->entityIndex = rootIndex;
    // We'll fill in transform/name/etc. below

    // A stack item for our DFS
    struct StackItem {
        ProductNode* node;   // pointer to the node in which we'll store data
        int productIndex;    // which product index this node corresponds to
        gp_Trsf parentTrsf;  // the parent's final transform to be combined with local transform
    };

    std::stack<StackItem> stack;
    stack.push({ rootNode.get(), rootIndex, rootTransform });

    // A set to track visited product indices
    // to avoid infinite loops if there's a cycle
    std::unordered_set<int> visited;
    visited.insert(rootIndex);

    while (!stack.empty()) {
        auto [nodePtr, productIdx, accumulatedTrsf] = stack.top();
        stack.pop();

        // --- 1) Safety checks to avoid segfaults ---

        // If productIdx is invalid (e.g. out of range for the model),
        // we skip filling data and proceed.  Depending on your data,
        // you might want to throw an exception instead.
        if (productIdx < 1 || productIdx > model->NbEntities()) {
            // Mark something or skip
            nodePtr->name = "(invalid index)";
            // nodePtr->instanceIndex = -1;
            continue;
        }

        // --- 2) Look up the product from the model ---
        Handle(Standard_Transient) ent = model->Value(productIdx);
        if (ent.IsNull()) {
            nodePtr->name = "(model->Value() returned null)";
            // nodePtr->instanceIndex = -1;
            continue;
        }
        auto product = Handle(StepBasic_Product)::DownCast(ent);
        if (product.IsNull()) {
            nodePtr->name = "(not a StepBasic_Product)";
            // nodePtr->instanceIndex = -1;
            continue;
        }

        // --- 3) Compute local transform and combine with parent's ---
        auto nauo = Get_NextAssemblyUsageOccurrence(product, theGraph);
        if (!nauo.IsNull()) {
            gp_Trsf localTransform = GetAssemblyInstanceTransformation(nauo, theGraph);

            gp_Trsf finalTransform = accumulatedTrsf;
            finalTransform.Multiply(localTransform);
            nodePtr->transformation = finalTransform;

            // Instance index from the graph
            nodePtr->instanceIndex = theGraph.Model()->Number(nauo);
        } else {
            // If NAUO is invalid for some reason
            nodePtr->transformation = accumulatedTrsf;
            // nodePtr->instanceIndex = -1;
        }

        // --- 4) Fill the node's name ---
        if (!product->Name().IsNull()) {
            nodePtr->name = product->Name()->ToCString();
        } else {
            nodePtr->name = "(unnamed product)";
        }

        // --- 5) Create child nodes and push them onto the stack ---
        auto it = parentToChildren.find(productIdx);
        if (it != parentToChildren.end()) {
            for (int childIdx : it->second) {
                // If we've already seen this index, skip to avoid cycles
                if (!visited.insert(childIdx).second) {
                    continue;
                }

                // Create a child node by value in the parent's children vector
                nodePtr->children.emplace_back();
                std::unique_ptr<ProductNode>& childRef = nodePtr->children.back();
                childRef->parent = nodePtr;
                childRef->entityIndex = childIdx;

                // We'll fill all the child details once we pop from the stack
                // For now, just push onto the stack
                stack.push({ childRef.get(), childIdx, nodePtr->transformation });
            }
        }
    }

    return rootNode;
}