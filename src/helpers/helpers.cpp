
#include <vector>
#include <random>
#include <gp_XYZ.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Solid.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <TDF_Label.hxx>
#include <TDataStd_Name.hxx>
#include <Quantity_Color.hxx>
#include <XCAFDoc_ColorType.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <optional>
#include "../geom/Color.h"


TopoDS_Solid create_box(const std::vector<float> &box_origin, const std::vector<float> &box_dims) {
    gp_Pnt aBoxOrigin(box_origin[0], box_origin[1], box_origin[2]);
    gp_XYZ aBoxDims(box_dims[0], box_dims[1], box_dims[2]);

    return BRepPrimAPI_MakeBox(aBoxOrigin, aBoxDims.X(), aBoxDims.Y(), aBoxDims.Z());
}

// a function that returns a random tuple std::tuple<double, double, double>
Color random_color() {
    static std::mt19937 generator(std::random_device{}());
    static std::uniform_real_distribution<float> distribution(0.0, 1.0);
    return Color(distribution(generator), distribution(generator), distribution(generator));
}

void set_name(const TDF_Label &label, const std::optional<std::string> &name) {
    if (!name) {
        return;
    }
    TCollection_ExtendedString ext_name(name->c_str());
    TDataStd_Name::Set(label, ext_name);
}

void set_color(const TDF_Label &label, const Color &color,
               const Handle(XCAFDoc_ColorTool) &tool) {
    float r = color.r;
    float g = color.g;
    float b = color.b;

    Quantity_Color qty_color(r, g, b, Quantity_TOC_RGB);
    tool->SetColor(label, qty_color, XCAFDoc_ColorType::XCAFDoc_ColorSurf);
}
