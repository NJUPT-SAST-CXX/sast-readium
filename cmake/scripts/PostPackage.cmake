# PostPackage.cmake - Post-packaging operations (checksums, verification)

message(STATUS "Running post-package operations...")

# Get package directory
if(NOT DEFINED CPACK_PACKAGE_DIRECTORY)
    set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()

# Generate SHA256 checksums for all packages
file(GLOB packages "${CPACK_PACKAGE_DIRECTORY}/*")
set(checksum_count 0)

foreach(package ${packages})
    # Skip directories and checksum files
    if(IS_DIRECTORY "${package}" OR package MATCHES "\\.sha256$")
        continue()
    endif()

    get_filename_component(package_name "${package}" NAME)

    # Generate SHA256
    file(SHA256 "${package}" checksum)
    file(WRITE "${package}.sha256" "${checksum}  ${package_name}\n")

    message(STATUS "  Generated SHA256 for ${package_name}")
    math(EXPR checksum_count "${checksum_count} + 1")
endforeach()

# Create consolidated checksums file
set(all_checksums_file "${CPACK_PACKAGE_DIRECTORY}/SHA256SUMS")
file(WRITE "${all_checksums_file}" "# SHA256 Checksums for SAST Readium Packages\n")
file(APPEND "${all_checksums_file}" "# Generated: ")

execute_process(
    COMMAND ${CMAKE_COMMAND} -E echo_append ""
    COMMAND date
    OUTPUT_VARIABLE date_output
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
file(APPEND "${all_checksums_file}" "${date_output}\n\n")

foreach(package ${packages})
    if(IS_DIRECTORY "${package}" OR package MATCHES "\\.sha256$")
        continue()
    endif()

    get_filename_component(package_name "${package}" NAME)
    file(SHA256 "${package}" checksum)
    file(APPEND "${all_checksums_file}" "${checksum}  ${package_name}\n")
endforeach()

message(STATUS "Generated ${checksum_count} package checksums")
message(STATUS "Consolidated checksums: ${all_checksums_file}")

# Create package manifest
set(manifest_file "${CPACK_PACKAGE_DIRECTORY}/MANIFEST.txt")
file(WRITE "${manifest_file}" "SAST Readium Package Manifest\n")
file(APPEND "${manifest_file}" "================================\n\n")
file(APPEND "${manifest_file}" "Version: ${CPACK_PACKAGE_VERSION}\n")
file(APPEND "${manifest_file}" "Build Type: ${CMAKE_BUILD_TYPE}\n")
file(APPEND "${manifest_file}" "Platform: ${CMAKE_SYSTEM_NAME}\n")
file(APPEND "${manifest_file}" "Architecture: ${CMAKE_SYSTEM_PROCESSOR}\n\n")
file(APPEND "${manifest_file}" "Packages:\n")

foreach(package ${packages})
    if(IS_DIRECTORY "${package}" OR package MATCHES "\\.(sha256|txt)$")
        continue()
    endif()

    get_filename_component(package_name "${package}" NAME)
    file(SIZE "${package}" package_size)
    math(EXPR package_size_mb "${package_size} / 1048576")

    file(APPEND "${manifest_file}" "  - ${package_name} (${package_size_mb} MB)\n")
endforeach()

message(STATUS "Package manifest created: ${manifest_file}")
message(STATUS "Post-package operations completed")
