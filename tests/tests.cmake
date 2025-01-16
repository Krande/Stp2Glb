add_test(NAME plate COMMAND STP2GLB
        --stp ${CMAKE_CURRENT_SOURCE_DIR}/files/flat_plate_abaqus_10x10_m_wColors.stp
        --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/flat_plate_abaqus_10x10_m_wColors.glb
        WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
)

add_test(NAME debug_plate COMMAND STP2GLB
        --stp ${CMAKE_CURRENT_SOURCE_DIR}/files/flat_plate_abaqus_10x10_m_wColors.stp
        --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/flat_plate_abaqus_10x10_m_wColors.glb
        --debug
        WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
)

add_test(NAME as1 COMMAND STP2GLB
        --stp "${CMAKE_CURRENT_SOURCE_DIR}/files/as1-oc-214.stp"
        --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/as1-oc-214-limited.glb
        WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
)

add_test(NAME debug_as1 COMMAND STP2GLB
        --stp "${CMAKE_CURRENT_SOURCE_DIR}/files/as1-oc-214.stp"
        --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/as1-oc-214-limited.glb
        --debug
        --solid-only
        --max-geometry-num=0
        WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
)

add_test(NAME debug_as1_filter COMMAND STP2GLB
        --stp "${CMAKE_CURRENT_SOURCE_DIR}/files/as1-oc-214.stp"
        --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/as1-oc-214-filtered.glb
        --debug
        --solid-only
        --filter-names="l-bracket"
        WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/temp/really_large.stp")
    add_test(NAME stp_glb_debug_large COMMAND STP2GLB
            --stp "${CMAKE_CURRENT_SOURCE_DIR}/temp/really_large.stp"
            --glb ${CMAKE_CURRENT_SOURCE_DIR}/temp/really_large-v2.glb
            --debug
            --solid-only
            --max-geometry-num=0
            --filter-names-file-exclude=${CMAKE_CURRENT_SOURCE_DIR}/temp/skip-these-nodes.txt
            WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin"
    )
endif ()