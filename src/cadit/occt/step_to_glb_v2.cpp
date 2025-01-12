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
#include "step_to_glb_v2.h"

#include <BRepBuilderAPI_Transform.hxx>

#include "step_writer.h"
#include <Interface_Static.hxx>
#include <Interface_Graph.hxx>
#include <StepBasic_Product.hxx>
#include <TDataStd_Name.hxx>

#include "gltf_writer.h"
#include "step_helpers.h"
#include "step_tree.h"
#include "../../config_structs.h"


void stp_to_glb_v2(const GlobalConfig& config)
{
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
    const Handle(TDocStd_Document) doc = new TDocStd_Document("MDTV-XCAF");
    TDF_Label label = doc->Main();
    Handle(XCAFDoc_ShapeTool) shape_tool = XCAFDoc_DocumentTool::ShapeTool(label);

    auto num_roots = reader.NbRootsForTransfer();
    std::cout << "Number of roots for transfer: " << num_roots << "\n";

    auto default_reader = reader.ChangeReader();
    auto model = default_reader.StepModel();

    // Build the graph of references
    Interface_Graph theGraph(model, /*keepTransient*/ Standard_False);

    auto iterator = model->Entities();
    auto num_entities = iterator.NbEntities();

    std::cout << "Number of entities: " << num_entities << "\n";

    auto curr_shape = 0;

    // Extract hierarchy
    std::vector<ProductNode> roots = ExtractProductHierarchy(model, theGraph);
    add_geometries_to_nodes(roots, theGraph);

    // Use the iterator to get all shape entities
    AdaCPPStepWriter stp_writer = AdaCPPStepWriter("Assembly", roots);

    // Convert to JSON
    std::string jsonOutput = ExportHierarchyToJson(roots);

    // Then write to file or print to console:
    const std::filesystem::path out_json_file = config.glbFile.parent_path() / config.glbFile.stem().concat(
        "-hierarchy.json");
    std::ofstream file(out_json_file);
    file << jsonOutput;
    file.close();

    std::cout << "Hierarchy exported to assembly_hierarchy.json\n";

    // Iterate over all nodes with geometry indices
    for (const auto& node : GeometryRange(roots))
    {
        Handle(StepBasic_Product) product = Handle(StepBasic_Product)::DownCast(model->Entity(node.entityIndex));

        std::cout << "Node: " << node.name
            << ", EntityIndex: " << node.entityIndex
            << ", Geometry count: " << node.geometryIndices.size() << '\n';

        for (int geometryIndex : node.geometryIndices)
        {
            auto brep = model->Entity(geometryIndex);
            ConvertObject cobject = entity_to_shape(brep, default_reader, shape_tool, meshParams, config.solidOnly);
            TopoDS_Shape shape = cobject.shape;

            TDF_Label shape_label;

            cobject.name = node.name;

            if (!config.filter_names.empty())
            {
                auto vector_contains = check_if_string_in_vector(config.filter_names, cobject.name);

                if (!vector_contains)
                {
                    iterator.Next();
                    continue;
                }
            }

            {
                TIME_BLOCK("Applying tessellation");
                // Perform tessellation (discretization) on the shape
                BRepMesh_IncrementalMesh mesh(shape, meshParams);
                // Adjust 0.1 for finer/coarser tessellation
            }

            // Add the TopoDS_Shape to the document
            {
                TIME_BLOCK("Adding shape '" + cobject.name + "' to document\n");
                shape_label = shape_tool->AddShape(shape);
            }

            if (!shape_label.IsNull())
            {
                std::cout << "Shape added to the document successfully!" << "\n";
                cobject.shape_label = shape_label;
            }
            else
            {
                std::cerr << "Failed to add shape to the document." << "\n";
            }

            TDataStd_Name::Set(cobject.shape_label, cobject.name.c_str());

            std::cout << "Adding Shape: " << cobject.name << " (Entity: " << node.entityIndex << ")\n";
            // no forced flush
            auto color = random_color();

            // Add any additional location transformations from product to shape
            BRepBuilderAPI_Transform shapeTransform(node.transformation);
            shapeTransform.Perform(shape, Standard_False);

            stp_writer.add_shape(shape, cobject.name, color, node.name);

            curr_shape++;
            if (config.max_geometry_num != 0 && curr_shape >= config.max_geometry_num)
            {

                break;
            }
        }
    }

    // Write to GLB
    std::cout << "Writing to GLB file: " << config.stpFile << "\n";
    {
        TIME_BLOCK("Writing to GLB file");
        to_glb_from_doc(config.glbFile, doc);
    }

    const std::filesystem::path out_file = config.glbFile.parent_path() / config.glbFile.stem().concat("-debug.stp");

    stp_writer.export_step(out_file.string().c_str());
}
