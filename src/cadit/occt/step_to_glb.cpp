#include <STEPCAFControl_Reader.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <RWGltf_CafWriter.hxx>
#include <TDocStd_Document.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <filesystem>
#include <XCAFDoc_ShapeTool.hxx>


void stp_to_glb(const std::string& stp_file,
                const std::string& glb_file,
                const double linearDeflection = 0.1,
                const double angularDeflection = 0.5,
                const bool relativeDeflection = false)
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

    // Read the STEP file
    auto start = std::chrono::high_resolution_clock::now();
    std::cout << "Reading STEP file: " << stp_file << std::endl;
    if (reader.ReadFile(stp_file.c_str()) != IFSelect_RetDone)
        throw std::runtime_error("Error reading STEP file");
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(stop - start).count();
    
    std::cout << "Read complete in " << std::fixed << std::setprecision(2) << duration << " seconds" << std::endl;

    // Transfer to a document
    std::cout << "Transferring data to document" << std::endl;
    start = std::chrono::high_resolution_clock::now();
    const Handle(TDocStd_Document) doc = new TDocStd_Document("MDTV-XCAF");

    if (!reader.Transfer(doc))
        throw std::runtime_error("Error transferring data to document");
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration<double>(stop - start).count();
    std::cout << "Transfer complete in " << std::fixed << std::setprecision(2) << duration << " seconds" << std::endl;

    // Tessellation
    const Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    TDF_LabelSequence labelSeq;
    shapeTool->GetShapes(labelSeq);

    std::cout << "Beginning tessellation of shapes: " << labelSeq.Length() << std::endl;
    start = std::chrono::high_resolution_clock::now();
    for (Standard_Integer i = 1; i <= labelSeq.Length(); ++i)
    {
        TopoDS_Shape shape = shapeTool->GetShape(labelSeq.Value(i));
        std::cout << "Tessellating shape " << i << " of " << labelSeq.Length() << std::endl;
        BRepMesh_IncrementalMesh(shape, linearDeflection, relativeDeflection, angularDeflection, false);
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration<double>(stop - start).count();
    std::cout << "Tessellation complete in " << std::fixed << std::setprecision(2) << duration << " seconds" << std::endl;

    // Write to GLB
    std:: cout << "Writing to GLB file: " << glb_file << std::endl;
    start = std::chrono::high_resolution_clock::now();
    RWGltf_CafWriter writer(glb_file.c_str(), true); // true for binary format

    // Additional file information (can be empty if not needed)
    const TColStd_IndexedDataMapOfStringString file_info;

    // Progress indicator (can be null if progress tracking is not needed)
    const Message_ProgressRange progress;

    // if output parent directory is != "" and does not exist, create it
    const std::filesystem::path glb_path(glb_file);
    if (const std::filesystem::path glb_dir = glb_path.parent_path(); !glb_dir.empty() && !exists(glb_dir))
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
}

