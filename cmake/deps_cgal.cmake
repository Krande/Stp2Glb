# Find CGAL
find_package(CGAL REQUIRED)

if (CGAL_FOUND)
    message(STATUS "CGAL version found: " ${CGAL_VERSION})
    message(STATUS "CGAL include directory: " ${CGAL_INCLUDE_DIRS})
    message(STATUS "CGAL binary directory: " ${CGAL_DIR})
    message(STATUS "CGAL library directory: " ${CGAL_LIBRARY_DIRS})

    include_directories(${CGAL_INCLUDE_DIRS})
    link_directories(${CGAL_LIBRARY_DIRS})
endif (CGAL_FOUND)