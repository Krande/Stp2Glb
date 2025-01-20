#include <vector>
#include <random>
#include <gp_XYZ.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Solid.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <TDF_Label.hxx>
#include <TDataStd_Name.hxx>
#include <Quantity_Color.hxx>
#include <XCAFDoc_ColorType.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <optional>
#include "../../geom/Color.h"
#include "helpers.h"

#include <chrono>


TopoDS_Solid create_box(const std::vector<float>& box_origin, const std::vector<float>& box_dims)
{
    gp_Pnt aBoxOrigin(box_origin[0], box_origin[1], box_origin[2]);
    gp_XYZ aBoxDims(box_dims[0], box_dims[1], box_dims[2]);

    return BRepPrimAPI_MakeBox(aBoxOrigin, aBoxDims.X(), aBoxDims.Y(), aBoxDims.Z());
}

// a function that returns a random tuple std::tuple<double, double, double>
Color random_color()
{
    static std::mt19937 generator(std::random_device{}());
    static std::uniform_real_distribution<float> distribution(0.0, 1.0);
    return Color(distribution(generator), distribution(generator), distribution(generator), 1.0);
}

void set_name(const TDF_Label& label, const std::optional<std::string>& name)
{
    if (!name)
    {
        return;
    }
    TCollection_ExtendedString ext_name(name->c_str());
    TDataStd_Name::Set(label, ext_name);
}

void set_color(const TDF_Label& label, const Color& color,
               const Handle(XCAFDoc_ColorTool)& tool)
{
    float r = color.r;
    float g = color.g;
    float b = color.b;

    Quantity_Color qty_color(r, g, b, Quantity_TOC_RGB);
    tool->SetColor(label, qty_color, XCAFDoc_ColorType::XCAFDoc_ColorSurf);
}

// Utility function to split a string by a delimiter
std::vector<std::string> split(const std::string& input, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(input);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// Utility function to strip surrounding quotes from a string
std::string strip_quotes(const std::string& input) {
    if (input.size() >= 2 && input.front() == '"' && input.back() == '"') {
        return input.substr(1, input.size() - 2);
    }
    return input;
}

bool check_if_string_in_vector(const std::vector<std::string>& vec, const std::string& str)
{
    return std::any_of(vec.begin(), vec.end(), [&str](const std::string& filter_name)
    {
        return std::equal(str.begin(), str.end(),
                          filter_name.begin(), filter_name.end(),
                          [](char a, char b)
                          {
                              return std::tolower(static_cast<unsigned char>(a)) ==
                                  std::tolower(static_cast<unsigned char>(b));
                          });
    });
}

TimingContext::TimingContext(std::string name)
    : label(std::move(name)), start(std::chrono::high_resolution_clock::now())
{
    std::cout << "Starting: " << label << std::endl;
}

TimingContext::~TimingContext()
{
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(stop - start).count();
    std::cout << label << " took " << std::fixed << std::setprecision(2)
        << duration << " seconds." << std::endl;
}

#define TIME_BLOCK(name) TimingContext timer_##__LINE__(name)
