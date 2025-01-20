// Check if the platform is Unix-based
#if defined(__unix__) || defined(__unix)

#include <cstdint>

#endif // Unix platform check

#include <filesystem>
#include "config_structs.h"
#include <chrono>
#include "CLI/CLI.hpp"
#include "cadit/occt/debug.h"
#include "cadit/occt/convert.h"
#include "cadit/occt/bsplinesurf.h"
#include "cadit/occt/helpers.h"
#include "config_utils.h"

void print_status(const GlobalConfig config) {
    std::cout << "STP2GLB Converter" << "\n";
    std::cout << "STP File: " << config.stpFile << "\n";
    std::cout << "GLB File: " << config.glbFile << "\n\n";
    std::cout << "Tessellation Parameters: " << "\n";
    std::cout << "Linear Deflection: " << config.linearDeflection << "\n";
    std::cout << "Angular Deflection: " << config.angularDeflection << "\n";
    std::cout << "Relative Deflection: " << config.relativeDeflection << "\n\n";
    std::cout << "Debug Parameters: " << "\n";
    std::cout << "Debug Mode: " << config.debug_mode << "\n";
    std::cout << "Solid Only: " << config.solidOnly << "\n";
    std::cout << "Max Geometry Num: " << config.max_geometry_num << "\n";
    std::cout << "Tessellation Timeout: " << config.tessellation_timout << "\n\n";

    // Debug output
    if (!config.filter_names_include.empty())
    {
        std::cout << "Included Filter Names:" << std::endl;
        for (const auto& name : config.filter_names_include)
        {
            std::cout << name << std::endl;
        }
    }
    if (!config.filter_names_exclude.empty())
    {
        std::cout << "Excluded Filter Names:" << std::endl;
        for (const auto& name : config.filter_names_exclude)
        {
            std::cout << name << std::endl;
        }
    }
}

int main(int argc, char* argv[])
{
    CLI::App app{"STEP to GLB converter"};
    app.add_option("--stp", "STEP filepath")->required();
    app.add_option("--glb", "GLB filepath")->required();

    app.add_option("--lin-defl", "Linear deflection")->default_val(0.1)->check(CLI::Range(0.0, 1.0));
    app.add_option("--ang-defl", "Angular deflection")->default_val(0.5)->check(CLI::Range(0.0, 1.0));
    app.add_flag("--rel-defl", "Relative deflection");

    app.add_flag("--debug", "Debug mode. More robust but slower");
    app.add_flag("--solid-only", "Solid only");
    app.add_option("--max-geometry-num", "Maximum number of geometries to convert")->default_val(0);
    app.add_option("--filter-names-include", "Include Filter name. Command separated list")->default_val("");
    app.add_option("--filter-names-file-include", "Include Filter name file")->default_val("");
    app.add_option("--filter-names-exclude", "Exclude Filter name. Command separated list")->default_val("");
    app.add_option("--filter-names-file-exclude", "Exclude Filter name file")->default_val("");
    app.add_option("--tessellation-timeout", "Tessellation timeout")->default_val(30);

    // const auto build = app.add_subcommand("build", "Build");
    // build->add_option("--b-spline-surf", "Build a B-Spline surface")->default_val(false);

    CLI11_PARSE(app, argc, argv);

    const auto config = process_parameters(app);

    print_status(config);
    std::cout << "\n";
    std::cout << "Starting conversion..." << "\n";
    try
    {
        const auto start = std::chrono::high_resolution_clock::now();
        if (config.buildConfig.build_bspline_surf)
            make_a_bspline_surf(config);

        if (config.debug_mode == 1)
            debug_stp_to_glb(config);
        else
        {
            convert_stp_to_glb(config);
        }
        const auto stop = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

        std::cout << "STP converted in: " << duration.count() << " microseconds" << "\n";
    }
    catch (...)
    {
        std::cout << "Unknown error occurred." << "\n";
        return 1;
    }

    return 0;
}
