
include_directories(${STEPCODE_INCLUDE_DIR})
list(APPEND
        ADA_CPP_LINK_LIBS
        steplazyfile
        stepdai
        stepcore
        stepeditor
        steputils
)