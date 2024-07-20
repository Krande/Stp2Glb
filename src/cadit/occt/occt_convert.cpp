//
// Created by ofskrand on 24.01.2024.
//

#include "occt_convert.h"
#include "../../binding_core.h"
#include "step_to_glb.h"

// Main function or other appropriate entry point
void step_to_glb_module(nb::module_& m)
{
    m.def("stp_to_glb", &stp_to_glb, "stp_file"_a, "glb_file"_a, "linearDeflection"_a = 0.1,
        "angularDeflection"_a = 0.5, "relativeDeflection"_a = false,
          "Convert a step file to glb file");
}
