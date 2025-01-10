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
#include "step_tree.h"
#include "../../geom/Color.h"



class AdaCPPStepWriter {
public:
    explicit AdaCPPStepWriter(const std::string& top_level_name = "Assembly");

    AdaCPPStepWriter(const std::string& top_level_name, const std::vector<ProductNode>& product_hierarchy);

    void add_shape(const TopoDS_Shape& shape, const std::string& name, const Color& rgb_color,
        const std::string& parent_product_name = "", const TDF_Label& parent = TDF_Label());

    void export_step(const std::filesystem::path& step_file) const;


private:
    // Handles (smart pointers) to OCC classes
    Handle(TDocStd_Application) app_;
    Handle(TDocStd_Document) doc_;
    Handle(XCAFDoc_ShapeTool) shape_tool_;
    Handle(XCAFDoc_ColorTool) color_tool_;

    TopoDS_Compound comp_;
    BRep_Builder comp_builder_;
    TDF_Label tll_;

    // Map to store product name to TDF_Label mapping for hierarchy
    std::unordered_map<std::string, TDF_Label> product_labels_;

    void initialize(const std::string& top_level_name);
    void create_hierarchy(const std::vector<ProductNode>& nodes, const TDF_Label& parent_label);
    void set_name(const TDF_Label& label, const std::string& name);
    void set_color(const TDF_Label& label, const Color& rgb_color, const Handle(XCAFDoc_ColorTool)& color_tool);
};
