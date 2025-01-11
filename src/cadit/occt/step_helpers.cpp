#include <STEPCAFControl_Reader.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <RWGltf_CafWriter.hxx>
#include <TDocStd_Document.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <filesystem>
#include <XCAFDoc_ShapeTool.hxx>
#include <Interface_EntityIterator.hxx>
#include <StepShape_SolidModel.hxx>
#include <StepShape_Face.hxx>
#include <StepShape_AdvancedFace.hxx>
#include <BRepBuilderAPI_MakeShape.hxx>
#include <BRep_Builder.hxx>
#include <TDataStd_Name.hxx>
#include <iostream>
#include "step_writer.h"
#include "step_helpers.h"

#include <Interface_Graph.hxx>
#include <StepGeom_Axis2Placement3D.hxx>
#include <StepGeom_CartesianPoint.hxx>
#include <StepGeom_Direction.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Trsf.hxx>
#include <StepRepr_ShapeRepresentationRelationship.hxx>
#include <StepShape_ShapeRepresentation.hxx>
#include <StepRepr_ProductDefinitionShape.hxx>
#include <StepShape_ShapeDefinitionRepresentation.hxx>
#include <StepBasic_ProductDefinition.hxx>
#include <StepBasic_ProductDefinitionFormation.hxx>
#include <StepBasic_Product.hxx>
#include <StepRepr_Representation.hxx>
#include <TCollection_HAsciiString.hxx>
#include <unordered_set>

Handle(Standard_Transient) get_entity_from_graph_path(const Handle(Standard_Transient)& entity,
                                                      Interface_Graph& theGraph, std::vector<std::string> path)
{
    // First check if the entity is contained in the path
    if (std::find(path.begin(), path.end(), entity->DynamicType()->Name()) == path.end())
    {
        return {};
    }

    // find index of entity in path
    auto index = std::find(path.begin(), path.end(), entity->DynamicType()->Name());
    // if entity is last item in path, return it
    if (index == path.end() - 1)
    {
        return entity;
    }
    // if not last item, find the next entity in the path
    auto target_type = path[index - path.begin() + 1];

    auto parents = theGraph.Sharings(entity);

    while (parents.More())
    {
        Handle(Standard_Transient) parent = parents.Value();
        auto parent_name = parent->DynamicType()->Name();
        if (parent_name == target_type)
        {
            return get_entity_from_graph_path(parent, theGraph, path);
        }
    }
    return {};
}

std::string extractProductNameFromSDR(const Handle(StepShape_ShapeDefinitionRepresentation)& sdr)
{
    if (sdr.IsNull())
    {
        return {};
    }

    // 1) sdr->Definition() returns a "StepRepr_RepresentedDefinition" (by value)
    StepRepr_RepresentedDefinition def = sdr->Definition();
    if (def.Value().IsNull())
    {
        // Nothing linked
        return {};
    }

    // 2) The underlying handle is def.Value(). We expect it might be a StepBasic_ProductDefinition
    Handle(StepRepr_ProductDefinitionShape) pds =
        Handle(StepRepr_ProductDefinitionShape)::DownCast(def.Value());
    if (pds.IsNull())
    {
        // It's not a product definition. Could be something else.
        return {};
    }

    StepRepr_CharacterizedDefinition cd = pds->Definition();
    if (cd.IsNull())
    {
        return {};
    }

    // 3) Now from the product definition, we can get the formation -> product
    Handle(StepBasic_ProductDefinition) pd = cd.ProductDefinition();
    if (pd.IsNull())
    {
        return {};
    }

    // 3) Now from the product definition, we can get the formation -> product
    Handle(StepBasic_ProductDefinitionFormation) pdf = pd->Formation();
    if (pdf.IsNull())
    {
        return {};
    }

    Handle(StepBasic_Product) product = pdf->OfProduct();
    if (product.IsNull() || product->Name().IsNull())
    {
        return {};
    }

    return product->Name()->ToCString();
}

