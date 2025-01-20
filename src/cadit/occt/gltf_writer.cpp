//
// Created by Kristoffer on 07/05/2023.
//

#include <vector>
#include <filesystem>

#include <Message_ProgressRange.hxx>
#include <RWGltf_CafWriter.hxx>
#include <RWGltf_WriterTrsfFormat.hxx>
#include <TCollection_AsciiString.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TColStd_IndexedDataMapOfStringString.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS_Compound.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include "../../geom/OccShape.h"
#include "helpers.h"



enum class Units {
    M,
    MM,
};

void to_gltf_from_shapes(
    const std::filesystem::path& gltf_file,
    const std::vector<OccShape>& occ_shape_iterable,
    Units export_units = Units::M,
    Units source_units = Units::M
) {
    Handle(TDocStd_Document) doc = new TDocStd_Document(TCollection_ExtendedString("ada-py"));
    Handle(XCAFDoc_ShapeTool) shape_tool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    Handle(XCAFDoc_ColorTool) color_tool = XCAFDoc_DocumentTool::ColorTool(doc->Main());

    int i = 1;
    for (const auto& step_shape : occ_shape_iterable) {
        TopoDS_Shape shp = step_shape.shape;
        if (shp.ShapeType() == TopAbs_COMPOUND) {
            continue;
        }

//        Mesh mesh = tessellate_shape(shp, true, 1.0, false);

        TDF_Label sub_shape_label = shape_tool->AddShape(shp);
        set_color(sub_shape_label, step_shape.color, color_tool);
        set_name(sub_shape_label, step_shape.name);

        i++;
    }

    RWGltf_WriterTrsfFormat a_format = RWGltf_WriterTrsfFormat_Compact;

    TColStd_IndexedDataMapOfStringString a_file_info;
    a_file_info.Add(TCollection_AsciiString("Authors"), TCollection_AsciiString("ada-py"));

    bool binary = gltf_file.extension() == ".glb";

    RWGltf_CafWriter glb_writer(TCollection_AsciiString(gltf_file.string().c_str()), binary);
    if (export_units == Units::M && source_units == Units::MM) {
        glb_writer.ChangeCoordinateSystemConverter().SetInputLengthUnit(0.001);
    } else if (export_units == Units::MM && source_units == Units::M) {
        glb_writer.ChangeCoordinateSystemConverter().SetInputLengthUnit(1000);
    }

    glb_writer.ChangeCoordinateSystemConverter().SetInputCoordinateSystem(RWMesh_CoordinateSystem_Zup);
    glb_writer.SetTransformationFormat(a_format);
    Message_ProgressRange pr;
    glb_writer.Perform(doc, a_file_info, pr);
}


void to_glb_from_doc(const std::filesystem::path& glb_file, const Handle(TDocStd_Document)& doc) {
    RWGltf_CafWriter writer(glb_file.c_str(), true); // true for binary format

    // Additional file information (can be empty if not needed)
    const TColStd_IndexedDataMapOfStringString file_info;

    // Progress indicator (can be null if progress tracking is not needed)
    const Message_ProgressRange progress;

    // if output parent directory is != "" and does not exist, create it
    if (const std::filesystem::path glb_dir = glb_file.parent_path(); !glb_dir.empty() && !exists(glb_dir))
    {
        create_directories(glb_dir);
    }
    if (!writer.Perform(doc, file_info, progress))
    {
        throw std::runtime_error("Error writing GLB file");
    }
}