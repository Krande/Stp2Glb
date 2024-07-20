#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_3.h>
#include "../../../geom/geometries.h"

typedef CGAL::Simple_cartesian<double> Kernel;
typedef Kernel::Point_3 Point_3;
typedef Kernel::Vector_3 Vector_3;
typedef CGAL::Polyhedron_3<Kernel> Polyhedron;

std::vector<Box> generate_boxes();  // Replace with your actual function.

void tessellate_boxes(const std::vector<Box>& boxes,
                      std::vector<Point_3>& positions,
                      std::vector<int>& indices) {
  for (const auto& box : boxes) {
    // Define the 8 vertices of the box.
    std::vector<Point_3> vertices = {
        box.origin,
        box.origin + Vector_3(box.width, 0, 0),
        box.origin + Vector_3(box.width, box.height, 0),
        box.origin + Vector_3(0, box.height, 0),
        box.origin + Vector_3(0, 0, box.length),
        box.origin + Vector_3(box.width, 0, box.length),
        box.origin + Vector_3(box.width, box.height, box.length),
        box.origin + Vector_3(0, box.height, box.length)
    };

    // Add vertices to the position list.
    positions.insert(positions.end(), vertices.begin(), vertices.end());

    // Define the 12 triangles of the box (2 per face).
    std::vector<std::array<int, 3>> faces = {
        {0, 1, 2}, {0, 2, 3}, // front face
        {4, 6, 5}, {4, 7, 6}, // back face
        {0, 5, 1}, {0, 4, 5}, // bottom face
        {2, 7, 3}, {2, 6, 7}, // top face
        {0, 7, 4}, {0, 3, 7}, // left face
        {1, 6, 2}, {1, 5, 6}  // right face
    };

    // Add the indices of the faces to the index list, offset by the current
    // number of vertices.
    int offset = positions.size() - vertices.size();
    for (const auto& face : faces) {
      indices.push_back(face[0] + offset);
      indices.push_back(face[1] + offset);
      indices.push_back(face[2] + offset);
    }
  }
}