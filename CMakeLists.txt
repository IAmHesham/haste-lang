cmake_minimum_required(VERSION 3.22.1)
project(haste-lang)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(BINARY_NAME haste)
set(MAIN_FILE ./main.cpp)

file(GLOB_RECURSE CPP_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE C_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

add_definitions(-DSOME_DEFINITION)

# Dependencies
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

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

project(haste-lang-test)

set(TEST_BINARY_NAME haste-test)
set(TEST_MAIN_FILE ./test/main.cpp)

file(GLOB_RECURSE CPP_TEST_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp")

add_executable(${TEST_BINARY_NAME} ${TEST_MAIN_FILE} ${CPP_TEST_SRC_FILES} ${CPP_SRC_FILES} ${C_SRC_FILES})
target_include_directories(${TEST_BINARY_NAME} PUBLIC include/)
target_link_libraries(${TEST_BINARY_NAME} PUBLIC GTest::gtest_main)

target_compile_features(${TEST_BINARY_NAME} PRIVATE cxx_std_20)
