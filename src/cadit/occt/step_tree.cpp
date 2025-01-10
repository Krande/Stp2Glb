#include <Interface_InterfaceModel.hxx>
#include <Interface_Graph.hxx>
#include <Interface_EntityIterator.hxx>

// STEP entity classes
#include <StepBasic_Product.hxx>
#include <StepBasic_ProductDefinition.hxx>
#include <StepBasic_ProductDefinitionFormation.hxx>
#include <StepRepr_NextAssemblyUsageOccurrence.hxx>

#include <TCollection_HAsciiString.hxx>
#include <Standard_Type.hxx>
#include <Standard_Transient.hxx>

#include <unordered_map>
#include <unordered_set>
#include <queue>

#include <sstream>
#include "step_tree.h"


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
