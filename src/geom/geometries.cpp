#include "geometries.h"

//
// Created by Kristoffer on 30/07/2023.
//
int Shape::next_id = 0;

void shape_module(nb::module_ &m) {
    nb::class_<Shape>(m, "Shape")
            .def_ro("id", &Shape::id);

    nb::class_<Box, Shape>(m, "Box")
            .def(nb::init<std::vector<double>, double, double, double>())
            .def_rw("origin", &Box::origin)
            .def_rw("width", &Box::width)
            .def_rw("length", &Box::length)
            .def_rw("height", &Box::height);
};