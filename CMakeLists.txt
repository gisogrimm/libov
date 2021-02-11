cmake_minimum_required(VERSION 3.18.0)

## GLOBAL CONFIGURATION
project(libov 
    LANGUAGES CXX 
    VERSION 0.5 
    DESCRIPTION "common header files and library code for ov-client and ov-server"
)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB
    SOURCE_FILES
    src/*.h
    src/*.cc
)

if(LINUX)
    add_compile_definitions(LINUX)
elseif(APPLE)
    add_compile_definitions(OSX)
elseif(WIN32)
    add_compile_definitions(WIN32)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES arm*)
    add_compile_definitions(ARM)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES x86_64)
    add_compile_definitions(AMD64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES *86)
    add_compile_definitions(IA32)
endif()

add_compile_definitions(OVBOXVERSION="${PROJECT_VERSION}")
add_compile_definitions(${CMAKE_SYSTEM_PROCESSOR})

add_library(libov STATIC ${SOURCE_FILES})