std::string getStepProductNameFromGraph(const Handle(Standard_Transient)& entity, Interface_Graph& theGraph)
{
    // Relationship tree
    // StepShape_ManifoldSolidBrep -> SHAPE_REPRESENTATION -> ShapeDefinitionRepresentation
    // 1) Get the model index for this entity
    std::vector<std::string> path = {
        "StepShape_ManifoldSolidBrep", "StepShape_AdvancedBrepShapeRepresentation",
        "StepShape_ShapeDefinitionRepresentation"
    };
    // "StepBasic_ProductDefinition", "StepBasic_ProductDefinitionFormation", "StepBasic_Product"
    Handle(StepRepr_RepresentationItem) item =
        Handle(StepRepr_RepresentationItem)::DownCast(entity);

    auto base_type = entity->DynamicType()->Name();
    auto shape_def_rep = get_entity_from_graph_path(entity, theGraph, path);

    // get PRODUCT_DEFINITION_SHAPE from shape_def_rep
    // get PRODUCT_DEFINITION from PRODUCT_DEFINITION_SHAPE
    // get PRODUCT from PRODUCT_DEFINITION
    // get NAME from PRODUCT

    if (!shape_def_rep.IsNull())
    {
        auto productName = extractProductNameFromSDR(
            Handle(StepShape_ShapeDefinitionRepresentation)::DownCast(shape_def_rep));
        if (!productName.empty())
        {
            return productName;
        }
    }


    return "";
}

