# Find OpenCASCADE
find_package(OpenCASCADE REQUIRED)
if (OpenCASCADE_FOUND)
    message(STATUS "OpenCASCADE version found: " ${OpenCASCADE_MAJOR_VERSION} ".." ${OpenCASCADE_MINOR_VERSION} ".." ${OpenCASCADE_MAINTENANCE_VERSION})
    message(STATUS "OpenCASCADE include directory: " ${OpenCASCADE_INCLUDE_DIR})
    message(STATUS "OpenCASCADE binary directory: " ${OpenCASCADE_BINARY_DIR})
    message(STATUS "OpenCASCADE library directory: " ${OpenCASCADE_LIBRARY_DIR})

    include_directories(${OpenCASCADE_INCLUDE_DIR})
    link_directories(${OpenCASCADE_LIBRARY_DIR})
    # Order of linking is important
    # https://dev.opencascade.org/node/71506#comment-847
    list(APPEND
            ADA_CPP_LINK_LIBS
   "TKBin"     "TKCAF"       "TKDEPLY"  "TKG2d"      "TKMesh"    "TKShHealing" "TKXCAF"
   "TKBinL"    "TKCDF"       "TKDESTEP" "TKG3d"      "TKMeshVS"  "TKStd"       "TKXMesh"
   "TKBinTObj" "TKDE"        "TKDESTL"  "TKGeomAlgo" "TKOffset"  "TKStdL"      "TKXml"
   "TKBinXCAF" "TKDECascade" "TKDEVRML" "TKGeomBase" "TKOpenGl"  "TKTObj"      "TKXmlL"
   "TKBO"      "TKDEGLTF"    "TKernel"  "TKHLR"      "TKPrim"    "TKTopAlgo"   "TKXmlTObj"
   "TKBool"    "TKDEIGES"    "TKFeat"   "TKLCAF"     "TKRWMesh"  "TKV3d"       "TKXmlXCAF"
   "TKBRep"    "TKDEOBJ"     "TKFillet" "TKMath"     "TKService" "TKVCAF"      "TKXSBase"
    )
endif (OpenCASCADE_FOUND)