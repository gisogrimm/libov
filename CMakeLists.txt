cmake_minimum_required(VERSION 3.15)

project(libov)

set(SOURCES
	src/callerlist.cc
	src/callerlist.h
	src/common.cc
	src/common.h
	src/errmsg.cc
	src/errmsg.h
	src/MACAddressUtility.cc
	src/MACAddressUtility.h
	src/ovboxclient.cc
	src/ovboxclient.h
	src/ov_types.cc
	src/ov_types.h
	src/udpsocket.cc
	src/udpsocket.h
)

add_library(libov ${SOURCES})
add_compile_definitions(OVBOXVERSION="0.0.0" NOMINMAX)

# For Windows, where there is no getopt
target_include_directories(libov PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../third_party/getopt-for-windows ${CMAKE_CURRENT_LIST_DIR}/../third_party/pthreads4w)
