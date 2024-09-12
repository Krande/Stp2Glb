//
// Created by ofskrand on 23.01.2024.
//
#ifndef STEP_TO_GLB_H
#define STEP_TO_GLB_H


#include <string>


// Function to convert STEP file to GLB file
void stp_to_glb_v2(const std::string& stp_file, const std::string& glb_file, double linearDeflection = 0.1,
                double angularDeflection = 0.5, bool relativeDeflection = false);




#endif //STEP_TO_GLB_H
