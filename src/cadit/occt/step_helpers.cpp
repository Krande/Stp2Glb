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
#include <StepShape_ShapeRepresentation.hxx>
#include <StepRepr_ProductDefinitionShape.hxx>
#include <StepShape_ShapeDefinitionRepresentation.hxx>
#include <StepBasic_ProductDefinition.hxx>
#include <StepBasic_ProductDefinitionFormation.hxx>
#include <StepBasic_Product.hxx>
#include <StepRepr_Representation.hxx>
#include <TCollection_HAsciiString.hxx>

Handle(Standard_Transient) get_entity_from_graph_path(const Handle(Standard_Transient) &entity,
                                                      Interface_Graph &theGraph, std::vector<std::string> path) {
    // find index of entity in path
    auto index = std::find(path.begin(), path.end(), entity->DynamicType()->Name());
    // if entity is last item in path, return it
    if (index == path.end() - 1) {
        return entity;
    }
    // if not last item, find the next entity in the path
    auto target_type = path[index - path.begin() + 1];

    auto parents = theGraph.Sharings(entity);

    while (parents.More()) {
        Handle(Standard_Transient) parent = parents.Value();
        auto parent_name = parent->DynamicType()->Name();
        if (parent_name == target_type) {
            return get_entity_from_graph_path(parent, theGraph, path);
        }
    }
    return {};
}

std::string extractProductNameFromSDR(const Handle(StepShape_ShapeDefinitionRepresentation)& sdr)
{
    if (sdr.IsNull()) {
        return {};
    }

    // 1) sdr->Definition() returns a "StepRepr_RepresentedDefinition" (by value)
    StepRepr_RepresentedDefinition def = sdr->Definition();
    if (def.Value().IsNull()) {
        // Nothing linked
        return {};
    }

    // 2) The underlying handle is def.Value(). We expect it might be a StepBasic_ProductDefinition
    Handle(StepRepr_ProductDefinitionShape) pds =
        Handle(StepRepr_ProductDefinitionShape)::DownCast(def.Value());
    if (pds.IsNull()) {
        // It's not a product definition. Could be something else.
        return {};
    }

    StepRepr_CharacterizedDefinition cd = pds->Definition();
    if (cd.IsNull()) {
        return {};
    }

    // 3) Now from the product definition, we can get the formation -> product
    Handle(StepBasic_ProductDefinition) pd = cd.ProductDefinition();
    if (pd.IsNull()) {
        return {};
    }

    // 3) Now from the product definition, we can get the formation -> product
    Handle(StepBasic_ProductDefinitionFormation) pdf = pd->Formation();
    if (pdf.IsNull()) {
        return {};
    }

    Handle(StepBasic_Product) product = pdf->OfProduct();
    if (product.IsNull() || product->Name().IsNull()) {
        return {};
    }

    return product->Name()->ToCString();
}

