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

add_test(NAME stp_glb_cli_as1_v2 COMMAND STP2GLB
        --stp "${CMAKE_CURRENT_SOURCE_DIR}/files/as1-oc-214.stp"
        --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/as1-oc-214-limited.glb
        --version 2
        --solid-only=true
        --max-geometry-num=0
        WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
)

add_test(NAME stp_glb_cli_as1_v2_filter COMMAND STP2GLB
        --stp "${CMAKE_CURRENT_SOURCE_DIR}/files/as1-oc-214.stp"
        --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/as1-oc-214-filtered.glb
        --version 2
        --solid-only=true
        --filter-names="l-bracket"
        WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/temp/really_large.stp")
    add_test(NAME stp_glb_cli_large_v2 COMMAND STP2GLB
            --stp "${CMAKE_CURRENT_SOURCE_DIR}/temp/really_large.stp"
            --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/really_large-v2.glb
            --version 2
            --solid-only=true
            --max-geometry-num=4000
            --filter-names-file-exclude=${CMAKE_CURRENT_SOURCE_DIR}/temp/skip-these-nodes.txt
            WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
    )

    add_test(NAME stp_glb_cli_large_v3 COMMAND STP2GLB
            --stp "${CMAKE_CURRENT_SOURCE_DIR}/temp/really_large.stp"
            --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/output.glb
            --version 3
            WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
    )
endif ()