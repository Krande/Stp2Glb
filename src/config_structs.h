//
// Created by ofskrand on 07.01.2025.
//

#ifndef CONFIG_STRUCTS_H
#define CONFIG_STRUCTS_H


#include <filesystem>

struct GlobalConfig {
    std::filesystem::path stpFile;
    std::filesystem::path glbFile;

    // Conversion parameters
    int version;

    double linearDeflection;
    double angularDeflection;
    bool relativeDeflection;

    // Debug parameters
    bool solidOnly;
    int max_geometry_num;
    std::string filter_name;
};



#endif //CONFIG_STRUCTS_H
