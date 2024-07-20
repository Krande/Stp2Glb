#ifndef NANO_OCCT_MESH_H
#define NANO_OCCT_MESH_H

#include <vector>
#include <cstdint>
#include <optional>
#include "MeshType.h"
#include "GroupReference.h"
#include "Color.h"

class Mesh {
public:
    Mesh(int id,
         std::vector<float> positions,
         std::vector<uint32_t> faces,
         std::vector<uint32_t> edges = {},
         std::vector<float> normals = {},
         MeshType mesh_type = MeshType::TRIANGLES,
         Color color = Color(),
         std::vector<GroupReference> group_reference = {});

    int id;
    std::vector<float> positions;
    std::vector<uint32_t> indices;
    std::vector<uint32_t> edges;
    std::vector<float> normals;
    MeshType mesh_type;
    Color color;
    std::vector<GroupReference> group_reference;
};

#endif //NANO_OCCT_MESH_H
