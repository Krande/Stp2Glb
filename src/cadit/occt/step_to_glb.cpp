#include <STEPCAFControl_Reader.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <RWGltf_CafWriter.hxx>
#include <TDocStd_Document.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <execution>
#include <filesystem>
#include <XCAFDoc_ShapeTool.hxx>
#include <utility>
#include <StepData_StepModel.hxx>
#include <Interface_EntityIterator.hxx>
#include <StepShape_SolidModel.hxx>
#include <StepShape_Face.hxx>
#include <StepShape_AdvancedFace.hxx>
#include <StepRepr_RepresentationItem.hxx>
#include <BRepBuilderAPI_MakeShape.hxx>
#include <BRep_Builder.hxx>
#include <iostream>

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
    auto default_reader = reader.ChangeReader();

    auto model = default_reader.StepModel();
    auto iterator = model->Entities();
    auto num_entities = iterator.NbEntities();
    std::cout << "Number of entities: " << num_entities << "\n";
    const Handle(TDocStd_Document) doc = new TDocStd_Document("MDTV-XCAF");
    // Get the root label of the document
    TDF_Label label = doc->Main();
    Handle(XCAFDoc_ShapeTool) shape_tool = XCAFDoc_DocumentTool::ShapeTool(label);
    // Use the iterator to get all shape entities
    while (iterator.More())
    {
        auto entity = iterator.Value();
        const Handle(Standard_Type) type = entity->DynamicType();

        // Check if the entity is a solid model
        if (entity->IsKind(STANDARD_TYPE(StepShape_SolidModel)))
        {
            // Safely cast the entity to StepGeom_SolidModel
            Handle(StepShape_SolidModel) solid_model = Handle(StepShape_SolidModel)::DownCast(entity);

            if (!solid_model.IsNull()) {
                // Convert the solid model into an OpenCascade shape
                if (!default_reader.TransferEntity(solid_model))
                {
                    std::cerr << "Error transferring entity" << std::endl;
                };
                TopoDS_Shape shape = default_reader.Shape(default_reader.NbShapes());

                // Apply the location (transformation) to the shape
                TopLoc_Location loc = shape.Location();
                if (!loc.IsIdentity())
                {
                    shape.Location(loc);  // Apply the transformation
                }
                // Perform tessellation (discretization) on the shape
                BRepMesh_IncrementalMesh mesh(shape, 0.1);  // Adjust 0.1 for finer/coarser tessellation
                // Add the TopoDS_Shape to the document
                TDF_Label shape_label = shape_tool->AddShape(shape);
                if (!shape_label.IsNull())
                {
                    std::cout << "Shape added to the document successfully!" << std::endl;
                }
                else
                {
                    std::cerr << "Failed to add shape to the document." << std::endl;
                }
            }
        }
        // Check if the entity is a face (StepShape_AdvancedFace or StepShape_Face)
        if (entity->IsKind(STANDARD_TYPE(StepShape_AdvancedFace)) || entity->IsKind(STANDARD_TYPE(StepShape_Face)))
        {
            Handle(StepShape_Face) face = Handle(StepShape_Face)::DownCast(entity);

            if (!face.IsNull())
            {
                if (!default_reader.TransferEntity(face))
                {
                    std::cerr << "Error transferring face entity" << std::endl;
                }
                TopoDS_Shape face_shape = default_reader.Shape(default_reader.NbShapes());

                // Apply the location (transformation) to the shape
                TopLoc_Location loc = face_shape.Location();
                if (!loc.IsIdentity())
                {
                    face_shape.Location(loc);  // Apply the transformation
                }

                BRepMesh_IncrementalMesh mesh(face_shape, 0.1);
                TDF_Label face_label = shape_tool->AddShape(face_shape);
                if (!face_label.IsNull())
                {
                    std::cout << "Face added to the document successfully!" << std::endl;
                }
                else
                {
                    std::cerr << "Failed to add face to the document." << std::endl;
                }
            }
        }
        iterator.Next();

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

