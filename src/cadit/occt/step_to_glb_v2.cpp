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
#include <BRepBuilderAPI_MakeShape.hxx>
#include <BRep_Builder.hxx>
#include <TDataStd_Name.hxx>
#include <iostream>
#include "step_to_glb_v2.h"
#include <Interface_Static.hxx>

class TimingContext
{
public:
    explicit TimingContext(std::string name)
        : label(std::move(name)), start(std::chrono::high_resolution_clock::now())
    {
        std::cout << "Starting: " << label << std::endl;
    }

    ~TimingContext()
    {
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

void update_location(TopoDS_Shape& shape)
{
    TopLoc_Location loc = shape.Location();
    // Extract translation components from the transformation matrix
    gp_XYZ translation = loc.Transformation().TranslationPart();
    Standard_Real x = translation.X();
    Standard_Real y = translation.Y();
    Standard_Real z = translation.Z();

    // Output the coordinates
    std::cout << "X: " << x << ", Y: " << y << ", Z: " << z << "\n";

    if (!loc.IsIdentity())
    {
        shape.Location(loc); // Apply the transformation
    }
}

std::string get_name(const Handle(StepRepr_RepresentationItem)& repr_item)
{
    if (!repr_item.IsNull())
    {
        auto name = repr_item->Name()->ToCString();

        if (name)
        {
            return name;
        }
        return "Unnamed";
    }
    return {};
}

TopoDS_Shape make_shape(const Handle(StepShape_SolidModel)& solid_model, STEPControl_Reader& reader)
{
    if (!solid_model.IsNull())
    {
        TIME_BLOCK("Transferring solid model entity");
        // Convert the solid model into an OpenCascade shape
        if (!reader.TransferEntity(solid_model))
        {
            std::cerr << "Error transferring entity" << std::endl;
        };
        TopoDS_Shape shape = reader.Shape(reader.NbShapes());

        // Apply the location (transformation) to the shape
        update_location(shape);
        return shape;
    }
    return {};
}

TopoDS_Shape make_shape(const Handle(StepShape_Face)& face, STEPControl_Reader& reader)
{
    if (!face.IsNull())
    {
        TIME_BLOCK("Transferring face entity");
        if (!reader.TransferEntity(face))
        {
            std::cerr << "Error transferring face entity" << std::endl;
        }

        TopoDS_Shape shape = reader.Shape(reader.NbShapes());


        // Apply the location (transformation) to the shape
        update_location(shape);
        return shape;
    }
    return {};
}

bool add_shape_to_document(const TopoDS_Shape& shape, const std::string& name,
                           const Handle(XCAFDoc_ShapeTool)& shape_tool, IMeshTools_Parameters& meshParams)
{
    if (!shape.IsNull())
    {
        {
            TIME_BLOCK("Applying tessellation");
            // Perform tessellation (discretization) on the shape
            BRepMesh_IncrementalMesh mesh(shape, meshParams); // Adjust 0.1 for finer/coarser tessellation
        }

        // Add the TopoDS_Shape to the document
        {
            TIME_BLOCK("Adding shape '" + name + "' to document\n");
            TDF_Label shape_label = shape_tool->AddShape(shape);
            TDataStd_Name::Set(shape_label, name.c_str());

            if (!shape_label.IsNull())
            {
                std::cout << "Shape added to the document successfully!" << std::endl;
                return true;
            }
            else
            {
                std::cerr << "Failed to add shape to the document." << std::endl;
                return false;
            }
        }
    }
    return false;
}

bool entity_to_shape(const Handle(Standard_Transient)& entity,
                     STEPControl_Reader default_reader,
                     const Handle(XCAFDoc_ShapeTool)& shape_tool,
                     IMeshTools_Parameters& meshParams)
{
    const Handle(Standard_Type) type = entity->DynamicType();
    bool added_to_model = false;
    // Check if the entity is a solid model
    if (entity->IsKind(STANDARD_TYPE(StepShape_SolidModel)))
    {
        Handle(StepShape_SolidModel) solid_model = Handle(StepShape_SolidModel)::DownCast(entity);
        auto shape = make_shape(solid_model, default_reader);
        auto name = get_name(solid_model);
        if (add_shape_to_document(shape, name, shape_tool, meshParams))
            added_to_model = true;
    }
    // Check if the entity is a face (StepShape_AdvancedFace or StepShape_Face)
    if (entity->IsKind(STANDARD_TYPE(StepShape_AdvancedFace)) || entity->IsKind(
        STANDARD_TYPE(StepShape_Face)))
    {
        Handle(StepShape_Face) face = Handle(StepShape_Face)::DownCast(entity);
        auto shape = make_shape(face, default_reader);
        auto name = get_name(face);
        if (add_shape_to_document(shape, name, shape_tool, meshParams))
            added_to_model = true;
    }
    return added_to_model;
}

void stp_to_glb_v2(const std::string& stp_file,
                   const std::string& glb_file,
                   const double linearDeflection,
                   const double angularDeflection,
                   const bool relativeDeflection)
{
    // Initialize the STEPCAFControl_Reader
    STEPCAFControl_Reader reader;
    Interface_Static::SetIVal ("FromSTEP.FixShape.FixShellOrientationMode", 0);
    // Set reader parameters
    StepData_ConfParameters params;
    params.ReadProps = false;
    params.ReadRelationship = true;
    params.ReadLayer = true;
    params.ReadAllShapes = true;
    params.ReadName = true;
    params.ReadColor = true;
    params.ReadPrecisionMode = StepData_ConfParameters::ReadMode_Precision_User;
    params.ReadPrecisionVal = 1;
    params.ReadNonmanifold = true;

    // Mesh parameters
    IMeshTools_Parameters meshParams;
    meshParams.Angle = angularDeflection;
    meshParams.Deflection = linearDeflection;
    meshParams.Relative = relativeDeflection;
    meshParams.MinSize = 0.1;
    meshParams.AngleInterior = 0.5;
    meshParams.DeflectionInterior = 0.1;
    meshParams.CleanModel = Standard_True;
    meshParams.InParallel = Standard_True;
    meshParams.AllowQualityDecrease = Standard_True;

    {
        TIME_BLOCK("Reading STEP file");

        if (reader.ReadFile(stp_file.c_str(), params) != IFSelect_RetDone)
            throw std::runtime_error("Error reading STEP file");
    }
    Interface_Static::SetIVal ("FromSTEP.FixShape.FixShellOrientationMode", 0);
    const Handle(TDocStd_Document) doc = new TDocStd_Document("MDTV-XCAF");
    TDF_Label label = doc->Main();
    Handle(XCAFDoc_ShapeTool) shape_tool = XCAFDoc_DocumentTool::ShapeTool(label);

    auto num_roots = reader.NbRootsForTransfer();
    std::cout << "Number of roots for transfer: " << num_roots << "\n";
    auto default_reader = reader.ChangeReader();
    auto model = default_reader.StepModel();
    auto iterator = model->Entities();
    auto num_entities = iterator.NbEntities();

    std::cout << "Number of entities: " << num_entities << "\n";

    auto max_shape = 5;
    auto curr_shape = 0;

    // Use the iterator to get all shape entities
    while (iterator.More())
    {
        if (entity_to_shape(iterator.Value(), default_reader, shape_tool, meshParams))
        {
            curr_shape++;
            if (curr_shape >= max_shape)
            {
                break;
            }
        }

        iterator.Next();
    }

    // Write to GLB
    std::cout << "Writing to GLB file: " << glb_file << "\n";
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
