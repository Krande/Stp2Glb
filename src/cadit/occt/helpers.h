#ifndef NANO_OCCT_HELPERS_H
#define NANO_OCCT_HELPERS_H

#include <vector>
#include <TopoDS_Solid.hxx>
#include <TDF_Label.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <optional>
#include "../../geom/Color.h"
#include <string>
#include <chrono>

std::string strip_quotes(const std::string& input);

std::vector<std::string> split(const std::string& input, char delimiter);

bool check_if_string_in_vector(const std::vector<std::string> &vec, const std::string &str);

TopoDS_Solid create_box(const std::vector<float> &box_origin, const std::vector<float> &box_dims);

Color random_color();

void set_name(const TDF_Label &label, const std::optional<std::string> &name);

void set_color(const TDF_Label &label, const Color &color,
               const Handle(XCAFDoc_ColorTool) &tool);



class TimingContext
{
public:
    explicit TimingContext(std::string name);
    ~TimingContext();

private:
    std::string label;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

// Macro to create a timing block
#define TIME_BLOCK(name) TimingContext timer_##__LINE__(name)

#endif // NANO_OCCT_HELPERS_H
