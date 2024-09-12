//
// Created by ofskrand on 12.09.2024.
//

#ifndef STEP_TO_GLB_V1_H
#define STEP_TO_GLB_V1_H
#include <filesystem>
// Function to convert STEP file to GLB file
void stp_to_glb_v1(const std::string& stp_file, const std::string& glb_file, double linearDeflection = 0.1,
                double angularDeflection = 0.5, bool relativeDeflection = false);

#endif //STEP_TO_GLB_V1_H
