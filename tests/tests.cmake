add_test(NAME stp_glb_cli_bspline_surf_test COMMAND STP2GLB
        --stp ${CMAKE_CURRENT_SOURCE_DIR}/temp/output.stp
        --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/flat_plate_abaqus_10x10_m_wColors.glb
        --version 0
        WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
)

add_test(NAME stp_glb_cli_flat_plate_v1 COMMAND STP2GLB
        --stp ${CMAKE_CURRENT_SOURCE_DIR}/files/flat_plate_abaqus_10x10_m_wColors.stp
        --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/flat_plate_abaqus_10x10_m_wColors.glb
        --version 1
        WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
)

add_test(NAME stp_glb_cli_flat_plate_v2 COMMAND STP2GLB
        --stp ${CMAKE_CURRENT_SOURCE_DIR}/files/flat_plate_abaqus_10x10_m_wColors.stp
        --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/flat_plate_abaqus_10x10_m_wColors.glb
        --version 2
        WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
)

add_test(NAME stp_glb_cli_flat_plate_v3 COMMAND STP2GLB
        --stp ${CMAKE_CURRENT_SOURCE_DIR}/files/flat_plate_abaqus_10x10_m_wColors.stp
        --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/flat_plate_abaqus_10x10_m_wColors.glb
        --version 3
        WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
)

add_test(NAME stp_glb_cli_large_v2 COMMAND STP2GLB
        --stp "${CMAKE_CURRENT_SOURCE_DIR}/temp/really_large.stp"
        --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/output.glb
        --version 2
        WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
)

add_test(NAME stp_glb_cli_large_v3 COMMAND STP2GLB
        --stp "${CMAKE_CURRENT_SOURCE_DIR}/temp/really_large.stp"
        --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/output.glb
        --version 3
        WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
)