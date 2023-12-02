cmake_minimum_required(VERSION 3.10)
project(httpparserlibrary VERSION 0.1.1)

# Options
option(DEBUG "Enable debugging" OFF)
option(ADDRESS_SANITIZER "Enable Address Sanitizer" OFF)

set(CMAKE_INSTALL_INCLUDEDIR include)
set(CMAKE_INSTALL_BINDIR bin)
set(CMAKE_INSTALL_DOCDIR share/doc/httpparserlibrary)
set(CMAKE_C_STANDARD 99)

# Set C standard
set(CMAKE_C_STANDARD 99)

# Find the required libraries
find_package(amemorylibrary REQUIRED)

# Find the libuv package
find_package(libuv REQUIRED)

# Include libuv headers
include_directories(${LIBUV_INCLUDE_DIRS})

# Compiler options
if(ADDRESS_SANITIZER)
    add_compile_options(-fsanitize=address)
endif()

# Source files
set(SOURCE_FILES src/hpl.c)

# Debug library
add_library(httpparserlibrary_debug STATIC ${SOURCE_FILES})
target_include_directories(httpparserlibrary_debug PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_definitions(httpparserlibrary_debug PUBLIC -D_AML_DEBUG_)
target_compile_options(httpparserlibrary_debug PRIVATE -g -O0)
set_target_properties(httpparserlibrary_debug PROPERTIES OUTPUT_NAME "httpparserlibrary_debug")
target_compile_options(httpparserlibrary_debug PRIVATE -g -O0)


# Release library
add_library(httpparserlibrary_static STATIC ${SOURCE_FILES})
target_include_directories(httpparserlibrary_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_options(httpparserlibrary_static PRIVATE -O3)
set_target_properties(httpparserlibrary_static PROPERTIES OUTPUT_NAME "httpparserlibrary_static")
target_compile_options(httpparserlibrary_static PRIVATE -O3)

# Shared library
add_library(httpparserlibrary SHARED ${SOURCE_FILES})
target_include_directories(httpparserlibrary PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_options(httpparserlibrary PRIVATE -O3)
set_target_properties(httpparserlibrary PROPERTIES OUTPUT_NAME "httpparserlibrary")
target_compile_options(httpparserlibrary PRIVATE -O3)

# Link libraries
target_link_libraries(httpparserlibrary_debug PUBLIC amemorylibrary uv_a)
target_link_libraries(httpparserlibrary_static PUBLIC amemorylibrary uv_a)
target_link_libraries(httpparserlibrary PUBLIC amemorylibrary uv)

# Installation of the library
install(TARGETS httpparserlibrary httpparserlibrary_static httpparserlibrary_debug
        EXPORT httpparserlibraryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include)

# Export the targets
install(EXPORT httpparserlibraryTargets
        FILE httpparserlibraryTargets.cmake
        DESTINATION lib/cmake/httpparserlibrary)

# Create and install the config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "httpparserlibraryConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

configure_file(httpparserlibraryConfig.cmake.in httpparserlibraryConfig.cmake @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/httpparserlibraryConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/httpparserlibraryConfigVersion.cmake"
        DESTINATION lib/cmake/httpparserlibrary)

# Installation of other files
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/http-parser-library DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES README.md AUTHORS NEWS.md CHANGELOG.md LICENSE NOTICE DESTINATION ${CMAKE_INSTALL_DOCDIR})

# Uninstall command
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/cmake_uninstall.cmake.in"
               "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
               IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
