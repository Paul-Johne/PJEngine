include(CMakeParseArguments)
# -------- CMake Vulkan Shader Compilation Helper --------
# Creates a custom target that compiles a given set of shader files into SPIR-V using "glslc".
# Optionally, it can apply the spirv-opt optimizer to the SPIR-V output.
# CREATED BY FABIAN FRIEDERICHS
# --------------------------------------------------------
# Usage:
#   compile_shaders(
#       target_name
#       SOURCES
#           shader_sources
#       OUTPUT_DIRECTORY
#           output_directory
#       [COMPILE_OPTIONS
#           option option option ...]
#       [OPIMIZE_SPIRV]
#       [OPTIMIZER_OPTIONS
#           option option option ...]
#   )
# --------------------------------------------------------
# Parameters:
#   target_name:        Name of the target that should be defined.
#   SOURCES:            List of all shader sources to compile.
#                       Each source file will be compiled to an output
#                       file with the postfix ".spv" appended.
#   OUTPUT_DIRECTORY:   Output directory into which the compiled .spv files will be written.
#   COMPILE_OPTIONS:    If defined specifies command line options fed to glslc.
#                       Must be a whitespace-separated list of strings.
#   OPTIMIZE_SPIRV:     If defined, apply the optimizer "spirv-opt" after compilation
#                       with glslc.
#   OPTIMIZER_OPTIONS:  If defined, specifies command line options fed to the optimizer.
#                       Must be a whitespace-separated list of strings.
# --------------------------------------------------------

function(compile_shaders target_name)
    # parse arguments
    set(options
        OPTIMIZE_SPIRV
    )
    set(one_value_params
        OUTPUT_DIRECTORY
    )
    set(multi_value_params
        SOURCES
        COMPILE_OPTIONS
        OPTIMIZER_OPTIONS
    )
    cmake_parse_arguments(
        "compsh"
        "${options}"
        "${one_value_params}"
        "${multi_value_params}"
        ${ARGN}
    )
    set(spirv_output_files "")
    foreach(source_path IN ITEMS ${compsh_SOURCES})
        set(source_name "")
        get_filename_component(source_name "${source_path}" NAME)
        set(output_path "${compsh_OUTPUT_DIRECTORY}/${source_name}.spv")
        list(APPEND spirv_output_files "${output_path}")
        if(compsh_OPTIMIZE_SPIRV)
            add_custom_command(
                OUTPUT "${output_path}"
                COMMAND ${CMAKE_COMMAND} -E echo "[glslc]: Compiling shader source \"${source_path}\"..."
                COMMAND glslc -o "${output_path}" "${source_path}" ${compsh_COMPILE_OPTIONS}
                COMMAND ${CMAKE_COMMAND} -E echo "[spirv-opt]: Optimizing SPIR-V output \"${output_path}\"..."
                COMMAND spirv-opt "${output_path}" -o "${output_path}" ${compsh_OPTIMIZER_OPTIONS}
                COMMAND ${CMAKE_COMMAND} -E echo "... done!"
                DEPENDS "${source_path}"
                VERBATIM            
            )  
        else()
            add_custom_command(
                OUTPUT "${output_path}"
                COMMAND ${CMAKE_COMMAND} -E echo "[glslc]: Compiling shader source \"${source_path}\"..."
                COMMAND glslc -o "${output_path}" "${source_path}" ${compsh_COMPILE_OPTIONS}
                COMMAND ${CMAKE_COMMAND} -E echo "... done!"
                DEPENDS "${source_path}"
                VERBATIM
            )            
        endif()  
    endforeach()
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${shader_source_files})
    add_custom_target(
        ${target_name}
        DEPENDS ${spirv_output_files}
        SOURCES ${shader_source_files}
    )    
endfunction()