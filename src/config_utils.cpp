//
// Created by ofskrand on 13.01.2025.
//

#include "config_utils.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "config_structs.h"
#include "cadit/occt/helpers.h"
#include "CLI/App.hpp"

// Helper function to process filter names from input or file
std::vector<std::string> process_filter_names(const std::string& input, const std::string& file_name)
{
    std::vector<std::string> filter_names;

    // Process input string
    if (!input.empty())
    {
        std::string stripped_input = strip_quotes(input);
        auto names = split(stripped_input, ',');
        filter_names.insert(filter_names.end(), names.begin(), names.end());
    }

    // Process file
    if (!file_name.empty())
    {
        if (std::ifstream file(file_name); file.is_open())
        {
            std::string line;
            while (std::getline(file, line))
            {
                if (!line.empty())
                {
                    filter_names.push_back(line);
                }
            }
            file.close();
        }
        else
        {
            throw std::runtime_error("Error: Could not open file: " + file_name);
        }
    }


    return filter_names;
}

// Helper function to check if a string ends with a specific suffix (case insensitive)
bool endsWithCaseInsensitive(const std::string& str, const std::string& suffix) {
    if (str.size() < suffix.size()) {
        return false;
    }
    auto strIt = str.end() - suffix.size();
    auto suffixIt = suffix.begin();
    while (suffixIt != suffix.end()) {
        if (tolower(*strIt++) != tolower(*suffixIt++)) {
            return false;
        }
    }
    return true;
}

// Main processing function
GlobalConfig process_parameters(CLI::App& app)
{
    const auto filter_names_include_input = app.get_option("--filter-names-include")->as<std::string>();
    const auto filter_names_file_include = app.get_option("--filter-names-file-include")->as<std::string>();

    const auto filter_names_exclude_input = app.get_option("--filter-names-exclude")->as<std::string>();
    const auto filter_names_file_exclude = app.get_option("--filter-names-file-exclude")->as<std::string>();

    // Process include and exclude filter names
    const auto filter_names_include = process_filter_names(filter_names_include_input, filter_names_file_include);
    const auto filter_names_exclude = process_filter_names(filter_names_exclude_input, filter_names_file_exclude);

    const std::string stpFilename = app.get_option("--stp")->results()[0];
    const std::string glbFilename = app.get_option("--glb")->results()[0];

    // Validate extensions
    const bool isStpValid = endsWithCaseInsensitive(stpFilename, ".stp") || endsWithCaseInsensitive(stpFilename, ".step");
    const bool isGlbValid = endsWithCaseInsensitive(glbFilename, ".glb");

    if (!isStpValid) {
        throw std::invalid_argument("Invalid --stp filename. It must end with .stp or .step.");
    }

    if (!isGlbValid) {
        throw std::invalid_argument("Invalid --glb filename. It must end with .glb.");
    }

    // Create configuration
    return {
        .stpFile = stpFilename,
        .glbFile = glbFilename,
        .debug_mode = app.get_option("--debug")->as<bool>(),
        .linearDeflection = app.get_option("--lin-defl")->as<double>(),
        .angularDeflection = app.get_option("--ang-defl")->as<double>(),
        .relativeDeflection = app.get_option("--rel-defl")->as<bool>(),
        .solidOnly = app.get_option("--solid-only")->as<bool>(),
        .max_geometry_num = app.get_option("--max-geometry-num")->as<int>(),
        .tessellation_timout = app.get_option("--tessellation-timeout")->as<int>(),
        .filter_names_include = filter_names_include,
        .filter_names_exclude = filter_names_exclude,
        .buildConfig = {
            .build_bspline_surf = false
        }
    };
}
