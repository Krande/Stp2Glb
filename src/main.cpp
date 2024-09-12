// Check if the platform is Unix-based
#if defined(__unix__) || defined(__unix)

#include <cstdint>

#endif // Unix platform check

#include <chrono>
#include "CLI/CLI.hpp"
#include "cadit/occt/step_to_glb_v2.h"
#include "cadit/occt/step_to_glb_v1.h"
#include "cadit/stepcode/sc_parser.h"


int main(int argc, char *argv[]) {
    CLI::App app{"STEP to GLB converter"};
    app.add_option("--stp", "STEP filepath")->required();
    app.add_option("--glb", "GLB filepath")->required();
    app.add_option("--lin-defl", "Linear deflection")->default_val(0.1)->check(CLI::Range(0.0, 1.0));
    app.add_option("--ang-defl", "Angular deflection")->default_val(0.5)->check(CLI::Range(0.0, 1.0));
    app.add_flag("--rel-defl", "Relative deflection");
    app.add_option("--version", "Version of the converter")->default_val(1)->check(CLI::Range(1, 3));
    CLI11_PARSE(app, argc, argv);

    const auto stp_file = app.get_option("--stp")->results()[0];
    const auto glb_file = app.get_option("--glb")->results()[0];
    const auto lin_defl = app.get_option("--lin-defl")->as<double>();
    const auto ang_defl = app.get_option("--ang-defl")->as<double>();
    const auto rel_defl = app.get_option("--rel-defl")->as<bool>();
    const auto version = app.get_option("--version")->as<int>();

    try {

        const auto start = std::chrono::high_resolution_clock::now();
        if (version == 1)
            stp_to_glb_v1(stp_file, glb_file, lin_defl, ang_defl, rel_defl);
        else if (version == 2)
            stp_to_glb_v2(stp_file, glb_file, lin_defl, ang_defl, rel_defl);
        else if (version == 3)
            lazy_step_parser(stp_file);

        const auto stop = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

        std::cout << "STP converted in: " << duration.count() << " microseconds" << std::endl;

    } catch (...) {
        std::cout << "Unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}