//--------------------------------------
// Extract product name from a single entity
//--------------------------------------
std::string getStepProductName(const Handle(Standard_Transient)& entity, Interface_Graph& theGraph)
{
    // ----------------------------------------------------------
    // 1) StepShape_ShapeDefinitionRepresentation: might link to a ProductDefinition
    // ----------------------------------------------------------
    {
        Handle(StepShape_ShapeDefinitionRepresentation) sdr =
            Handle(StepShape_ShapeDefinitionRepresentation)::DownCast(entity);
        if (!sdr.IsNull())
        {
            // sdr->Definition() returns a by-value StepRepr_RepresentedDefinition
            StepRepr_RepresentedDefinition def = sdr->Definition();
            if (!def.Value().IsNull())
            {
                // Try cast to StepBasic_ProductDefinition
                Handle(StepBasic_ProductDefinition) pd =
                    Handle(StepBasic_ProductDefinition)::DownCast(def.Value());
                if (!pd.IsNull())
                {
                    // ProductDefinitionFormation -> OfProduct
                    if (!pd->Formation().IsNull())
                    {
                        Handle(StepBasic_Product) product = pd->Formation()->OfProduct();
                        if (!product.IsNull() && !product->Name().IsNull())
                        {
                            std::string result = product->Name()->ToCString();
                            if (!result.empty())
                            {
                                return result;
                            }
                        }
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------
    // 2) StepRepr_ShapeRepresentation / StepRepr_Representation
    //    to see if there's a shape-level or representation-level name
    // ----------------------------------------------------------
    {
        Handle(StepShape_ShapeRepresentation) shapeRepr =
            Handle(StepShape_ShapeRepresentation)::DownCast(entity);
        if (!shapeRepr.IsNull())
        {
            // shapeRepr inherits from StepRepr_Representation, which has a Name()
            if (!shapeRepr->Name().IsNull())
            {
                std::string result = shapeRepr->Name()->ToCString();
                if (!result.empty())
                {
                    return result;
                }
            }
        }
    }
    {
        // Maybe it's a plain StepRepr_Representation
        Handle(StepRepr_Representation) repr =
            Handle(StepRepr_Representation)::DownCast(entity);
        if (!repr.IsNull())
        {
            if (!repr->Name().IsNull())
            {
                std::string result = repr->Name()->ToCString();
                if (!result.empty())
                {
                    return result;
                }
            }
        }
    }

    // ----------------------------------------------------------
    // 3) StepRepr_RepresentationItem (like faces, edges, etc.)
    // ----------------------------------------------------------
    {
        Handle(StepRepr_RepresentationItem) item =
            Handle(StepRepr_RepresentationItem)::DownCast(entity);
        if (!item.IsNull())
        {
            if (!item->Name().IsNull())
            {
                std::string result = item->Name()->ToCString();
                if (!result.empty())
                {
                    return result;
                }
            }
        }
    }

    // If everything fails or is empty, we return empty string
    return {};
}

void update_location(TopoDS_Shape& shape)
{
    TopLoc_Location loc = shape.Location();

    // Extract translation components from the transformation matrix
    gp_XYZ translation = loc.Transformation().TranslationPart();
    Standard_Real x = translation.X();
    Standard_Real y = translation.Y();
    Standard_Real z = translation.Z();

    // Output the coordinates
    std::cout << "X: " << x << ", Y: " << y << ", Z: " << z << "\n";

    if (!loc.IsIdentity())
    {
        shape.Location(loc); // Apply the transformation
    }
}

gp_Trsf get_product_transform(TopoDS_Shape& shape, const Handle(StepBasic_Product)& product)
{
    gp_Trsf transform;

    // Helper lambda to extract gp_Trsf from Axis2Placement3D
    auto extract_transform_from_placement = [](const Handle(StepGeom_Axis2Placement3d)& placement) -> gp_Trsf
    {
        gp_Trsf trsf;
        if (!placement.IsNull())
        {
            gp_Pnt location(
                placement->Location()->CoordinatesValue(1),
                placement->Location()->CoordinatesValue(2),
                placement->Location()->CoordinatesValue(3));

            gp_Dir zDir(
                placement->Axis()->DirectionRatios()->Value(1),
                placement->Axis()->DirectionRatios()->Value(2),
                placement->Axis()->DirectionRatios()->Value(3));

            gp_Dir xDir(
                placement->RefDirection()->DirectionRatios()->Value(1),
                placement->RefDirection()->DirectionRatios()->Value(2),
                placement->RefDirection()->DirectionRatios()->Value(3));

            gp_Ax3 ax3(location, zDir, xDir);
            trsf.SetTransformation(ax3);
        }
        return trsf;
    };

    // Traverse the STEP structure to accumulate transformations
    Handle(StepRepr_ProductDefinitionShape) prodDefShape;
    // Get FrameOfReference from Product
    Handle(StepRepr_Representation) representation;
    if (product->NbFrameOfReference() > 0)
    {
        // Assuming FrameOfReference() returns a list of StepBasic_ProductContext
        const auto& frameOfReference = product->FrameOfReference();
        for (Standard_Integer i = 1; i <= frameOfReference->Length(); ++i)
        {
            Handle(StepBasic_ProductContext) productContext = frameOfReference->Value(i);
            if (!productContext.IsNull())
            {
                // Retrieve the Representation (if applicable)
                Handle(StepRepr_Representation) rep =
                    Handle(StepRepr_Representation)::DownCast(productContext);
                if (!rep.IsNull())
                {
                    representation = rep;
                    break; // Use the first valid representation
                }
            }
        }
    }

    if (!representation.IsNull())
    {
        // Iterate over items in the representation to find transformations
        for (Standard_Integer i = 1; i <= representation->NbItems(); ++i)
        {
            Handle(Standard_Transient) item = representation->ItemsValue(i);

            // If the item is a ShapeRepresentation, get its transformation
            if (item->IsKind(STANDARD_TYPE(StepShape_ShapeRepresentation)))
            {
                Handle(StepShape_ShapeRepresentation) shapeRep =
                    Handle(StepShape_ShapeRepresentation)::DownCast(item);

                for (Standard_Integer j = 1; j <= shapeRep->NbItems(); ++j)
                {
                    Handle(Standard_Transient) subItem = shapeRep->ItemsValue(j);

                    if (subItem->IsKind(STANDARD_TYPE(StepGeom_Axis2Placement3d)))
                    {
                        Handle(StepGeom_Axis2Placement3d) axisPlacement =
                            Handle(StepGeom_Axis2Placement3d)::DownCast(subItem);

                        transform = transform * extract_transform_from_placement(axisPlacement);
                    }
                }
            }
        }
    }

    // Apply the accumulated transform to the shape
    {
        BRepBuilderAPI_Transform shapeTransform(transform);
    }

    return transform;
}

std::string get_name(const Handle(StepRepr_RepresentationItem)& repr_item)
{
    if (!repr_item.IsNull())
    {
        auto name = repr_item->Name()->ToCString();

        if (name)
        {
            return name;
        }
        return "Unnamed";
    }
    return {};
}

TopoDS_Shape make_shape(const Handle(StepShape_SolidModel)& solid_model, STEPControl_Reader& reader)
{
    if (!solid_model.IsNull())
    {
        TIME_BLOCK("Transferring solid model entity");
        // Convert the solid model into an OpenCascade shape
        if (!reader.TransferEntity(solid_model))
        {
            std::cerr << "Error transferring entity" << std::endl;
        };
        TopoDS_Shape shape = reader.Shape(reader.NbShapes());

        // Todo: traverse parent objects to resolve affected transformations applied indirectly to this shape
        return shape;
    }
    return {};
}

TopoDS_Shape make_shape(const Handle(StepShape_Face)& face, STEPControl_Reader& reader)
{
    if (!face.IsNull())
    {
        TIME_BLOCK("Transferring face entity");
        if (!reader.TransferEntity(face))
        {
            std::cerr << "Error transferring face entity" << std::endl;
        }

        TopoDS_Shape shape = reader.Shape(reader.NbShapes());


        return shape;
    }
    return {};
}

TDF_Label add_shape_to_document(const TopoDS_Shape& shape, const std::string& name,
                                const Handle(XCAFDoc_ShapeTool)& shape_tool, IMeshTools_Parameters& meshParams)
{
    TDF_Label shape_label;

    if (!shape.IsNull())
    {
        {
            TIME_BLOCK("Applying tessellation");
            // Perform tessellation (discretization) on the shape
            BRepMesh_IncrementalMesh mesh(shape, meshParams); // Adjust 0.1 for finer/coarser tessellation
        }

        // Add the TopoDS_Shape to the document
        {
            TIME_BLOCK("Adding shape '" + name + "' to document\n");
            shape_label = shape_tool->AddShape(shape);
        }
    }
    return shape_label;
}

// Constructor
ConvertObject::ConvertObject(const std::string& name, const TopoDS_Shape& shape, TDF_Label& shape_label,
                             bool addedToModel)
    : name(name), shape(shape), shape_label(shape_label), AddedToModel(addedToModel)
{
}

// Destructor (optional if needed)
ConvertObject::~ConvertObject()
{
    // Clean up resources if necessary
}

// Member function implementation
void ConvertObject::printDetails() const
{
    std::cout << "Name: " << name << "\n"
        << "Added to Model: " << (AddedToModel ? "Yes" : "No") << std::endl;
    // Note: Printing the shape details requires appropriate handling as TopoDS_Shape doesn't have a simple string representation
}

ConvertObject entity_to_shape(const Handle(Standard_Transient)& entity,
                              STEPControl_Reader default_reader,
                              const Handle(XCAFDoc_ShapeTool)& shape_tool,
                              IMeshTools_Parameters& meshParams,
                              const bool solid_only)
{
    const Handle(Standard_Type) type = entity->DynamicType();
    bool added_to_model = false;
    TopoDS_Shape shape;
    std::string name;
    TDF_Label shape_label;

    // Check if the entity is a solid model
    if (entity->IsKind(STANDARD_TYPE(StepShape_SolidModel)))
    {
        Handle(StepShape_SolidModel) solid_model = Handle(StepShape_SolidModel)::DownCast(entity);
        shape = make_shape(solid_model, default_reader);
        name = get_name(solid_model);
    }

    // Check if the entity is a face (StepShape_AdvancedFace or StepShape_Face)
    if (entity->IsKind(STANDARD_TYPE(StepShape_AdvancedFace)) || entity->IsKind(
        STANDARD_TYPE(StepShape_Face)))
    {
        if (!solid_only)
        {
            Handle(StepShape_Face) face = Handle(StepShape_Face)::DownCast(entity);
            shape = make_shape(face, default_reader);
            name = get_name(face);
        }
    }

    return {name, shape, shape_label, added_to_model};
}

// Custom filter function (example: filter by entity type or name)
bool CustomFilter(const Handle(Standard_Transient)& entity)
{
    if (entity->IsKind(STANDARD_TYPE(StepBasic_ProductDefinition)))
    {
        return true; // Matches filter
    }
    return false;
}

Interface_EntityIterator MyTypedExpansions(const Handle(Standard_Transient)& rootEntity,
                                           const Handle(Standard_Type)& targetType,
                                           const Interface_Graph& theGraph)
{
    // Prepare an empty sequence to accumulate matching entities
    Handle(TColStd_HSequenceOfTransient) matchedEntities =
        new TColStd_HSequenceOfTransient();

    if (rootEntity.IsNull())
    {
        // Return an empty iterator
        return {matchedEntities};
    }

    // We'll need the model to get entity indices (theGraph.Model()->Number())
    Handle(Interface_InterfaceModel) model = theGraph.Model();
    if (model.IsNull())
    {
        // Return empty if no model
        return {matchedEntities};
    }

    // BFS queue
    std::queue<Handle(Standard_Transient)> toVisit;
    // track visited with a set of entity indices to avoid loops
    std::unordered_set<Standard_Integer> visited;

    // Helper to enqueue unvisited
    auto enqueueIfNotVisited = [&](const Handle(Standard_Transient)& ent)
    {
        if (ent.IsNull()) return;
        // theGraph.Model()->Number(ent) gives 1-based index in the model
        Standard_Integer idx = model->Number(ent);
        // Only enqueue if it's in the model and hasn't been visited
        if (idx > 0 && visited.find(idx) == visited.end())
        {
            visited.insert(idx);
            toVisit.push(ent);
        }
    };

    // Initialize BFS with the root
    enqueueIfNotVisited(rootEntity);

    // BFS loop
    while (!toVisit.empty())
    {
        Handle(Standard_Transient) current = toVisit.front();
        toVisit.pop();

        // If current is or derives from targetType, record it
        if (current->IsKind(targetType))
        {
            matchedEntities->Append(current);
        }

        // Get child references using Sharings(...) = "downstream" references
        // (If you need “upstream,” you’d use Shareds(...).)
        Interface_EntityIterator sharings = theGraph.Sharings(current);
        for (sharings.Start(); sharings.More(); sharings.Next())
        {
            enqueueIfNotVisited(sharings.Value());
        }
    }

    // Build an Interface_EntityIterator from the matched sequence
    return {matchedEntities};
}

// A BFS that visits both Sharings (downstream references) and Shareds (upstream references).
// This ensures we don't miss geometry that might only be discovered by climbing "up"
// to a higher-level entity, then going "down" again.
Interface_EntityIterator Get_Associated_SolidModel_BiDirectional(
    const Handle(Standard_Transient)& rootEntity,
    const Handle(Standard_Type)& targetType,
    const Interface_Graph& theGraph)
{
    // Where we'll store any entity matching `targetType` that we encounter.
    Handle(TColStd_HSequenceOfTransient) matchedEntities = new TColStd_HSequenceOfTransient();

    if (rootEntity.IsNull())
    {
        return {matchedEntities};
    }

    // We'll need the model to get entity indices.
    Handle(Interface_InterfaceModel) model = theGraph.Model();
    if (model.IsNull())
    {
        return {matchedEntities};
    }

    std::queue<Handle(Standard_Transient)> toVisit;
    std::unordered_set<Standard_Integer> visited; // track visited by their 1-based model index

    // Helper to enqueue an entity if not already visited
    auto enqueueIfNotVisited = [&](const Handle(Standard_Transient)& ent)
    {
        if (!ent.IsNull())
        {
            Standard_Integer idx = model->Number(ent);
            if (idx > 0 && visited.find(idx) == visited.end())
            {
                visited.insert(idx);
                toVisit.push(ent);
            }
        }
    };

    // Start BFS from the root
    enqueueIfNotVisited(rootEntity);

    // BFS loop
    while (!toVisit.empty())
    {
        Handle(Standard_Transient) current = toVisit.front();
        toVisit.pop();

        // Gather children (Sharings): "downstream" references
        {
            Interface_EntityIterator childIter = theGraph.Sharings(current);
            for (childIter.Start(); childIter.More(); childIter.Next())
            {
                enqueueIfNotVisited(childIter.Value());
            }
        }

        // Gather parents (Shareds): "upstream" references
        {
            // Traverse SHAPE_REPRESENTATION_RELATIONSHIP to follow links
            if (current->IsKind(STANDARD_TYPE(StepRepr_ShapeRepresentationRelationship)))
            {
                Handle(StepRepr_ShapeRepresentationRelationship) relationship =
                    Handle(StepRepr_ShapeRepresentationRelationship)::DownCast(current);

                auto relatedRep2 = relationship->Rep2(); // Second representation

                enqueueIfNotVisited(relatedRep2);
            }
            // if iskind product, skip this
            else if (current->IsKind(STANDARD_TYPE(StepShape_ShapeDefinitionRepresentation)))
            {
                Handle(StepShape_ShapeDefinitionRepresentation) item =
                    Handle(StepShape_ShapeDefinitionRepresentation)::DownCast(current);
                auto rest = item->UsedRepresentation();
                enqueueIfNotVisited(rest);
            }
            else if (current->IsKind(STANDARD_TYPE(StepShape_ShapeRepresentation)))
            {
                Handle(StepShape_ShapeRepresentation) shapeRep =
                    Handle(StepShape_ShapeRepresentation)::DownCast(current);

                auto items = shapeRep->Items();
                if (!items.IsNull())
                {
                    Standard_Integer nbItems = items->Length();
                    for (Standard_Integer i = 1; i <= nbItems; i++)
                    {
                        // Each element is a StepRepr_RepresentationItem
                        Handle(StepRepr_RepresentationItem) repItem = items->Value(i);

                        // Check if it is a solid model or another geometric representation
                        if (repItem->IsKind(STANDARD_TYPE(StepShape_SolidModel)))
                        {
                            Handle(StepShape_SolidModel) solidModel =
                                Handle(StepShape_SolidModel)::DownCast(repItem);
                            // append only if solidmodel is not already added. Can contain many geometries
                            if (visited.find(model->Number(solidModel)) == visited.end())
                            {
                                visited.insert(model->Number(solidModel));
                                matchedEntities->Append(solidModel);
                            }
                        }
                    }
                }
            }
            else
            {
            }
        }
    }

    // Now build an iterator from the matched sequence
    return {matchedEntities};
}
