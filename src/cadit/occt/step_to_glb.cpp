#include <STEPCAFControl_Reader.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <RWGltf_CafWriter.hxx>
#include <TDocStd_Document.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <execution>
#include <filesystem>
#include <XCAFDoc_ShapeTool.hxx>
#include <utility>

class TimingContext {
public:
    explicit TimingContext(std::string  name)
        : label(std::move(name)), start(std::chrono::high_resolution_clock::now()) {
        std::cout << "Starting: " << label << std::endl;
    }

    ~TimingContext() {
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(stop - start).count();
        std::cout << label << " took " << std::fixed << std::setprecision(2)
                  << duration << " seconds." << std::endl;
    }

private:
    std::string label;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

#define TIME_BLOCK(name) TimingContext timer_##__LINE__(name)

void stp_to_glb(const std::string& stp_file,
                const std::string& glb_file,
                const double linearDeflection = 0.1,
                const double angularDeflection = 0.5,
                const bool relativeDeflection = false)
{
    // Initialize the STEPCAFControl_Reader
    STEPCAFControl_Reader reader;

    // Set reader parameters
    StepData_ConfParameters params;
    params.ReadProps = false;
    params.ReadRelationship = true;
    params.ReadLayer = true;
    params.ReadAllShapes = true;
    params.ReadName = true;
    params.ReadColor = true;
    params.ReadPrecisionVal = 0.1;

    // Read the STEP file
    {
        TIME_BLOCK("Reading STEP file");

        if (reader.ReadFile(stp_file.c_str(), params) != IFSelect_RetDone)
            throw std::runtime_error("Error reading STEP file");
    }

    auto num_roots = reader.NbRootsForTransfer();
    std::cout << "Number of roots for transfer: " << num_roots << "\n";

    // Transfer to a document
    const Handle(TDocStd_Document) doc = new TDocStd_Document("MDTV-XCAF");
    {
        TIME_BLOCK("Transferring data to document");

        if (!reader.Transfer(doc))
            throw std::runtime_error("Error transferring data to document");
    }

    // Tessellation
    const Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    TDF_LabelSequence labelSeq;
    shapeTool->GetShapes(labelSeq);

    IMeshTools_Parameters meshParams;
    meshParams.Angle = angularDeflection;
    meshParams.Deflection = linearDeflection;
    meshParams.Relative = relativeDeflection;
    meshParams.MinSize = 0.1;
    meshParams.AngleInterior = 0.5;
    meshParams.DeflectionInterior = 0.1;

    {
        TIME_BLOCK("Tessellation of shapes");
        for (Standard_Integer i = 1; i <= labelSeq.Length(); ++i)
        {
            TopoDS_Shape shape = XCAFDoc_ShapeTool::GetShape(labelSeq.Value(i));
            std::cout << "Tessellating shape " << i << " of " << labelSeq.Length() << "\n";
            auto mesh = BRepMesh_IncrementalMesh(shape, meshParams);
        }
    }

    // Write to GLB
    std:: cout << "Writing to GLB file: " << glb_file << "\n";
    {
        TIME_BLOCK("Writing to GLB file");

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
    }
}

