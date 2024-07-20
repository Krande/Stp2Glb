//
// Created by Kristoffer on 17/09/2023.
//

#include "colors.h"
#include <TDF_Label.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Shell.hxx>
#include <Quantity_Color.hxx>
#include <XCAFDoc_ColorTool.hxx>


void setInstanceColorIfAvailable(XCAFDoc_ColorTool *color_tool, const TDF_Label &lab, TopoDS_Shape &shape,
                                         Quantity_Color &c) {
    auto c1 = static_cast<const XCAFDoc_ColorType>(0);
    auto c2 = static_cast<const XCAFDoc_ColorType>(1);
    auto c3 = static_cast<const XCAFDoc_ColorType>(2);

    if (XCAFDoc_ColorTool::GetColor(lab, c1, c) || XCAFDoc_ColorTool::GetColor(lab, c2, c) ||
        XCAFDoc_ColorTool::GetColor(lab, c3, c)) {
        color_tool->SetInstanceColor(shape, c1, c);
        color_tool->SetInstanceColor(shape, c2, c);
        color_tool->SetInstanceColor(shape, c3, c);
    }
}

// add nanobind wrapping to the function
void occt_color_module(nb::module_ &m) {
    m.def("setInstanceColorIfAvailable", &setInstanceColorIfAvailable, "color_tool"_a, "lab"_a, "shape"_a, "c"_a,
          "sets a color");

    // TopoDS_Shape
    nanobind::class_<TopoDS_Shape>(m, "TopoDS_Shape")
            .def("get_ptr", [](TopoDS_Shape &self) {
                return reinterpret_cast<uintptr_t>(&self);
            })
            .def_static("from_ptr", [](uintptr_t ptr) {
                return reinterpret_cast<TopoDS_Shape *>(ptr);
            });

    // TopoDS_Shell
    nanobind::class_<TopoDS_Shell, TopoDS_Shape>(m, "TopoDS_Shell")
            .def("get_ptr", [](TopoDS_Shell &self) {
                return reinterpret_cast<uintptr_t>(&self);
            })
            .def_static("from_ptr", [](uintptr_t ptr) {
                return reinterpret_cast<TopoDS_Shell *>(ptr);
            });

    // XCAFDoc_ColorTool
    nanobind::class_<XCAFDoc_ColorTool>(m, "XCAFDoc_ColorTool")
            .def("get_ptr", [](XCAFDoc_ColorTool &self) {
                return reinterpret_cast<uintptr_t>(&self);
            })
            .def_static("from_ptr", [](uintptr_t ptr) {
                return reinterpret_cast<XCAFDoc_ColorTool *>(ptr);
            });

    // TDF_Label
    nanobind::class_<TDF_Label>(m, "TDF_Label")
            .def("get_ptr", [](TDF_Label &self) {
                return reinterpret_cast<uintptr_t>(&self);
            })
            .def_static("from_ptr", [](uintptr_t ptr) {
                return reinterpret_cast<TDF_Label *>(ptr);
            });

    // Quantity_Color
    nanobind::class_<Quantity_Color>(m, "Quantity_Color")
            .def("get_ptr", [](Quantity_Color &self) {
                return reinterpret_cast<uintptr_t>(&self);
            })
            .def_static("from_ptr", [](uintptr_t ptr) {
                return reinterpret_cast<Quantity_Color *>(ptr);
            });
}