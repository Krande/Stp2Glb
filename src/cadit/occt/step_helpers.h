//
// Created by ofskrand on 07.01.2025.
//

#ifndef STEP_HELPERS_H
#define STEP_HELPERS_H

#include <STEPCAFControl_Reader.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <StepBasic_Product.hxx>
#include <Interface_Graph.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <StepShape_SolidModel.hxx>
#include <StepShape_Face.hxx>
#include <string>
#include <TopoDS_Shape.hxx> // Include the necessary OpenCascade header for TopoDS_Shape
#include "custom_progress.h"

std::string getStepProductName(const Handle(Standard_Transient) &entity, Interface_Graph &theGraph);

void update_location(TopoDS_Shape &shape);

std::string get_name(const Handle(StepRepr_RepresentationItem) &repr_item);

TopoDS_Shape make_shape(const Handle(StepShape_Face) &face, STEPControl_Reader &reader);

TDF_Label add_shape_to_document(const TopoDS_Shape &shape, const std::string &name,
                           const Handle(XCAFDoc_ShapeTool) &shape_tool, IMeshTools_Parameters &meshParams);

struct ConvertObject {
    std::string name;        // Name of the object
    TopoDS_Shape shape;      // Shape data
    bool AddedToModel = false; // Indicates if the object is added to the model
    TDF_Label shape_label;   // Label for the shape in the document

    // Constructor
    ConvertObject(const std::string& name, const TopoDS_Shape& , TDF_Label& shape_label, bool addedToModel = false);

    // Optional: Destructor
    ~ConvertObject();
};


ConvertObject entity_to_shape(const Handle(Standard_Transient) &entity,
                             STEPControl_Reader default_reader,
                             const Handle(XCAFDoc_ShapeTool) &shape_tool,
                             IMeshTools_Parameters &meshParams,
                             const bool solid_only = false);

std::string getStepProductNameFromGraph(const Handle(Standard_Transient) &entity, Interface_Graph &theGraph);

bool CustomFilter(const Handle(Standard_Transient)& entity);
Interface_EntityIterator MyTypedExpansions(const Handle(Standard_Transient)& rootEntity,
                                          const Handle(Standard_Type)& targetType,
                                          const Interface_Graph& theGraph);

Interface_EntityIterator Get_Associated_SolidModel_BiDirectional(
    const Handle(Standard_Transient)& rootEntity,
    const Handle(Standard_Type)& targetType,
    const Interface_Graph& theGraph);

gp_Trsf get_product_transform(TopoDS_Shape& shape, const Handle(StepBasic_Product)& product);

bool perform_tessellation_with_timeout(const TopoDS_Shape &shape, const IMeshTools_Parameters &meshParams,
                                       const int timeoutSeconds, const Handle(CustomProgressIndicator) &progress);

#endif //STEP_HELPERS_H
