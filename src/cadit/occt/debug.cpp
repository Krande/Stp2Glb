#include <STEPCAFControl_Reader.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <RWGltf_CafWriter.hxx>
#include <TDocStd_Document.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <execution>
#include <filesystem>
#include <XCAFDoc_ShapeTool.hxx>
#include <StepData_StepModel.hxx>
#include <Interface_EntityIterator.hxx>
#include <BRepBuilderAPI_MakeShape.hxx>
#include "geometry_iterator.h"
#include <iostream>
#include "debug.h"

#include <future>

#include "step_writer.h"
#include <Interface_Static.hxx>
#include <Interface_Graph.hxx>
#include <StepBasic_Product.hxx>

#include "custom_progress.h"
#include "helpers.h"
#include "step_helpers.h"
#include "step_tree.h"
#include "../../config_structs.h"


bool should_process_geometry(const Handle(Standard_Transient) &brep, const ProductNode &node,
                             const GlobalConfig &config) {
    if (!brep->IsKind(STANDARD_TYPE(StepShape_SolidModel))) {
        if (config.solidOnly) {
            return false;
        }
    }
    if (!config.filter_names_include.empty()) {
        if (!check_if_string_in_vector(config.filter_names_include, node.name)) {
            return false;
        }
    }

    if (!config.filter_names_exclude.empty()) {
        if (check_if_string_in_vector(config.filter_names_exclude, node.name)) {
            return false;
        }
    }
    return true;
}


