# Optional deps for extensions (ssh needs libssh)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBSSH libssh)
endif()

# Get a list of all subdirectories in the current folder
file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)

foreach(child ${children})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
        # Check if the .cpp file exists (e.g., extensions/os/os.cpp)
        set(source_file "${child}/${child}.cpp")

        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}")
            # zip extension requires libzip
            if(child STREQUAL "zip" AND NOT LIBZIP_FOUND)
                message(STATUS "Skipping extension ${child}: libzip not found")
            else()
                # Define the shared library name (e.g., ext_os)
                set(lib_name "ext_${child}")

                add_library(${lib_name} MODULE ${source_file})

                # Ensure the output name is exactly what you want (e.g., os.so)
                set_target_properties(${lib_name} PROPERTIES
                    PREFIX ""
                    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/modules
                    OUTPUT_NAME ${child}
                )

                # Optional: Include paths if they need headers from the core
                target_include_directories(${lib_name} PRIVATE ${MELT_INCLUDE_HEADERS})

                # zip extension links libzip
                if(child STREQUAL "zip" AND LIBZIP_FOUND)
                    target_include_directories(${lib_name} PRIVATE ${LIBZIP_INCLUDE_DIRS})
                    target_link_directories(${lib_name} PRIVATE ${LIBZIP_LIBRARY_DIRS})
                    target_link_libraries(${lib_name} PRIVATE ${LIBZIP_LIBRARIES})
                endif()

                # ssh extension links libssh (optional; stub if not found)
                if(child STREQUAL "ssh" AND LIBSSH_FOUND)
                    target_compile_definitions(${lib_name} PRIVATE USE_LIBSSH=1)
                    target_include_directories(${lib_name} PRIVATE ${LIBSSH_INCLUDE_DIRS})
                    target_link_directories(${lib_name} PRIVATE ${LIBSSH_LIBRARY_DIRS})
                    target_link_libraries(${lib_name} PRIVATE ${LIBSSH_LIBRARIES})
                    message(STATUS "SSH extension: building with libssh")
                elseif(child STREQUAL "ssh")
                    message(STATUS "SSH extension: building stub (libssh not found)")
                endif()

                # Extensions resolve Interpreter::registerBuiltin etc. from the main binary at load time.
                if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
                    target_link_options(${lib_name} PRIVATE "LINKER:-undefined,dynamic_lookup")
                endif()
            endif()
        endif()
    endif()
endforeach()
