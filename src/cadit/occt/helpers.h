#ifndef NANO_OCCT_HELPERS_H
#define NANO_OCCT_HELPERS_H

#include <vector>
#include <TopoDS_Solid.hxx>
#include <TDF_Label.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <optional>
#include "../../geom/Color.h"


TopoDS_Solid create_box(const std::vector<float> &box_origin, const std::vector<float> &box_dims);

Color random_color();

void set_name(const TDF_Label &label, const std::optional<std::string> &name);

void set_color(const TDF_Label &label, const Color &color,
               const Handle(XCAFDoc_ColorTool) &tool);

#endif // NANO_OCCT_HELPERS_H
