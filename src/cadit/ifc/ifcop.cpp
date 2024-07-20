#include "ifcop.h"
#include "ifcparse/Ifc4x1.h"
#include "ifcparse/IfcFile.h"
//#include "ifcparse/IfcHierarchyHelper.h"
//#include "ifcparse/IfcBaseClass.h"
//#include "ifcgeom_schema_agnostic/Serialization.h"
#define IfcSchema Ifc4x1


using namespace std::string_literals;


int read_ifc_file(const std::string &file_name) {
    IfcParse::IfcFile file(file_name);
    if (!file.good()) {
        std::cout << "Unable to parse .ifc file" << std::endl;
        return 1;
    }

//    std::cout << "file name: " << file.header().file_name().name() << std::endl;

    IfcSchema::IfcBeam::list::ptr elements = file.instances_by_type<IfcSchema::IfcBeam>();

    std::cout << "Found " << elements->size() << " elements in " << file_name << ":" << std::endl;

    for (auto element : *elements) {

        std::cout << element->data().toString() << std::endl;
    }

    return 0;
}


void ifc_module(nb::module_ &m) {
    m.def("read_ifc_file", &read_ifc_file, "file_name"_a, "Read an ifc file");
}