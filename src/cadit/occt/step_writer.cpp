#include "step_writer.h"

#include <BRepBuilderAPI_Transform.hxx>
#include <iostream>
#include <filesystem>

#include <BRep_Builder.hxx>
#include <Interface_Static.hxx>
#include <Quantity_Color.hxx>
#include <Quantity_TypeOfColor.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDataStd_Name.hxx>
#include <TDocStd_Application.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS_Compound.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XSControl_WorkSession.hxx>
#include <TDF_Label.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include "helpers.h"
#include "step_tree.h"
#include "../../geom/Color.h"
#include <Standard_Handle.hxx>  // For Handle


// Constructor with only top-level name
AdaCPPStepWriter::AdaCPPStepWriter(const std::string &top_level_name) {
    initialize(top_level_name);
}

// Constructor with product hierarchy
AdaCPPStepWriter::AdaCPPStepWriter(const std::string &top_level_name, std::vector<std::unique_ptr<ProductNode>> &product_hierarchy) {
    initialize(top_level_name);
    create_hierarchy(product_hierarchy, tll_);
}

// Add a shape
void AdaCPPStepWriter::add_shape(const TopoDS_Shape &shape, const std::string &name,
                                 const Color &rgb_color, const ProductNode &parent_product,
                                 const TDF_Label &parent) {

    TDF_Label parent_label = entity_labels_[parent_product.entityIndex];
    if (parent_label.IsNull()) {
        throw std::runtime_error("Parent product not found: " + parent_product.name);
    }
    auto location = TopLoc_Location(parent_product.transformation);
    TDF_Label shape_label = shape_tool_->AddShape(shape, Standard_False, Standard_False);
    shape_tool_->AddComponent(parent_label, shape_label, location);

    set_color(shape_label, rgb_color, color_tool_);
    set_name(shape_label, name);
    auto new_shape = shape_tool_->GetShape(shape_label);

    // Add any additional location transformations from product to shape
    BRepBuilderAPI_Transform shapeTransform(parent_product.transformation);
    shapeTransform.Perform(new_shape, Standard_False);

}

// Export the STEP file
void AdaCPPStepWriter::export_step(const std::filesystem::path &step_file) const
{
    shape_tool_->UpdateAssemblies();

    if (!step_file.parent_path().empty() && step_file.parent_path() != "") {
        create_directories(step_file.parent_path());
    }

    const Handle(XSControl_WorkSession) session = new XSControl_WorkSession();

    STEPCAFControl_Writer writer(session, Standard_False);
    writer.SetColorMode(Standard_True);
    writer.SetNameMode(Standard_True);

    Interface_Static::SetCVal("write.step.assembly", "ON");
    Interface_Static::SetCVal("write.step.product.context", "PRODUCT");
    Interface_Static::SetCVal("write.step.unit", "m");
    Interface_Static::SetCVal("write.step.schema", "AP242");

    writer.Transfer(doc_, STEPControl_AsIs);
    const IFSelect_ReturnStatus status = writer.Write(step_file.string().c_str());

    if (status != IFSelect_RetDone) {
        throw std::runtime_error("STEP export failed");
    }

    std::cout << "STEP export status: " << status << "\n";
}

// Private initialization function
void AdaCPPStepWriter::initialize(const std::string &top_level_name) {
    app_ = new TDocStd_Application();
    doc_ = new TDocStd_Document(TCollection_ExtendedString("XmlOcaf"));
    app_->InitDocument(doc_);

    shape_tool_ = XCAFDoc_DocumentTool::ShapeTool(doc_->Main());
    XCAFDoc_ShapeTool::SetAutoNaming(false);
    color_tool_ = XCAFDoc_DocumentTool::ColorTool(doc_->Main());

    comp_builder_.MakeCompound(comp_);
    tll_ = shape_tool_->AddShape(comp_, Standard_True);

    set_name(tll_, top_level_name);

}

// Create a hierarchy of products
void AdaCPPStepWriter::create_hierarchy(std::vector<std::unique_ptr<ProductNode>> &nodes, const TDF_Label &parent_label) {
    for (auto &node : nodes) {

        TDF_Label child_label = shape_tool_->NewShape();
        auto target_label = shape_tool_->AddComponent(parent_label, child_label, TopLoc_Location());
        set_name(child_label, node->name);
        product_labels_[node->name] = child_label;
        entity_labels_[node->entityIndex] = child_label;
        node->targetIndex = target_label;

        if (!node->children.empty()) {
            create_hierarchy(node->children, child_label);
        }
    }
}

// Set name for a label
void AdaCPPStepWriter::set_name(const TDF_Label &label, const std::string &name) {
    TDataStd_Name::Set(label, TCollection_ExtendedString(name.c_str()));
}

// Set color for a label
void AdaCPPStepWriter::set_color(const TDF_Label &label, const Color &rgb_color,
                                 const Handle(XCAFDoc_ColorTool) &color_tool) {
    Quantity_Color color(rgb_color.r, rgb_color.g, rgb_color.b, Quantity_TOC_RGB);
    color_tool->SetColor(label, color, XCAFDoc_ColorGen);
}