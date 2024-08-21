cmake_minimum_required(VERSION 3.22.1)
project(haste-lang)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BINARY_NAME haste)
set(MAIN_FILE ./main.cpp)

file(GLOB_RECURSE CPP_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE C_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

# Dependencies
include(FetchContent)

# The Accual project
add_executable(${BINARY_NAME} ${MAIN_FILE} ${CPP_SRC_FILES} ${C_SRC_FILES})

target_include_directories(${BINARY_NAME} PUBLIC include/)

target_compile_features(${BINARY_NAME} PRIVATE cxx_std_20)

# Costom make command
add_custom_command(
    OUTPUT run_command
    COMMAND ${BINARY_NAME}
    DEPENDS ${BINARY_NAME}
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
    COMMENT "Running the program..."
)

add_custom_target(run run DEPENDS run_command)

