# Get a list of all subdirectories in the current folder
file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)

foreach(child ${children})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
        # Check if the .cpp file exists (e.g., extensions/os/os.cpp)
        set(source_file "${child}/${child}.cpp")
        
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}")
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
        endif()
    endif()
endforeach()
