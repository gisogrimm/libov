cmake_minimum_required(VERSION 3.16.0)

## GLOBAL CONFIGURATION
project(ov
        LANGUAGES CXX
        VERSION 0.5
        DESCRIPTION "common header files and library code for ov-client and ov-server"
        )
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


## SOURCE MANAGEMENT
file(GLOB
        HEADER
        src/*.h
        )
file(GLOB
        SOURCES
        src/*.cc
        )
include_directories(src)

# Only use libtascar here (however, ov-client and ds-client will add tascar and so also plugins)
add_subdirectory(tascar/libtascar)
add_library(tascar::tascar ALIAS tascar)

## DEPENDENCIES
find_package(CURL REQUIRED)
find_package(cpprestsdk REQUIRED NAMES cpprestsdk cpprest)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(Jack REQUIRED)

## COMPILER ADDONS
if (LINUX)
    add_compile_definitions(LINUX)
elseif (APPLE)
    add_compile_definitions(OSX)
    #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework IOKit -framework CoreFoundation")
elseif (WIN32)
    add_compile_definitions(WIN32)
endif ()
if (CMAKE_SYSTEM_PROCESSOR MATCHES arm*)
    add_compile_definitions(ARM)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES x86_64)
    add_compile_definitions(AMD64)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES *86)
    add_compile_definitions(IA32)
endif ()
set(OVBOXVERSION ${PROJECT_VERSION})
add_compile_definitions(OVBOXVERSION="${OVBOXVERSION}")
add_compile_definitions(${CMAKE_SYSTEM_PROCESSOR})


## TARGET
add_library(ov ${SOURCES})
set_target_properties(tascar PROPERTIES PUBLIC_HEADER "${HEADER}")
target_include_directories(ov
        PRIVATE
        ${PROJECT_SOURCE_DIR}/tascar/libtascar/include
        PUBLIC
        ${CURL_INCLUDE_DIRS}
        ${JACK_INCLUDE_DIRS}
        )
target_link_libraries(ov
        PRIVATE
        tascar::tascar
        PUBLIC
        nlohmann_json::nlohmann_json
        cpprestsdk::cpprest
        OpenSSL::Crypto
        ${CURL_LIBRARIES}
        ${JACK_LIBRARIES}
        )
set_target_properties(ov PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        )
if (APPLE)
    target_link_libraries(ov
            PUBLIC
            "-framework IOKit"
            "-framework CoreFoundation"
            )
endif ()


# CMake package config
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Target")
set(INCLUDE_INSTALL_DIR "include/${PROJECT_NAME}" CACHE PATH "Install path for include files")
configure_package_config_file(
        "cmake/Config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
        PATH_VARS INCLUDE_INSTALL_DIR
)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake" COMPATIBILITY SameMajorVersion
)
install(
        TARGETS ov
        EXPORT "${TARGETS_EXPORT_NAME}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}"
)
install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)
install(
        EXPORT "${TARGETS_EXPORT_NAME}"
        NAMESPACE "ov::"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)