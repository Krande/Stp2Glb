add_test(NAME stp_glb_cli_basic COMMAND STP2GLB
        --stp ${CMAKE_CURRENT_SOURCE_DIR}/files/flat_plate_abaqus_10x10_m_wColors.stp
        --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/flat_plate_abaqus_10x10_m_wColors.glb
        WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
)

add_test(NAME stp_glb_cli_custom COMMAND STP2GLB
        --stp "C:/AibelProgs/downloads/Munin crane 100621 - AA040770.stp"
        --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/output.glb
        WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
)