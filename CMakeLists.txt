cmake_minimum_required(VERSION 3.16.0)

## GLOBAL CONFIGURATION
project(ov
        LANGUAGES CXX
        VERSION 0.5
        DESCRIPTION "common header files and library code for ov-client and ov-server"
        )
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(POSITION_INDEPENDENT_CODE ON)

# Include useful scripts for CMake
include(FetchContent)

set(PLUGINPREFIX "ovclient")
add_compile_definitions(PLUGINPREFIX="ovclient")

## SOURCE MANAGEMENT
file(GLOB
        HEADER
        src/*.h
        )
file(GLOB
        SOURCES
        src/*.cc
        )

add_subdirectory(tascar/libtascar libtascar)
if( TARGET tascar ) 
        add_library(tascar::tascar ALIAS tascar)
        set(TASCAR_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/tascar/libtascar/include)
else()
        find_package(tascar REQUIRED)
endif()

## DEPENDENCIES
if (APPLE)
    find_program(BREW brew)
    # only if OPENSSL_ROOT_DIR is not set yet
        if (BREW AND NOT OPENSSL_ROOT_DIR AND NOT "$ENV{OPENSSL_ROOT_DIR}")
                execute_process(COMMAND ${BREW} --prefix openssl
                OUTPUT_VARIABLE BREW_OPENSSL_PREFIX
                RESULT_VARIABLE BREW_OPENSSL_RESULT
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE
                )
                if (BREW_OPENSSL_RESULT EQUAL 0)
                message(STATUS "Set OPENSSL_ROOT_DIR=${BREW_OPENSSL_PREFIX} (from brew)")
                set(OPENSSL_ROOT_DIR "${BREW_OPENSSL_PREFIX}" CACHE PATH "")
                endif()
        endif()
elseif(WIN32)
  
endif()
find_package(CURL REQUIRED)
find_package(cpprestsdk REQUIRED NAMES cpprestsdk cpprest)
find_package(nlohmann_json 3.2.0 REQUIRED)

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
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES .*86)
    add_compile_definitions(IA32)
endif ()
set(OVBOXVERSION ${PROJECT_VERSION})
add_compile_definitions(OVBOXVERSION="${OVBOXVERSION}")
add_compile_definitions(${CMAKE_SYSTEM_PROCESSOR})


## TARGET
add_library(ov SHARED ${SOURCES})
##set_target_properties(tascar PROPERTIES PUBLIC_HEADER "${HEADER}")
target_include_directories(ov
        PUBLIC
        #${CURL_INCLUDE_DIRS}
		PRIVATE
		 ${CMAKE_CURRENT_LIST_DIR}/../third_party/getopt-for-windows
		 ${CMAKE_CURRENT_LIST_DIR}/../third_party/pthreads4w
		 ${CMAKE_CURRENT_LIST_DIR}/../third_party/unistd
        )
target_link_libraries(ov
        PUBLIC
        ##tascar::tascar
        cpprestsdk::cpprest
        ##OpenSSL::Crypto
        ${CURL_LIBRARIES}
        nlohmann_json::nlohmann_json
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
