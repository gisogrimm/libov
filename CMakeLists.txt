cmake_minimum_required(VERSION 3.16.0)

### GLOBAL CONFIGURATION
project(ov
        LANGUAGES CXX
        VERSION 0.5
        DESCRIPTION "common header files and library code for ov-client and ov-server"
        )
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
if (APPLE)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FixAppleArchitecture.cmake)
endif (APPLE)


### ADDITIONAL FLAGS
include("${CMAKE_CURRENT_LIST_DIR}/version.cmake")
if (UNIX)
    execute_process(
            COMMAND "${CMAKE_CURRENT_LIST_DIR}/get_version.sh"
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            RESULT_VARIABLE VERSION_RETURN_CODE
            OUTPUT_VARIABLE OVBOXVERSION
    )
    if (NOT VERSION_RETURN_CODE STREQUAL "0")
        message(FATAL_ERROR "Could not determine version")
    endif ()
    string(STRIP "${OVBOXVERSION}" OVBOXVERSION)
    if (NOT ${OVBOXVERSION} STREQUAL ${PROJECT_VERSION})
        message(FATAL_ERROR "Please check version.cmake and get_version.sh, since they deliver different versions!\nversion.cmake provides  ${PROJECT_VERSION}\nget_version.sh provides ${OVBOXVERSION}")
    endif ()
endif ()
message("Building version ${PROJECT_VERSION}")
set(PLUGINPREFIX "ovclient")
add_compile_definitions(PLUGINPREFIX="ovclient")
option(DEBUG_EVENTS "Debug events to console" ON)
if (DEBUG_EVENTS)
    add_compile_definitions(DEBUG_EVENTS)
endif (DEBUG_EVENTS)
if (LINUX)
    add_compile_definitions(LINUX)
elseif (APPLE)
    add_compile_definitions(OSX)
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
add_compile_definitions(OVBOXVERSION="${PROJECT_VERSION}")
add_compile_definitions(${CMAKE_SYSTEM_PROCESSOR})


### SOURCE MANAGEMENT
file(GLOB
        HEADER
        src/*.h
        )
file(GLOB
        SOURCES
        src/*.cc
        )


### DEPENDENCIES
if (UNIX AND NOT APPLE)
    if (EXISTS "/usr/lib/x86_64-linux-gnu/cmake")
        message("FIX: Resolving a bug produced by malformed cpprestsdk packages, see: https://bugs.launchpad.net/ubuntu/+source/cpprest/+bug/1838826")
        set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};/usr/lib/x86_64-linux-gnu/cmake")
    endif ()
    if (EXISTS "/usr/lib/arm-linux-gnueabihf/cmake")
        message("FIX: Resolving a bug produced by malformed cpprestsdk packages, see: https://bugs.launchpad.net/ubuntu/+source/cpprest/+bug/1838826")
        set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};/usr/lib/arm-linux-gnueabihf/cmake")
    endif ()
endif ()
if (APPLE)
    include(cmake/BrewResolver.cmake)
endif (APPLE)
add_subdirectory(tascar/libtascar libtascar EXCLUDE_FROM_ALL)
if (TARGET tascar)
    add_library(tascar::tascar ALIAS tascar)
    set(TASCAR_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/tascar/libtascar/include)
else ()
    find_package(tascar REQUIRED)
endif ()
find_package(Jack REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(cpprestsdk REQUIRED)
find_package(SoundIo REQUIRED)
find_package(LibXml++ REQUIRED)
if (UNIX AND NOT APPLE)
    find_package(ALSA REQUIRED)
endif ()



### TARGETS
# libov
add_library(ov STATIC ${SOURCES})
set_target_properties(tascar PROPERTIES PUBLIC_HEADER "${HEADER}")
target_include_directories(ov
        PRIVATE
        ${SOUNDIO_INCLUDE_DIR}
        ${LIBXML++_INCLUDE_DIRS}
        ${JACK_INCLUDE_DIRS}
        )
target_link_libraries(ov
        PRIVATE
        tascar::tascar
        ${CURL_LIBRARIES}
        nlohmann_json::nlohmann_json
        OpenSSL::Crypto
        cpprestsdk::cpprest
        ${SOUNDIO_LIBRARY}
        ${LIBXML++_LIBRARIES}
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
if (UNIX AND NOT APPLE)
    target_link_libraries(ov
            PRIVATE
            ALSA::ALSA
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