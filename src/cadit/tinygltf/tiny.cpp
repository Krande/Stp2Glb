//
// Created by Kristoffer on 26/07/2023.
//
#include <iostream>
#include "tinyload.h"
#include "../../geom/Mesh.h"
#include "../../binding_core.h"
#include "../../visit/tess_helpers.h"
#include "../../helpers/helpers.h"

std::pair<std::vector<double>, std::vector<double>> calculateBounds(const std::vector<float>& positions) {
    assert(positions.size() % 3 == 0); // Ensure there is 3D data

    std::vector<double> minValues = { std::numeric_limits<double>::max(),
                                      std::numeric_limits<double>::max(),
                                      std::numeric_limits<double>::max() };

    std::vector<double> maxValues = { std::numeric_limits<double>::lowest(),
                                      std::numeric_limits<double>::lowest(),
                                      std::numeric_limits<double>::lowest() };

    for (size_t i = 0; i < positions.size(); i += 3) {
        minValues[0] = std::min(static_cast<double>(minValues[0]), static_cast<double>(positions[i]));
        minValues[1] = std::min(static_cast<double>(minValues[1]), static_cast<double>(positions[i + 1]));
        minValues[2] = std::min(static_cast<double>(minValues[2]), static_cast<double>(positions[i + 2]));

        maxValues[0] = std::max(static_cast<double>(maxValues[0]), static_cast<double>(positions[i]));
        maxValues[1] = std::max(static_cast<double>(maxValues[1]), static_cast<double>(positions[i + 1]));
        maxValues[2] = std::max(static_cast<double>(maxValues[2]), static_cast<double>(positions[i + 2]));
    }

    return {minValues, maxValues};
}


void AddMesh(tinygltf::Model &model, const std::string &name, Mesh my_mesh) {
    std::vector<float> positions = my_mesh.positions;
    std::vector<uint32_t> indices = my_mesh.indices;

    // Append buffer for position data
    tinygltf::Buffer posBuffer;
    posBuffer.data = std::vector<unsigned char>(reinterpret_cast<const unsigned char*>(positions.data()),
                                                reinterpret_cast<const unsigned char*>(positions.data() + positions.size()));
    model.buffers.push_back(posBuffer);

    // Buffer for indices
    tinygltf::Buffer indexBuffer;
    indexBuffer.data = std::vector<unsigned char>(reinterpret_cast<const unsigned char*>(indices.data()),
                                                  reinterpret_cast<const unsigned char*>(indices.data() + indices.size()));
    model.buffers.push_back(indexBuffer);

    // BufferView for positions
    tinygltf::BufferView posBufferView;
    posBufferView.buffer = model.buffers.size() - 2;
    posBufferView.byteLength = posBuffer.data.size();
    posBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER; // Add this line
    model.bufferViews.push_back(posBufferView);

    // BufferView for indices
    tinygltf::BufferView indexBufferView;
    indexBufferView.buffer = model.buffers.size() - 1;
    indexBufferView.byteLength = indexBuffer.data.size();
    indexBufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER; // Add this line
    model.bufferViews.push_back(indexBufferView);

    // Accessor for positions
    tinygltf::Accessor posAccessor;
    posAccessor.bufferView = model.bufferViews.size() - 2;
    posAccessor.byteOffset = 0;
    posAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    posAccessor.count = positions.size() / 3;
    posAccessor.type = TINYGLTF_TYPE_VEC3;

    // Calculate and add min and max here
    auto [minValues, maxValues] = calculateBounds(positions);
    posAccessor.minValues = minValues;
    posAccessor.maxValues = maxValues;

    model.accessors.push_back(posAccessor);

    // Accessor for indices
    tinygltf::Accessor indexAccessor;
    indexAccessor.bufferView = model.bufferViews.size() - 1;
    indexAccessor.byteOffset = 0;
    indexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT; // uint32_t indices
    indexAccessor.count = indices.size();
    indexAccessor.type = TINYGLTF_TYPE_SCALAR;
    model.accessors.push_back(indexAccessor);

    // Create a mesh that references the accessors
    tinygltf::Mesh mesh;
    tinygltf::Primitive primitive;
    primitive.attributes["POSITION"] = model.accessors.size() - 2;
    primitive.indices = model.accessors.size() - 1;
    primitive.mode = TINYGLTF_MODE_TRIANGLES;
    mesh.primitives.push_back(primitive);

    // Create a material with PBR properties
    tinygltf::Material material;
    material.pbrMetallicRoughness.baseColorFactor = {my_mesh.color.r, my_mesh.color.g, my_mesh.color.b, my_mesh.color.a};
    material.pbrMetallicRoughness.metallicFactor = 1.0;
    material.pbrMetallicRoughness.roughnessFactor = 0.5;
    material.alphaMode = "OPAQUE";

    // Add the material to the model
    int materialIndex = model.materials.size();
    model.materials.push_back(material);

    // Assign the material to the primitive
    mesh.primitives[0].material = materialIndex; // Modify this line

    model.meshes.push_back(mesh);

    // Create a node that references the mesh
    tinygltf::Node node;
    node.mesh = model.meshes.size() - 1;
    model.nodes.push_back(node);

    // Add the node to the scene
    model.scenes[0].nodes.push_back(model.nodes.size() - 1);
}

int write_to_gltf(const std::string& filename, Mesh mesh) {
    tinygltf::Model model;

    // Create a scene
    tinygltf::Scene scene;
    model.scenes.push_back(scene);
    model.defaultScene = 0;
    AddMesh(model, "mesh", mesh);

    // Save to file
    tinygltf::TinyGLTF gltf;
    if (!gltf.WriteGltfSceneToFile(&model, filename, true, true, true, false)) {
        std::cerr << "Failed to write glTF file" << std::endl;
        return -1;
    }

    return 0;
}

// take in a list of box dimensions and origins and write to step file using the AdaCPPStepWriter class
int write_boxes_to_gltf(const std::string &filename, const std::vector<std::vector<float>> &box_origins,
                         const std::vector<std::vector<float>> &box_dims) {
    tinygltf::Model model;

    // Create a scene
    tinygltf::Scene scene;
    model.scenes.push_back(scene);
    model.defaultScene = 0;

    for (int i = 0; i < box_origins.size(); i++) {
        TopoDS_Solid box = create_box(box_origins[i], box_dims[i]);
        Mesh mesh = tessellate_shape(0, box, true, 1.0, false);
        mesh.color = random_color();
        AddMesh(model, "mesh", mesh);
    }
    // If filename contains .glb set variable "glb" to true
    bool glb = filename.find(".glb") != std::string::npos;

    // Save to file
    tinygltf::TinyGLTF gltf;
    if (!gltf.WriteGltfSceneToFile(&model, filename, true, true, true, glb)) {
        std::cerr << "Failed to write glTF file" << std::endl;
        return -1;
    }

    return 0;
}


void tiny_gltf_module(nb::module_ &m) {
    m.def("write_mesh_to_gltf", &write_to_gltf, "filename"_a, "mesh"_a, "Write a Mesh to GLTF");
    m.def("write_boxes_to_gltf", &write_boxes_to_gltf, "filename"_a, "box_origins"_a, "box_dims"_a, "Write a list of boxes to GLTF");
}