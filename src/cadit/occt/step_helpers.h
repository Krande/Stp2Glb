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

std::string getStepProductName(const Handle(Standard_Transient) &entity, Interface_Graph &theGraph);

void update_location(TopoDS_Shape &shape);

std::string get_name(const Handle(StepRepr_RepresentationItem) &repr_item);

TopoDS_Shape make_shape(const Handle(StepShape_Face) &face, STEPControl_Reader &reader);

bool add_shape_to_document(const TopoDS_Shape &shape, const std::string &name,
                           const Handle(XCAFDoc_ShapeTool) &shape_tool, IMeshTools_Parameters &meshParams);

TopoDS_Shape entity_to_shape(const Handle(Standard_Transient) &entity,
                             STEPControl_Reader default_reader,
                             const Handle(XCAFDoc_ShapeTool) &shape_tool,
                             IMeshTools_Parameters &meshParams,
                             const bool solid_only = false);

#endif //STEP_HELPERS_H
