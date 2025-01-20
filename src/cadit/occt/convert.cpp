#include "convert.h"
#include <STEPCAFControl_Reader.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <RWGltf_CafWriter.hxx>
#include <TDocStd_Document.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <filesystem>
#include <XCAFDoc_ShapeTool.hxx>
#include <Interface_Graph.hxx>
#include <Interface_Static.hxx>
#include <StepData_StepModel.hxx>
#include "custom_progress.h"
#include "step_helpers.h"
#include "step_tree.h"
#include "../../config_structs.h"

void convert_stp_to_glb(const GlobalConfig& config)
{
    // Initialize the STEPCAFControl_Reader
    STEPCAFControl_Reader reader;
    reader.SetColorMode(true);
    reader.SetNameMode(true);
    reader.SetLayerMode(true);
    reader.SetPropsMode(false);
    reader.SetGDTMode(true);
    reader.SetMatMode(true);
    reader.SetViewMode(false);

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

    // Read the STEP file
    auto start = std::chrono::high_resolution_clock::now();
    std::cout << "Reading STEP file: " << config.stpFile << std::endl;
    if (reader.ReadFile(config.stpFile.string().c_str()) != IFSelect_RetDone)
        throw std::runtime_error("Error reading STEP file");
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(stop - start).count();

    std::cout << "Read complete in " << std::fixed << std::setprecision(2) << duration << " seconds" << std::endl;

    // Transfer to a document
    std::cout << "Transferring data to document" << std::endl;
    start = std::chrono::high_resolution_clock::now();
    const Handle(TDocStd_Document) doc = new TDocStd_Document("MDTV-XCAF");

    Handle(CustomProgressIndicator) progress_indicator = new CustomProgressIndicator();

    // Create a progress range with a default name and range
    if (Message_ProgressRange progressRange = progress_indicator->Start(); !reader.Transfer(doc, progressRange))
        throw std::runtime_error("Error transferring data to document");
    progress_indicator->Reset();

    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration<double>(stop - start).count();
    std::cout << "Transfer complete in " << std::fixed << std::setprecision(2) << duration << " seconds" << std::endl;

    auto default_reader = reader.ChangeReader();
    auto model = default_reader.StepModel();

    // Build the graph of references
    Interface_Graph theGraph(model, /*keepTransient*/ Standard_False);

    // Tessellation
    const Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    TDF_LabelSequence labelSeq;
    shapeTool->GetShapes(labelSeq);

    std::vector<ProcessResult> failed_nodes;

    std::cout << "Beginning tessellation of shapes: " << labelSeq.Length() << std::endl;
    start = std::chrono::high_resolution_clock::now();
    for (Standard_Integer i = 1; i <= labelSeq.Length(); ++i)
    {
        auto label = labelSeq.Value(i);
        TopoDS_Shape shape = XCAFDoc_ShapeTool::GetShape(label);
        std::cout << "Tessellating shape " << i << " of " << labelSeq.Length() << std::endl;
        if (!perform_tessellation_with_timeout(shape, meshParams, config.tessellation_timout, progress_indicator)) {
            std::cout << "Tessellation timed out.\n";
            // get entity from shape
            auto entity = model->Entity(i);
            auto entityIndex = theGraph.Model()->Number(entity);
            auto result = ProcessResult();
            result.added_to_model = false;
            result.geometryIndex = entityIndex;
            result.skip_reason = "Tessellation timed out";
            failed_nodes.push_back(result);
        }
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration<double>(stop - start).count();
    std::cout << "Tessellation complete in " << std::fixed << std::setprecision(2) << duration << " seconds" << std::endl;

    // Write to GLB
    std:: cout << "Writing to GLB file: " << config.glbFile << std::endl;
    start = std::chrono::high_resolution_clock::now();
    RWGltf_CafWriter writer(config.glbFile.c_str(), true); // true for binary format

    // Additional file information (can be empty if not needed)
    const TColStd_IndexedDataMapOfStringString file_info;

    // Progress indicator (can be null if progress tracking is not needed)
    const Message_ProgressRange progress;

    // if output parent directory is != "" and does not exist, create it
    if (const std::filesystem::path glb_dir = config.glbFile.parent_path(); !glb_dir.empty() && !exists(glb_dir))
    {
        create_directories(glb_dir);
    }

    if (!writer.Perform(doc, file_info, progress))
    {
        throw std::runtime_error("Error writing GLB file");
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration<double>(stop - start).count();
    std::cout << "Write complete in " << std::fixed << std::setprecision(2) << duration << " seconds" << std::endl;

    // iterate over all nodes that weren't added to the model and save the list to json
    const std::filesystem::path out_json_log_file = config.glbFile.parent_path() / config.glbFile.stem().concat(
                                                    "-log.json");
    std::ofstream log_file(out_json_log_file);
    log_file << "[\n";

    for (const auto &result: failed_nodes) {
        log_file << "{\n";
        log_file << "\"geometryIndex\": " << result.geometryIndex << ",\n";
        log_file << R"("skipReason": ")" << result.skip_reason << "\"\n";
        log_file << "},\n";
    }
    log_file << "]\n";
    log_file.close();
}

