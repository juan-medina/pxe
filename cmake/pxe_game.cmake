# SPDX-FileCopyrightText: 2026 Juan Medina
# SPDX-License-Identifier: MIT

set(PXE_MODULE_DIR ${CMAKE_CURRENT_LIST_DIR})
set(PXE_SCRIPTS_DIR ${PXE_MODULE_DIR}/../scripts)

function(pxe_add_game TARGET_NAME)
    if (NOT TARGET_NAME)
        message(FATAL_ERROR "pxe_add_game: TARGET_NAME required")
    endif ()

    file(GLOB_RECURSE APP_HEADER_FILES "src/*.hpp")
    file(GLOB_RECURSE APP_SOURCE_FILES "src/*.cpp")

    set(EMSCRIPTEN FALSE)
    if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        set(EMSCRIPTEN TRUE)
    endif ()

    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    set(VERSION_JSON ${CMAKE_SOURCE_DIR}/resources/version/version.json)
    set(VERSION_SCRIPT ${PXE_SCRIPTS_DIR}/version.py)

    # --- Version bump toggle ---
    option(ENABLE_VERSION_BUMP "Increment build number at build time" ON)

    # --- Version bump (optional, all platforms, build-time) ---
    if (ENABLE_VERSION_BUMP)
        add_custom_target(version_bump
                COMMAND ${CMAKE_COMMAND} -E echo "Incrementing build number..."
                COMMAND ${Python3_EXECUTABLE} ${VERSION_SCRIPT} ${VERSION_JSON}
                BYPRODUCTS ${VERSION_JSON}
                COMMENT "Version bumped"
                VERBATIM
        )
    endif ()

    # --- Windows resource generation (ONLY Windows) ---
    if (WIN32)
        enable_language(RC)

        set(APP_ICON_ICO ${CMAKE_SOURCE_DIR}/src/res/icon.ico)
        set(APP_RC_OUT ${CMAKE_BINARY_DIR}/energy-swap.rc)

        # Make .rc depend on bumped json only when bumping is enabled
        if (ENABLE_VERSION_BUMP)
            set(_RC_DEPENDS version_bump ${VERSION_JSON} ${VERSION_SCRIPT} ${APP_ICON_ICO})
        else ()
            set(_RC_DEPENDS ${VERSION_JSON} ${VERSION_SCRIPT} ${APP_ICON_ICO})
        endif ()

        add_custom_command(
                OUTPUT ${APP_RC_OUT}
                COMMAND ${CMAKE_COMMAND} -E echo "Generating Windows version resource (.rc)..."
                COMMAND ${Python3_EXECUTABLE} ${VERSION_SCRIPT}
                --emit-rc
                ${VERSION_JSON}
                ${APP_RC_OUT}
                ${APP_ICON_ICO}
                "${PROJECT_NAME}"
                "${PROJECT_DESCRIPTION}"
                "${APP_COMPANY}"
                "${APP_COPYRIGHT}"
                DEPENDS ${_RC_DEPENDS}
                VERBATIM
        )
    endif ()

    # --- Create executable (include generated .rc on Windows) ---
    if (WIN32)
        add_executable(${TARGET_NAME}
                ${APP_HEADER_FILES}
                ${APP_SOURCE_FILES}
                ${APP_RC_OUT}
        )

        target_link_options(${TARGET_NAME} PRIVATE
                $<$<CONFIG:Debug>:/SUBSYSTEM:CONSOLE>
                $<$<NOT:$<CONFIG:Debug>>:/SUBSYSTEM:WINDOWS>
        )
    else ()
        add_executable(${TARGET_NAME}
                ${APP_HEADER_FILES}
                ${APP_SOURCE_FILES}
        )
    endif ()

    # Ensure bump happens before building the executable (only if enabled)
    if (ENABLE_VERSION_BUMP)
        add_dependencies(${TARGET_NAME} version_bump)
    endif ()

    if (WIN32)
        set_source_files_properties(${APP_RC_OUT} PROPERTIES LANGUAGE RC)
        source_group("Resources" FILES ${APP_RC_OUT})

        target_compile_definitions(${TARGET_NAME} PRIVATE
                WIN32_LEAN_AND_MEAN
                NOMINMAX
                NOUSER
                NOGDI
        )
    endif ()

    if (MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX /permissive-)
    else ()
        target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
    endif ()

    target_link_libraries(${TARGET_NAME} PRIVATE pxe)

    # Always merge resources (game root, engine under pxe/)
    set(MERGED_RESOURCES_DIR ${CMAKE_BINARY_DIR}/merged_resources)
    file(TO_CMAKE_PATH "${MERGED_RESOURCES_DIR}" MERGED_RESOURCES_DIR_CMAKE)
    add_custom_command(
        OUTPUT ${MERGED_RESOURCES_DIR}/.merged
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${MERGED_RESOURCES_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${MERGED_RESOURCES_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_SOURCE_DIR}/resources ${MERGED_RESOURCES_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_SOURCE_DIR}/external/pxe/resources ${MERGED_RESOURCES_DIR}/pxe
        COMMAND ${CMAKE_COMMAND} -E touch ${MERGED_RESOURCES_DIR}/.merged
        DEPENDS ${CMAKE_SOURCE_DIR}/resources ${CMAKE_SOURCE_DIR}/external/pxe/resources
        COMMENT "Merging game resources (root) and engine resources (under pxe/)"
        VERBATIM
    )
    add_custom_target(merge_resources_all DEPENDS ${MERGED_RESOURCES_DIR}/.merged)
    add_dependencies(${TARGET_NAME} merge_resources_all)

    if (EMSCRIPTEN)
        set(CMAKE_EXECUTABLE_SUFFIX ".html" PARENT_SCOPE)
        set(CUSTOM_SHELL ${CMAKE_SOURCE_DIR}/src/web/template.html)
        target_link_options(${TARGET_NAME} PRIVATE
                "--shell-file=${CUSTOM_SHELL}"
                "--preload-file=${MERGED_RESOURCES_DIR_CMAKE}@resources"
                "-sUSE_GLFW=3"
                "-sALLOW_MEMORY_GROWTH=1"
                "--bind"
                "-sEXPORTED_RUNTIME_METHODS=['HEAPF32','HEAP32','HEAPU8','requestFullscreen']"
        )
        set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME "index")
        configure_file(${CMAKE_SOURCE_DIR}/src/res/icon.ico favicon.ico COPYONLY)
    else ()
        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Copying merged resources to output directory..."
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                ${MERGED_RESOURCES_DIR}
                $<TARGET_FILE_DIR:${TARGET_NAME}>/resources
            COMMENT "Copying merged resources (game root, engine under pxe/) to output resources directory"
            VERBATIM
        )
    endif ()

    target_compile_definitions(${TARGET_NAME} PRIVATE SPDLOG_USE_STD_FORMAT)

endfunction()
