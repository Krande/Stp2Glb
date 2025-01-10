cmake_minimum_required(VERSION 3.20)

project(stp2glb LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

OPTION(CONDA_LOCAL_DEV "Install into activate local development conda env." OFF)

if (CONDA_LOCAL_DEV)
    message(STATUS "Building for local development conda env.")
    set(EXE_BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)
endif ()

include(cmake/pre_checks.cmake)

# Create a list called ADA_CPP_SOURCES of all cpp files inside the src dir
set(ADA_CPP_SOURCES
        src/cadit/ifc/ifcop.cpp
        src/cadit/occt/helpers.cpp
        src/geom/Models.cpp
        src/geom/geometries.cpp
        src/cadit/occt/step_to_glb.cpp
        src/cadit/occt/step_helpers.cpp
        src/cadit/occt/step_writer.cpp
        src/cadit/occt/gltf_writer.cpp
        src/cadit/occt/colors.cpp
        src/cadit/tinygltf/tiny.cpp
        src/visit/ShapeTesselator.cpp
        src/visit/tess_helpers.cpp
        src/visit/manual/solids/boxes.cpp
        src/visit/TessellateFactory.cpp
)
set(ADA_CPP_HEADERS
        src/cadit/ifc/ifcop.h
        src/cadit/occt/helpers.h
        src/geom/geometries.h
        src/geom/Color.h
        src/cadit/occt/step_to_glb.h
        src/cadit/occt/step_helpers.h
        src/cadit/occt/step_writer.h
        src/cadit/occt/gltf_writer.h
        src/cadit/occt/colors.h
        src/cadit/tinygltf/tiny.h
        src/cadit/tinygltf/tinyload.h
        src/visit/ShapeTesselator.h
        src/visit/tess_helpers.h
        src/visit/TessellateFactory.h
)
# Create a empty list to hold all the linked libs
set(ADA_CPP_LINK_LIBS)

# Add dependencies
include(cmake/deps_cgal.cmake)
include(cmake/deps_occ.cmake)
include(cmake/deps_ifc.cmake)
include(cmake/deps_stepcode.cmake)

add_definitions(-DWIN32_LEAN_AND_MEAN)
add_definitions(-DNOMINMAX)

# Install the C++ executable
set(SOURCES
        src/main.cpp
        src/cadit/occt/step_tree.cpp
        src/geom/Color.cpp
        src/cadit/occt/step_to_glb_v2.cpp
        src/cadit/stepcode/sc_parser.cpp
        src/cadit/occt/gltf_writer.cpp
        src/cadit/occt/step_to_glb_v1.cpp
        src/cadit/occt/step_helpers.cpp
        src/cadit/occt/bsplinesurf.cpp
        src/cadit/occt/helpers.cpp
        src/cadit/occt/step_writer.cpp

)
set(HEADERS
        src/cadit/occt/step_tree.h
        src/config_structs.h
        src/geom/Color.h
        src/cadit/occt/step_to_glb_v1.h
        src/cadit/occt/step_to_glb_v2.h
        src/cadit/occt/step_helpers.h
        src/cadit/occt/gltf_writer.h
        src/cadit/stepcode/sc_parser.h
        src/cadit/occt/bsplinesurf.h
        src/cadit/occt/helpers.h
        src/cadit/occt/step_writer.h
)

add_executable(STP2GLB ${SOURCES} ${HEADERS})

#set(BUILD_SHARED_LIBS OFF)
#set(CMAKE_EXE_LINKER_FLAGS "/static")
#set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
#target_compile_options(STP2GLB PUBLIC /MT)
#target_link_options(STP2GLB PUBLIC /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)

set_target_properties(STP2GLB PROPERTIES
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/bin")

target_link_libraries(STP2GLB ${ADA_CPP_LINK_LIBS})

# install to executable into the bin dir
message(STATUS "Installing executable to ${EXE_BIN_DIR}")
install(TARGETS STP2GLB DESTINATION ${EXE_BIN_DIR})

# Include the tests directory
OPTION(BUILD_TESTING "Build the testing tree." ON)
if (BUILD_TESTING)
    message(STATUS "Building the testing tree.")
    enable_testing()
    #set(CMAKE_INSTALL_RPATH "${EXE_BIN_DIR}")
    #set(CMAKE_BUILD_RPATH "${EXE_BIN_DIR}")
    include(tests/tests.cmake)
endif (BUILD_TESTING)