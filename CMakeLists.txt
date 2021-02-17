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


## DEPENDENCIES
find_package(cpprestsdk REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
set(OPENSSL_USE_STATIC_LIBS TRUE)
#find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBXML++ REQUIRED libxml++-2.6)
add_subdirectory(tascar/libtascar)
add_library(tascar::tascar ALIAS tascar)


## COMPILER ADDONS
if (LINUX)
    add_compile_definitions(LINUX)
elseif (APPLE)
    add_compile_definitions(OSX)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework IOKit -framework CoreFoundation")
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
add_library(ov SHARED ${SOURCES} ${HEADER})
target_include_directories(ov
        PUBLIC
        ${LIBXML++_INCLUDE_DIRS}
        ${tascar_INCLUDE_DIRS}
        )
target_link_libraries(ov
        PUBLIC
        nlohmann_json::nlohmann_json
        cpprestsdk::cpprest
        OpenSSL::Crypto
        tascar::tascar
        )
set_target_properties(ov PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        )


# CMake package config
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Target")
write_basic_package_version_file(
        "${generated_dir}/ovConfigVersion.cmake" COMPATIBILITY SameMajorVersion
)
set(TARGETS_EXPORT_NAME "ovTarget")
configure_package_config_file(
        "cmake/Config.cmake.in"
        "${generated_dir}/ovConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ov"
)
install(
        TARGETS ov
        EXPORT "${TARGETS_EXPORT_NAME}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(
        FILES "${generated_dir}/ovConfig.cmake" "${generated_dir}/ovConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ov"
)
install(
        EXPORT "${TARGETS_EXPORT_NAME}"
        NAMESPACE "ov::"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ov"
)