
# Read version
file(READ "${CMAKE_CURRENT_LIST_DIR}/Makefile" makefile)
string(REGEX MATCH "VERSION=([0-9]*).([0-9]*)" _ "${makefile}")
set(PROJECT_VERSION_MAJOR "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}")

# Read minorversion
message("Executing version script inside ${CMAKE_CURRENT_LIST_DIR}")
execute_process(COMMAND bash -c "git rev-list --count c700290e24cc00ab203f1784036871e03cd519f9..HEAD"
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
        OUTPUT_VARIABLE MINORVERSION)
string(REGEX REPLACE "\n$" "" MINORVERSION "${MINORVERSION}")
string(STRIP "${MINORVERSION}" MINORVERSION)
set(PROJECT_VERSION_MINOR "${MINORVERSION}")
execute_process(
        COMMAND bash -c "git rev-parse --short HEAD"
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
        OUTPUT_VARIABLE COMMIT)
execute_process(
        COMMAND bash -c "git diff --quiet --exit-code || echo -modified"
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
        OUTPUT_VARIABLE COMMITMOD)
string(REGEX REPLACE "\n$" "" COMMIT "${COMMIT}")
string(REGEX REPLACE "\n$" "" COMMITMOD "${COMMITMOD}")
string(STRIP "${COMMIT}" COMMIT)
string(STRIP "${COMMITMOD}" COMMITMOD)
set(PROJECT_VERSION_PATCH "${COMMIT}${COMMITMOD}")

set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}-${PROJECT_VERSION_PATCH}")
