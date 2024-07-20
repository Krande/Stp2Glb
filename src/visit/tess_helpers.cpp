//
// Created by Kristoffer on 07/05/2023.
//

#include <TopoDS_Shape.hxx>
#include <algorithm>
#include <memory>
#include <map>
#include "ShapeTesselator.h"
#include "tess_helpers.h"
#include "../helpers/helpers.h"


Mesh tessellate_shape(int id, const TopoDS_Shape &shape, bool compute_edges, float mesh_quality, bool parallel_meshing) {
    ShapeTesselator shape_tess(shape);
    shape_tess.Compute(compute_edges, mesh_quality, parallel_meshing);
    std::vector pos = shape_tess.GetVerticesPositionAsTuple();
    int number_of_triangles = shape_tess.ObjGetTriangleCount();

    std::vector<uint32_t> faces(number_of_triangles * 3);
    for (size_t i = 0; i < faces.size(); ++i) {
        faces[i] = static_cast<uint32_t>(i);
    }

    Mesh mesh(id, pos, faces);
    return mesh;
}


Mesh concatenate_meshes(const std::vector<std::shared_ptr<Mesh>> &meshes) {
    std::vector<GroupReference> groups;
    size_t total_positions = 0;
    size_t total_indices = 0;
    size_t total_normals = 0;
    bool has_normal = !meshes[0]->normals.empty();

    // Calculate the total number of elements in positions, indices, and normals
    for (const auto &s: meshes) {
        total_positions += s->positions.size();
        total_indices += s->indices.size();
        if (has_normal) {
            total_normals += s->normals.size();
        }
    }

    // Pre-allocate vector sizes
    std::vector<float> position_list(total_positions);
    std::vector<uint32_t> indices_list(total_indices);
    std::vector<float> normal_list(has_normal ? total_normals : 0);

    size_t position_offset = 0;
    size_t indices_offset = 0;
    size_t normal_offset = 0;
    size_t sum_positions = 0;

    for (const auto &s: meshes) {
        groups.emplace_back(s->id, indices_offset, s->indices.size());

        // Copy positions
        std::copy(s->positions.begin(), s->positions.end(), position_list.begin() + position_offset);
        position_offset += s->positions.size();

        // Copy and adjust indices
        std::transform(s->indices.begin(), s->indices.end(), indices_list.begin() + indices_offset,
                       [sum_positions](uint32_t index) { return index + sum_positions / 3; });
        indices_offset += s->indices.size();

        // Copy normals if they exist
        if (has_normal) {
            std::copy(s->normals.begin(), s->normals.end(), normal_list.begin() + normal_offset);
            normal_offset += s->normals.size();
        }

        sum_positions += s->positions.size();
    }
    Mesh merged_mesh = Mesh(meshes[0]->id,
                            position_list,
                            indices_list,
                            {},
                            has_normal ? normal_list : std::vector<float>{},
                            meshes[0]->mesh_type,
                            meshes[0]->color,
                            groups);
    return merged_mesh;
}

// take a vector of meshes, organize them by color and make a new concatenated mesh per color class
std::vector<std::shared_ptr<Mesh>> meshes_by_color(const std::vector<std::shared_ptr<Mesh>> &meshes) {
    std::vector<std::shared_ptr<Mesh>> result;
    std::map<std::vector<float>, std::vector<std::shared_ptr<Mesh>>> color_map;

    for (const auto &mesh: meshes) {
        // Create a vector that includes the RGB(A) values of the color object
        std::vector<float> color_key {mesh->color.r, mesh->color.g, mesh->color.b, mesh->color.a};

        // Use this vector as the key for the color_map
        color_map[color_key].push_back(mesh);
    }

    for (const auto &color_meshes: color_map) {
        result.push_back(std::make_shared<Mesh>(concatenate_meshes(color_meshes.second)));
    }

    return result;
}




Mesh get_box_mesh(const std::vector<float> &box_origin,
                  const std::vector<float> &box_dims) {

    TopoDS_Solid box = create_box(box_origin, box_dims);
    Mesh mesh = tessellate_shape(0, box, true, 1.0, false);
    return mesh;
}

void tess_helper_module(nb::module_ &m) {
    m.def("get_box_mesh", &get_box_mesh, "box_origin"_a, "box_dims"_a, "Write a box to a step file");

    nb::class_<Mesh>(m, "Mesh")
            .def_ro("id", &Mesh::id, "The id of the mesh")
            .def_ro("positions", &Mesh::positions, "The positions of the mesh")
            .def_ro("indices", &Mesh::indices, "The indices of the mesh")
            .def_ro("normals", &Mesh::normals, "The normals of the mesh")
            .def_ro("mesh_type", &Mesh::mesh_type, "The type of mesh", nb::enum_<MeshType>(m, "MeshType"))
            .def_ro("color", &Mesh::color, "The color of the mesh", nb::class_<Color>(m, "Color"))
            .def_ro("groups", &Mesh::group_reference, "The groups of the mesh",
                    nb::class_<GroupReference>(m, "GroupReference"));

    nb::class_<Color>(m, "Color")
            .def_rw("r", &Color::r)
            .def_rw("g", &Color::g)
            .def_rw("b", &Color::b)
            .def_rw("a", &Color::a);
}