//
// Created by ofskrand on 07.01.2025.
//

#ifndef STEP_HELPERS_H
#define STEP_HELPERS_H

#include <STEPCAFControl_Reader.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <Interface_Graph.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <StepShape_SolidModel.hxx>
#include <StepShape_Face.hxx>
#include <iostream>
#include <string>
#include <TopoDS_Shape.hxx> // Include the necessary OpenCascade header for TopoDS_Shape


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

    // Optional: Member functions
    void printDetails() const;
};


ConvertObject entity_to_shape(const Handle(Standard_Transient) &entity,
                             STEPControl_Reader default_reader,
                             const Handle(XCAFDoc_ShapeTool) &shape_tool,
                             IMeshTools_Parameters &meshParams,
                             const bool solid_only = false);

std::string getStepProductNameFromGraph(const Handle(Standard_Transient) &entity, Interface_Graph &theGraph);

#endif //STEP_HELPERS_H
