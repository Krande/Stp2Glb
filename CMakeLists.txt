cmake_minimum_required(VERSION 3.20)

project(stp2glb LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

OPTION(CONDA_LOCAL_DEV "Install into activate local development conda env." OFF)

if (CONDA_LOCAL_DEV)
    message(STATUS "Building for local development conda env.")
    set(EXE_BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)
endif ()

include(cmake/pre_checks.cmake)

# Create a empty list to hold all the linked libs
set(ADA_CPP_LINK_LIBS)
# Add dependencies

include(cmake/deps_occ.cmake)

# Add an option to enable static builds
OPTION(BUILD_STATIC "Build STP2GLB as a static executable." OFF)

if (BUILD_STATIC)
    message(STATUS "Building STP2GLB as a static executable.")

    # Force static linking
    set(BUILD_SHARED_LIBS OFF) # Disable shared libraries
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib" ".a") # Prefer static libraries
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

    # For MSVC, ensure the static runtime is used
    if (MSVC)
        foreach (flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            set(${flag_var} "${${flag_var}} /MT")
        endforeach ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MT")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")

        # Force dependencies to link statically
        set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE) # Prefer CMake config files
        foreach (lib ${ADA_CPP_LINK_LIBS})
            if (TARGET ${lib})
                set_target_properties(${lib} PROPERTIES
                        BUILD_WITH_INSTALL_RPATH TRUE
                        LINK_SEARCH_START_STATIC TRUE
                        LINK_SEARCH_END_STATIC TRUE
                )
            endif ()
        endforeach ()
    endif ()

else ()
    include(cmake/deps_ifc.cmake)
    include(cmake/deps_stepcode.cmake)
    include(cmake/deps_cgal.cmake)
endif ()

if (MSVC)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
endif ()


# Install the C++ executable
set(SOURCES
        src/main.cpp
        src/config_utils.cpp
        src/geom/Color.cpp
        src/cadit/occt/step_tree.cpp
        src/cadit/occt/debug.cpp
        src/cadit/occt/gltf_writer.cpp
        src/cadit/occt/convert.cpp
        src/cadit/occt/step_helpers.cpp
        src/cadit/occt/bsplinesurf.cpp
        src/cadit/occt/helpers.cpp
        src/cadit/occt/step_writer.cpp
        src/cadit/occt/custom_progress.cpp
)
set(HEADERS
        src/config_utils.h
        src/config_structs.h
        src/geom/Color.h
        src/cadit/occt/step_tree.h
        src/cadit/occt/convert.h
        src/cadit/occt/debug.h
        src/cadit/occt/step_helpers.h
        src/cadit/occt/gltf_writer.h
        src/cadit/occt/bsplinesurf.h
        src/cadit/occt/helpers.h
        src/cadit/occt/step_writer.h
        src/cadit/occt/geometry_iterator.h
        src/cadit/occt/custom_progress.h
)

add_executable(STP2GLB ${SOURCES} ${HEADERS})

if (NOT BUILD_STATIC)
    set_target_properties(STP2GLB PROPERTIES
            INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/bin")
endif ()

message(STATUS "CLI11_DIR: ${CLI11_DIR}")
if (BUILD_STATIC)
    target_include_directories(STP2GLB PRIVATE ${CLI11_DIR})

    if (UNIX)
        message(STATUS "Setting static compilation rules for unix")
        target_link_libraries(STP2GLB PRIVATE ${ADA_CPP_LINK_LIBS} pthread -static)
    else ()
        target_link_libraries(STP2GLB ${ADA_CPP_LINK_LIBS} -static)
        set_property(TARGET STP2GLB PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif ()

else ()
    target_link_libraries(STP2GLB ${ADA_CPP_LINK_LIBS})
endif ()

# install to executable into the bin dir
message(STATUS "Installing executable to ${EXE_BIN_DIR}")
install(TARGETS STP2GLB DESTINATION ${EXE_BIN_DIR})

# Include the tests directory
OPTION(BUILD_TESTING "Build the testing tree." ON)
if (BUILD_TESTING)
    message(STATUS "Building the testing tree.")
    enable_testing()
    include(tests/tests.cmake)
endif (BUILD_TESTING)

