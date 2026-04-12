cmake_minimum_required(VERSION 3.10)
project(bcomp VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Stage 1: compile the core library (static library)
# find all .c files inside src/core recursively
file(GLOB_RECURSE CORE_SOURCES "src/core/*.c")
add_library(bcomp_core STATIC ${CORE_SOURCES})

target_include_directories(bcomp_core 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/core
)

# Stage 2: Compile the executable (CLI)
file(GLOB_RECURSE CLI_SOURCES "src/cli/*.c")
add_executable(bcomp ${CLI_SOURCES})

target_include_directories(bcomp
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)

# Link the CLI to the core library
target_link_libraries(bcomp PRIVATE bcomp_core)

set_target_properties(bcomp PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