void debug_stp_to_glb(const GlobalConfig &config) {
    // Initialize the STEPCAFControl_Reader
    STEPCAFControl_Reader reader;
    Interface_Static::SetIVal("FromSTEP.FixShape.FixShellOrientationMode", 0);
    Interface_Static::SetIVal("read.step.shape.repair.mode", 0);
    Interface_Static::SetIVal("read.precision.mode", 0);

    // Set reader parameters
    StepData_ConfParameters params;
    params.ReadProps = false;
    params.ReadRelationship = true;
    params.ReadLayer = true;
    params.ReadAllShapes = true;
    params.ReadName = true;
    params.ReadSubshapeNames = true;
    params.ReadResourceName = true;
    params.ReadColor = true;
    params.ReadPrecisionMode = StepData_ConfParameters::ReadMode_Precision_User;
    params.ReadPrecisionVal = 1;
    params.ReadNonmanifold = true;

    // Mesh parameters
    IMeshTools_Parameters meshParams;
    meshParams.Angle = config.angularDeflection;
    meshParams.Deflection = config.linearDeflection;
    meshParams.Relative = config.relativeDeflection;
    meshParams.MinSize = 0.1;
    meshParams.AngleInterior = 0.5;
    meshParams.DeflectionInterior = 0.1;
    meshParams.CleanModel = Standard_True;
    meshParams.InParallel = Standard_True;
    meshParams.AllowQualityDecrease = Standard_True;

    {
        TIME_BLOCK("Reading STEP file");

        if (reader.ReadFile(config.stpFile.string().c_str(), params) != IFSelect_RetDone)
            throw std::runtime_error("Error reading STEP file");
    }

    Interface_Static::SetIVal("FromSTEP.FixShape.FixShellOrientationMode", 0);

    auto num_roots = reader.NbRootsForTransfer();
    std::cout << "Number of roots for transfer: " << num_roots << "\n";

    auto default_reader = reader.ChangeReader();
    auto model = default_reader.StepModel();

    // Build the graph of references
    Interface_Graph theGraph(model, /*keepTransient*/ Standard_False);

    auto iterator = model->Entities();
    auto num_entities = iterator.NbEntities();

    std::cout << "Number of entities: " << num_entities << "\n";

    // Extract hierarchy
    auto roots = ExtractProductHierarchy(model, theGraph);
    add_geometries_to_nodes(roots, theGraph);

    auto step_store = StepStore(roots);

    // Convert Hierarchy to JSON
    std::string jsonOutput = ExportHierarchyToJson(roots);

    // Then write to file or print to console:
    const std::filesystem::path out_json_file = config.glbFile.parent_path() / config.glbFile.stem().concat(
                                                    "-hierarchy.json");
    std::ofstream file(out_json_file);
    file << jsonOutput;
    file.close();

    std::cout << "Hierarchy exported to assembly_hierarchy.json\n";

    int num_geometry = 0;
    int num_products = 0;

    // first find the number of geometries
    for (const auto &node: GeometryRange(roots)) {
        num_geometry += node.geometryIndices.size();
        num_products++;
    }
    auto curr_shape = 0;
    auto curr_product = 0;

    // Iterate over all nodes with geometry indices
    for (const auto &node: GeometryRange(roots)) {
        Handle(StepBasic_Product) product = Handle(StepBasic_Product)::DownCast(model->Entity(node.entityIndex));

        std::cout << "Node: " << node.name << " (" << curr_product << "/" << num_products << ")"
                << ", EntityIndex: " << node.entityIndex
                << ", Geometry count: " << node.geometryIndices.size() << '\n';

        for (int geometryIndex: node.geometryIndices) {
            std::cout << "Geometry: " << geometryIndex << " (" << curr_shape << "/" << num_geometry << ")\n";

            auto geometry = model->Entity(geometryIndex);

            if (!should_process_geometry(geometry, node, config)) {
                std::cout << "Skipping shape: " << node.name << " (Entity: " << node.entityIndex << ")\n";
                node.processResult.added_to_model = false;
                node.processResult.geometryIndex = geometryIndex;
                node.processResult.skip_reason = "Skipped by filter";
                curr_shape++;
                continue;
            }

            if (!default_reader.TransferEntity(geometry)) {
                std::cerr << "Error transferring entity" << "\n";
                node.processResult.added_to_model = false;
                node.processResult.geometryIndex = geometryIndex;
                node.processResult.skip_reason = "Error transferring entity";
                curr_shape++;
                continue;
            };
            TopoDS_Shape shape = default_reader.Shape(default_reader.NbShapes());
            if (shape.IsNull()) {
                std::cerr << "Error converting entity to shape" << "\n";
                node.processResult.added_to_model = false;
                node.processResult.geometryIndex = geometryIndex;
                node.processResult.skip_reason = "Unable to convert entity to shape";
                curr_shape++;
                continue;
            }

            auto color = random_color();

            std::cout << "Adding Shape: " << node.name << " (Entity: " << node.entityIndex << ") to STEP Writer\n";
            step_store.add_shape(shape, node.name, color, node);

            // Updated code block
            {
                TIME_BLOCK("Applying tessellation");
                if (!perform_tessellation_with_timeout(shape, meshParams, config.tessellation_timout)) {
                    std::cout << "Tessellation timed out.\n";
                    node.processResult.added_to_model = false;
                    node.processResult.geometryIndex = geometryIndex;
                    node.processResult.skip_reason = "Tessellation timed out";
                    curr_shape++;
                    continue;
                }
            }
            curr_shape++;
        }

        if (config.max_geometry_num != 0 && curr_shape >= config.max_geometry_num) {
            break;
        }
        curr_product++;
    }

    step_store.to_glb(config.glbFile);

    const std::filesystem::path out_file = config.glbFile.parent_path() / config.glbFile.stem().concat("-debug.stp");
    step_store.to_step(out_file.string().c_str());

    // iterate over all nodes that werent added to the model and save the list to json
    const std::filesystem::path out_json_log_file = config.glbFile.parent_path() / config.glbFile.stem().concat(
                                                    "-log.json");
    std::ofstream log_file(out_json_log_file);
    log_file << "[\n";

    for (const auto &node: GeometryRange(roots)) {
        if (!node.processResult.added_to_model && node.processResult.geometryIndex != 0) {
            log_file << "{\n";
            log_file << R"("name": ")" << node.name << "\",\n";
            log_file << "\"entityIndex\": " << node.entityIndex << ",\n";
            log_file << "\"geometryIndex\": " << node.processResult.geometryIndex << ",\n";
            log_file << R"("skipReason": ")" << node.processResult.skip_reason << "\"\n";
            log_file << "},\n";
        }
    }
    log_file << "]\n";
    log_file.close();


}
