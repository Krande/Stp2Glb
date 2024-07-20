# link the library files located in %LIBRARY_PREFIX%/lib/ifcparse/IfcParse.lib (on windows as an example)
# to the executable

list(APPEND
        ADA_CPP_LINK_LIBS
        IfcParse
        IfcGeom
)
