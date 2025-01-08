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
#include <iostream>
#include "step_to_glb_v2.h"
#include "step_writer.h"
#include <Interface_Static.hxx>
#include <Interface_Graph.hxx>
#include <TDataStd_Name.hxx>

#include "gltf_writer.h"
#include "step_helpers.h"
#include "../../config_structs.h"


void stp_to_glb_v2(const GlobalConfig &config) {
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
    meshParams.AllowQualityDecrease = Standard_True; {
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
    auto iterator = model->Entities();
    auto num_entities = iterator.NbEntities();

    std::cout << "Number of entities: " << num_entities << "\n";

    // Build the graph of references
    Interface_Graph theGraph(model, /*keepTransient*/ Standard_False);
    auto curr_shape = 0;

    // Use the iterator to get all shape entities
    AdaCPPStepWriter stp_writer;
    while (iterator.More()) {
        auto const &entity = iterator.Value();
        ConvertObject cobject = entity_to_shape(entity, default_reader, shape_tool, meshParams, config.solidOnly);
        TopoDS_Shape shape = cobject.shape;

        if (!shape.IsNull()) {
            auto product_name = getStepProductName(entity, theGraph);
            auto graph_name = getStepProductNameFromGraph(entity, theGraph);
            if (product_name.empty()) {
                product_name = "NoName";
                std::cout << "Unable to find name for: " << entity->DynamicType()->Name() << "\n"; // no forced flush
            } else {
                if (cobject.name.empty() || cobject.name != product_name) {
                    std::cout << "Replaced Object name from: " << cobject.name << " to: " << product_name << "\n";
                    // no forced flush
                    cobject.name = product_name;
                }
            }

            TDataStd_Name::Set(cobject.shape_label, cobject.name.c_str());
            if (!config.filter_names.empty()) {
                auto vector_contains = check_if_string_in_vector(config.filter_names, product_name);

                if (!vector_contains) {
                    iterator.Next();
                    continue;
                }
            }

            std::cout << "Adding Shape: " << product_name << "\n"; // no forced flush
            auto color = random_color();
            stp_writer.add_shape(shape, product_name, color);
            curr_shape++;

            if (curr_shape >= config.max_geometry_num) {
                break;
            }
        }

        iterator.Next();
    }

    // Write to GLB
    std::cout << "Writing to GLB file: " << config.stpFile << "\n"; {
        TIME_BLOCK("Writing to GLB file");
        to_glb_from_doc(config.glbFile, doc);
    }
    const std::filesystem::path out_file = config.glbFile.parent_path() / config.glbFile.stem().concat("-debug.stp");
    stp_writer.export_step(out_file.string().c_str());
}
