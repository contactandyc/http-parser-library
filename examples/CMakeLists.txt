cmake_minimum_required(VERSION 3.10)
project(MyProject)

find_package(amemorylibrary REQUIRED)
find_package(httpparserlibrary REQUIRED)

# Find the libuv package
find_package(libuv REQUIRED)

# Include libuv headers
include_directories(${LIBUV_INCLUDE_DIRS})

# List of executables
set(EXECUTABLES json_svc_request)

# Loop through each executable
foreach(EXECUTABLE IN LISTS EXECUTABLES)
    # Create executable
    add_executable(${EXECUTABLE} src/${EXECUTABLE}.c)

    # Set compile definitions
    target_compile_definitions(${EXECUTABLE} PUBLIC -D_AML_DEBUG_)

    # Set compile options
    target_compile_options(${EXECUTABLE} PRIVATE -g -O0)

    # Link libraries
    target_link_libraries(${EXECUTABLE} PRIVATE amemorylibrary_debug httpparserlibrary_debug uv_a)
endforeach()