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
#include <TDataStd_Name.hxx>

#include "gltf_writer.h"
#include "step_helpers.h"
#include "step_tree.h"
#include "../../config_structs.h"

class CustomProgressIndicator : public Message_ProgressIndicator {
public:
    CustomProgressIndicator() : shouldCancel(false) {}

    //! Called by OpenCASCADE to check if the operation should stop
    Standard_Boolean UserBreak() override {
        return shouldCancel; // Return true to stop the operation
    }

    //! Updates progress
    void Show(const Message_ProgressScope& theScope, const Standard_Boolean isForce) override {
        // Optional: Print progress or log it
        // std::cout << "Progress: " << GetPosition() * 100.0 << "%\n";

        // Clear the previous output line and overwrite it
        //std::cout << "\rProgress: " << static_cast<int>(GetPosition() * 100.0) << "%   " << std::flush;

        const int barWidth = 50; // Width of the progress bar
        double progress = GetPosition(); // Current progress [0.0, 1.0]

        // Build the progress bar
        std::cout << "\r[";
        int pos = static_cast<int>(progress * barWidth);
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";  // Completed part
            else if (i == pos) std::cout << ">";  // Current progress
            else std::cout << " ";  // Remaining part
        }
        std::cout << "] " << static_cast<int>(progress * 100.0) << "%   " << std::flush;
    }

    //! Cancels the progress
    void Cancel() {
        shouldCancel = true;
    }

    //! Resets the progress indicator (optional implementation)
    void Reset() override {
        shouldCancel = false;
    }

private:
    std::atomic<bool> shouldCancel; // Thread-safe cancellation flag
};

// Function to perform tessellation with a timeout
bool perform_tessellation_with_timeout(const TopoDS_Shape& shape, IMeshTools_Parameters& meshParams, int timeoutSeconds) {
    // Create a custom progress indicator
    Handle(CustomProgressIndicator) progress = new CustomProgressIndicator();

    // Create a progress range with a default name and range
    Message_ProgressRange progressRange = progress->Start();

    // Flag to track if the operation timed out
    std::atomic<bool> tessellationComplete = false;

    // Launch tessellation in a separate thread
    std::thread tessellationThread([&]() {
        try {
            // Run tessellation with progress monitoring
            BRepMesh_IncrementalMesh mesh(shape, meshParams, progressRange);

            // Mark as complete
            tessellationComplete = true;
        } catch (const Standard_Failure& e) {
            std::cerr << "Tessellation failed: " << e.GetMessageString() << "\n";
        }
    });

    // Monitor the tessellation thread for timeout
    const auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(timeoutSeconds)) {
        if (tessellationComplete) {
            // Tessellation finished successfully
            tessellationThread.join();
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Poll every 50 ms
    }

    // Timeout occurred, cancel tessellation
    progress->Cancel();
    if (tessellationThread.joinable()) {
        tessellationThread.join();
    }

    return false; // Tessellation timed out
}

void debug_stp_to_glb(const GlobalConfig& config)
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

    // Extract hierarchy
    std::vector<std::unique_ptr<ProductNode>> roots = ExtractProductHierarchy(model, theGraph);
    add_geometries_to_nodes(roots, theGraph);

    AdaCPPStepWriter stp_writer = AdaCPPStepWriter("Assembly", roots);

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
    for (const auto& node : GeometryRange(roots))
    {
        num_geometry += node.geometryIndices.size();
        num_products++;
    }
    auto curr_shape = 0;
    auto curr_product = 0;
    // Iterate over all nodes with geometry indices
    for (const auto& node : GeometryRange(roots))
    {
        Handle(StepBasic_Product) product = Handle(StepBasic_Product)::DownCast(model->Entity(node.entityIndex));

        std::cout << "Node: " << node.name << " (" << curr_product << "/" << num_products << ")"
            << ", EntityIndex: " << node.entityIndex
            << ", Geometry count: " << node.geometryIndices.size() << '\n';

        for (int geometryIndex : node.geometryIndices)
        {
            std::cout << "Geometry: " << geometryIndex << " (" << curr_shape << "/" << num_geometry << ")\n";
            auto brep = model->Entity(geometryIndex);
            ConvertObject cobject = entity_to_shape(brep, default_reader, shape_tool, meshParams, config.solidOnly);
            TopoDS_Shape shape = cobject.shape;

            TDF_Label shape_label;

            cobject.name = node.name;

            if (!config.filter_names_include.empty()) {
                auto vector_contains = check_if_string_in_vector(config.filter_names_include, cobject.name);
                if (!vector_contains)
                {
                    std::cout << "Skipping shape: " << cobject.name << " (Entity: " << node.entityIndex << ")\n";
                    iterator.Next();
                    continue;
                }
            }

            if (!config.filter_names_exclude.empty())
            {
                auto vector_contains = check_if_string_in_vector(config.filter_names_exclude, cobject.name);

                if (vector_contains)
                {
                    std::cout << "Skipping shape: " << cobject.name << " (Entity: " << node.entityIndex << ")\n";
                    iterator.Next();
                    continue;
                }
            }

            // Updated code block
            {
                TIME_BLOCK("Applying tessellation");
                if (perform_tessellation_with_timeout(shape, meshParams, 30)) {
                    std::cout << "Tessellation completed successfully.\n";
                } else {
                    std::cout << "Tessellation timed out.\n";
                }
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


            auto color = random_color();

            std::cout << "Adding Shape: " << cobject.name << " (Entity: " << node.entityIndex << ") to STEP Writer\n";
            stp_writer.add_shape(shape, cobject.name, color, node);

            curr_shape++;
        }

        if (config.max_geometry_num != 0 && curr_shape >= config.max_geometry_num)
        {
            break;
        }
        curr_product++;
    }

    const std::filesystem::path out_file = config.glbFile.parent_path() / config.glbFile.stem().concat("-debug.stp");

    stp_writer.export_step(out_file.string().c_str());

    // Write to GLB
    std::cout << "Writing to GLB file: " << config.stpFile << "\n";
    {
        TIME_BLOCK("Writing to GLB file");

        to_glb_from_doc(config.glbFile, doc);
    }
}
