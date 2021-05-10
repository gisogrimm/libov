cmake_minimum_required(VERSION 3.16.0)

project(libov
        LANGUAGES CXX
        VERSION 0.5
        DESCRIPTION "common header files and library code for ov-client and ov-server"
        )


##
## CONFIGURATION
##
include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(POSITION_INDEPENDENT_CODE ON)

set(TARGET_NAME ${PROJECT_NAME})
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE PATH "Install path for include files")
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR} CACHE PATH "Install path for library files")

add_compile_definitions(PLUGINPREFIX="ovclient")
set(PLUGINPREFIX "ovclient")
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
add_compile_definitions(${CMAKE_SYSTEM_PROCESSOR})
add_compile_definitions(OVBOXVERSION="${PROJECT_VERSION}")


##
## DEPENDENCIES
##
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake/modules)
if (APPLE)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/FixAppleArchitecture.cmake)
    include(cmake/BrewResolver.cmake)
endif (APPLE)
if (UNIX AND NOT APPLE)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/CppRestHotfix.cmake)
endif (UNIX AND NOT APPLE)
find_package(Jack REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(cpprestsdk REQUIRED)
find_package(SoundIo REQUIRED)
find_package(XercesC REQUIRED)
if (UNIX AND NOT APPLE)
    find_package(ALSA REQUIRED)
endif ()
# Tascar
add_subdirectory(tascar/libtascar EXCLUDE_FROM_ALL)
target_compile_options(Tascar PRIVATE -Wno-error)
list(APPEND RECEIVERS
        ortf
        hrtf
        simplefdn
        omni
        )
list(APPEND TASCARMODS
        system
        touchosc
        waitforjackport
        route
        jackrec
        )
list(APPEND SOURCEMODS
        omni
        cardioidmod
        )
list(APPEND AUDIOPLUGINS
        sndfile
        delay
        metronome
        )
add_subdirectory(tascar/plugins)
get_property(TASCAR_PLUGIN_LIBRARIES GLOBAL PROPERTY TASCAR_PLUGIN_LIBRARIES)
foreach (TASCAR_PLUGIN_LIBRARY IN LISTS TASCAR_PLUGIN_LIBRARIES)
    add_library(Ov::Plugins::${TASCAR_PLUGIN_LIBRARY} ALIAS ${TASCAR_PLUGIN_LIBRARY})
    target_compile_options(${TASCAR_PLUGIN_LIBRARY} PRIVATE -Wno-error)
    list(APPEND OV_PLUGIN_LIBRARIES
            ${TASCAR_PLUGIN_LIBRARY})
    list(APPEND OV_PUBLIC_PLUGIN_LIBRARIES
            ${TASCAR_PLUGIN_LIBRARY})
endforeach ()
set_property(GLOBAL PROPERTY OV_PLUGIN_LIBRARIES)
set_property(GLOBAL PROPERTY OV_PUBLIC_PLUGIN_LIBRARIES)
add_subdirectory(libds EXCLUDE_FROM_ALL)


##
## TARGET
## create target and add include path
##
file(GLOB
        HEADER
        ${PROJECT_SOURCE_DIR}/src/*.h
        )
file(GLOB
        SOURCES
        ${PROJECT_SOURCE_DIR}/src/*.cc
        )
add_library(Ov STATIC ${SOURCES})
add_library(Ov::Ov ALIAS Ov)
target_compile_options(Ov PRIVATE -Wno-error)
set_target_properties(Ov PROPERTIES PUBLIC_HEADER "${HEADER}")
target_include_directories(Ov PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include/libov>)
target_link_libraries(Ov
        PUBLIC
        Tascar::Tascar
        DigitalStage::Api
        DigitalStage::Auth
        ${CURL_LIBRARIES}
        nlohmann_json::nlohmann_json
        OpenSSL::Crypto
        cpprestsdk::cpprest
        ${SOUNDIO_LIBRARY}
        )
set_target_properties(Ov PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        )
if (APPLE)
    target_link_libraries(Ov
            PUBLIC
            "-framework IOKit"
            "-framework CoreFoundation"
            )
endif ()
if (UNIX AND NOT APPLE)
    target_link_libraries(Ov
            PUBLIC
            ALSA::ALSA
            )
endif ()


##
## INSTALL
## install header files, generate and install cmake config files for find_package()
##
configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/Config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        INSTALL_DESTINATION ${LIB_INSTALL_DIR}/${PROJECT_NAME}/cmake
        PATH_VARS INCLUDE_INSTALL_DIR)
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        DESTINATION ${LIB_INSTALL_DIR}/cmake/${PROJECT_NAME})
export(
        TARGETS Ov Tascar ${OV_PLUGIN_LIBRARIES}
        NAMESPACE Ov::
        FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")
install(
        TARGETS Ov ${OV_PLUGIN_LIBRARIES}
        EXPORT ${TARGETS_EXPORT_NAME}
        PUBLIC_HEADER DESTINATION ${INCLUDE_INSTALL_DIR}/libov
        INCLUDES DESTINATION ${INCLUDE_INSTALL_DIR}/libov)
install(
        EXPORT ${TARGETS_EXPORT_NAME}
        NAMESPACE Ov::
        DESTINATION ${LIB_INSTALL_DIR}/cmake/${PROJECT_NAME})