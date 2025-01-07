// Check if the platform is Unix-based
#if defined(__unix__) || defined(__unix)

#include <cstdint>

#endif // Unix platform check

#include <filesystem>
#include "config_structs.h"
#include <chrono>
#include "CLI/CLI.hpp"
#include "cadit/occt/step_to_glb_v2.h"
#include "cadit/occt/step_to_glb_v1.h"
#include "cadit/stepcode/sc_parser.h"
#include "cadit/occt/bsplinesurf.h"



int main(int argc, char *argv[]) {
    CLI::App app{"STEP to GLB converter"};
    app.add_option("--stp", "STEP filepath")->required();
    app.add_option("--glb", "GLB filepath")->required();

    app.add_option("--lin-defl", "Linear deflection")->default_val(0.1)->check(CLI::Range(0.0, 1.0));
    app.add_option("--ang-defl", "Angular deflection")->default_val(0.5)->check(CLI::Range(0.0, 1.0));
    app.add_flag("--rel-defl", "Relative deflection");
    app.add_option("--version", "Version of the converter")->default_val(1)->check(CLI::Range(0, 3));
    app.add_option("--solid-only", "Solid only")->default_val(false);
    app.add_option("--max-geometry-num", "Maximum number of geometries to convert")->default_val(5);
    app.add_option("--filter-name", "Filter name");

    CLI11_PARSE(app, argc, argv);

    // make configuration
    GlobalConfig config{
        .stpFile = app.get_option("--stp")->results()[0],
        .glbFile = app.get_option("--glb")->results()[0],
        .version = app.get_option("--version")->as<int>(),
        .linearDeflection = app.get_option("--lin-defl")->as<double>(),
        .angularDeflection = app.get_option("--ang-defl")->as<double>(),
        .relativeDeflection = app.get_option("--rel-defl")->as<bool>(),
        .solidOnly = app.get_option("--solid-only")->as<bool>(),
        .max_geometry_num = app.get_option("--max-geometry-num")->as<int>(),
        .filter_name = app.get_option("--filter-name")->as<std::string>()
    };

    try {
        const auto start = std::chrono::high_resolution_clock::now();
        if (config.version == 0)
            make_a_bspline_surf(config);
        else if (config.version == 1)
            stp_to_glb_v1(config);
        else if (config.version == 2)
            stp_to_glb_v2(config);
        else if (config.version == 3)
            lazy_step_parser(config);

        const auto stop = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

        std::cout << "STP converted in: " << duration.count() << " microseconds" << std::endl;

    } catch (...) {
        std::cout << "Unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}
