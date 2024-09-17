// AdaCPPStepWriter.h

#pragma once

#include <iostream>

#include <BRep_Builder.hxx>
#include <Interface_Static.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <TDocStd_Application.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS_Compound.hxx>
#include <TDF_Label.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include "helpers.h"
#include "../../geom/Color.h"



class AdaCPPStepWriter {
public:
    explicit AdaCPPStepWriter(const std::string& top_level_name = "Assembly");

    void add_shape(const TopoDS_Shape& shape, const std::string& name,
                   Color& rgb_color,
                   const TDF_Label& parent = TDF_Label());

    void export_step(const std::filesystem::path& step_file);

private:
    // Handles (smart pointers) to OCC classes
    Handle(TDocStd_Application) app_;
    Handle(TDocStd_Document) doc_;
    Handle(XCAFDoc_ShapeTool) shape_tool_;
    Handle(XCAFDoc_ColorTool) color_tool_;

    TopoDS_Compound comp_;
    BRep_Builder comp_builder_;
    TDF_Label tll_;
};

// Function declarations
void write_boxes_to_step(const std::string& filename,
                         const std::vector<std::vector<float>>& box_origins,
                         const std::vector<std::vector<float>>& box_dims);

void write_box_to_step(const std::string& filename,
                       const std::vector<float>& box_origin,
                       const std::vector<float>& box_dims);
