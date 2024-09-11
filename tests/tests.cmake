add_test(NAME stp_glb_cli_basic COMMAND STP2GLB
        --stp ${CMAKE_CURRENT_SOURCE_DIR}/files/flat_plate_abaqus_10x10_m_wColors.stp
        --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/flat_plate_abaqus_10x10_m_wColors.glb
        WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
)