std::string getStepProductNameFromGraph(const Handle(Standard_Transient) &entity, Interface_Graph &theGraph) {
    // Relationship tree
    // StepShape_ManifoldSolidBrep -> SHAPE_REPRESENTATION -> ShapeDefinitionRepresentation
    // 1) Get the model index for this entity
    std::vector<std::string> path = {
        "StepShape_ManifoldSolidBrep", "StepShape_AdvancedBrepShapeRepresentation", "StepShape_ShapeDefinitionRepresentation"
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

    if (!shape_def_rep.IsNull()) {
        auto productName = extractProductNameFromSDR(Handle(StepShape_ShapeDefinitionRepresentation)::DownCast(shape_def_rep));
        if (!productName.empty()) {
            return productName;
        }
    }



    return "";
}

//--------------------------------------
// Extract product name from a single entity
//--------------------------------------
std::string getStepProductName(const Handle(Standard_Transient) &entity, Interface_Graph &theGraph) {
    // ----------------------------------------------------------
    // 1) StepShape_ShapeDefinitionRepresentation: might link to a ProductDefinition
    // ----------------------------------------------------------
    {
        Handle(StepShape_ShapeDefinitionRepresentation) sdr =
                Handle(StepShape_ShapeDefinitionRepresentation)::DownCast(entity);
        if (!sdr.IsNull()) {
            // sdr->Definition() returns a by-value StepRepr_RepresentedDefinition
            StepRepr_RepresentedDefinition def = sdr->Definition();
            if (!def.Value().IsNull()) {
                // Try cast to StepBasic_ProductDefinition
                Handle(StepBasic_ProductDefinition) pd =
                        Handle(StepBasic_ProductDefinition)::DownCast(def.Value());
                if (!pd.IsNull()) {
                    // ProductDefinitionFormation -> OfProduct
                    if (!pd->Formation().IsNull()) {
                        Handle(StepBasic_Product) product = pd->Formation()->OfProduct();
                        if (!product.IsNull() && !product->Name().IsNull()) {
                            std::string result = product->Name()->ToCString();
                            if (!result.empty()) {
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
        if (!shapeRepr.IsNull()) {
            // shapeRepr inherits from StepRepr_Representation, which has a Name()
            if (!shapeRepr->Name().IsNull()) {
                std::string result = shapeRepr->Name()->ToCString();
                if (!result.empty()) {
                    return result;
                }
            }
        } else {
            // Maybe it's a plain StepRepr_Representation
            Handle(StepRepr_Representation) repr =
                    Handle(StepRepr_Representation)::DownCast(entity);
            if (!repr.IsNull()) {
                if (!repr->Name().IsNull()) {
                    std::string result = repr->Name()->ToCString();
                    if (!result.empty()) {
                        return result;
                    }
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
        if (!item.IsNull()) {
            if (!item->Name().IsNull()) {
                std::string result = item->Name()->ToCString();
                if (!result.empty()) {
                    return result;
                }
            }
        }
    }
    {
        auto result = getStepProductNameFromGraph(entity, theGraph);
        if (!result.empty()) {
            return result;
        }
    }
    // If everything fails or is empty, we return empty string
    return {};
}

void update_location(TopoDS_Shape &shape) {
    TopLoc_Location loc = shape.Location();
    // Extract translation components from the transformation matrix
    gp_XYZ translation = loc.Transformation().TranslationPart();
    Standard_Real x = translation.X();
    Standard_Real y = translation.Y();
    Standard_Real z = translation.Z();

    // Output the coordinates
    std::cout << "X: " << x << ", Y: " << y << ", Z: " << z << "\n";

    if (!loc.IsIdentity()) {
        shape.Location(loc); // Apply the transformation
    }
}

std::string get_name(const Handle(StepRepr_RepresentationItem) &repr_item) {
    if (!repr_item.IsNull()) {
        auto name = repr_item->Name()->ToCString();

        if (name) {
            return name;
        }
        return "Unnamed";
    }
    return {};
}

TopoDS_Shape make_shape(const Handle(StepShape_SolidModel) &solid_model, STEPControl_Reader &reader) {
    if (!solid_model.IsNull()) {
        TIME_BLOCK("Transferring solid model entity");
        // Convert the solid model into an OpenCascade shape
        if (!reader.TransferEntity(solid_model)) {
            std::cerr << "Error transferring entity" << std::endl;
        };
        TopoDS_Shape shape = reader.Shape(reader.NbShapes());

        // Apply the location (transformation) to the shape
        update_location(shape);
        return shape;
    }
    return {};
}

TopoDS_Shape make_shape(const Handle(StepShape_Face) &face, STEPControl_Reader &reader) {
    if (!face.IsNull()) {
        TIME_BLOCK("Transferring face entity");
        if (!reader.TransferEntity(face)) {
            std::cerr << "Error transferring face entity" << std::endl;
        }

        TopoDS_Shape shape = reader.Shape(reader.NbShapes());


        // Apply the location (transformation) to the shape
        update_location(shape);
        return shape;
    }
    return {};
}

bool add_shape_to_document(const TopoDS_Shape &shape, const std::string &name,
                           const Handle(XCAFDoc_ShapeTool) &shape_tool, IMeshTools_Parameters &meshParams) {
    if (!shape.IsNull()) {
        {
            TIME_BLOCK("Applying tessellation");
            // Perform tessellation (discretization) on the shape
            BRepMesh_IncrementalMesh mesh(shape, meshParams); // Adjust 0.1 for finer/coarser tessellation
        }

        // Add the TopoDS_Shape to the document
        {
            TIME_BLOCK("Adding shape '" + name + "' to document\n");
            TDF_Label shape_label = shape_tool->AddShape(shape);
            TDataStd_Name::Set(shape_label, name.c_str());

            if (!shape_label.IsNull()) {
                std::cout << "Shape added to the document successfully!" << std::endl;
                return true;
            } else {
                std::cerr << "Failed to add shape to the document." << std::endl;
                return false;
            }
        }
    }
    return false;
}

TopoDS_Shape entity_to_shape(const Handle(Standard_Transient) &entity,
                             STEPControl_Reader default_reader,
                             const Handle(XCAFDoc_ShapeTool) &shape_tool,
                             IMeshTools_Parameters &meshParams,
                             const bool solid_only) {
    const Handle(Standard_Type) type = entity->DynamicType();
    bool added_to_model = false;
    TopoDS_Shape shape;
    // Check if the entity is a solid model
    if (entity->IsKind(STANDARD_TYPE(StepShape_SolidModel))) {
        Handle(StepShape_SolidModel) solid_model = Handle(StepShape_SolidModel)::DownCast(entity);
        shape = make_shape(solid_model, default_reader);
        auto name = get_name(solid_model);
        if (add_shape_to_document(shape, name, shape_tool, meshParams))
            added_to_model = true;
    }
    // Check if the entity is a face (StepShape_AdvancedFace or StepShape_Face)
    if (entity->IsKind(STANDARD_TYPE(StepShape_AdvancedFace)) || entity->IsKind(
            STANDARD_TYPE(StepShape_Face))) {
        if (solid_only) {
            return shape;
        }
        Handle(StepShape_Face) face = Handle(StepShape_Face)::DownCast(entity);
        shape = make_shape(face, default_reader);
        auto name = get_name(face);
        if (add_shape_to_document(shape, name, shape_tool, meshParams))
            added_to_model = true;
    }
    return shape;
}
