#ifndef ADA_CPP_GEOMETRIES_H
#define ADA_CPP_GEOMETRIES_H

#include <CGAL/Simple_cartesian.h>
#include "../binding_core.h"

typedef CGAL::Simple_cartesian<double> Kernel;
typedef Kernel::Point_3 Point_3;
typedef Kernel::Vector_3 Vector_3;

struct Shape {
    // create a global unique int id for each shape
    int id;
    // upon instantiation of a shape, increment the global id
    static int next_id;

    Shape() : id(next_id++) {}
};

struct Box : Shape {
    Point_3 origin;
    double width;
    double length;
    double height;
public:
    Box(const std::vector<double> &origin, double width, double length, double height) : origin(origin[0], origin[1],
                                                                                                origin[2]),
                                                                                         width(width), length(length),
                                                                                         height(height) {}
};

void shape_module(nb::module_ &m);

#endif //ADA_CPP_GEOMETRIES_H
