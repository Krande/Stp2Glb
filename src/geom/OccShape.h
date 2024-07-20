//
// Created by Kristoffer on 07/05/2023.
//

#ifndef NANO_OCCT_OCCSHAPE_H
#define NANO_OCCT_OCCSHAPE_H

#include <optional>
#include <vector>
#include <TopoDS_Shape.hxx>
#include "Color.h"


class OccShape {
public:
    explicit OccShape(TopoDS_Shape shape,
                      Color color = Color(),
                      int num_tot_entities = 0,
                      std::optional<std::string> name = std::nullopt);

    TopoDS_Shape shape;
    Color color;
    int num_tot_entities;
    std::optional<std::string> name;
};

#endif //NANO_OCCT_OCCSHAPE_H
