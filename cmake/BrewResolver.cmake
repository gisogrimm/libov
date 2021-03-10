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
        endif ()
    endif ()

    if (BREW AND NOT MACDEPLOYQT_EXECUTABLE AND NOT "$ENV{MACDEPLOYQT_EXECUTABLE}")
        execute_process(COMMAND ${BREW} --prefix qt@5
                OUTPUT_VARIABLE BREW_QT_PREFIX
                RESULT_VARIABLE BREW_QT_RESULT
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE
                )
        if (BREW_QT_RESULT EQUAL 0)
            message(STATUS "Set MACDEPLOYQT_EXECUTABLE=${BREW_QT_PREFIX}/bin/macdeployqt (from brew)")
            set(OPENSSL_ROOT_DIR "${BREW_OPENSSL_PREFIX}/bin/macdeployqt" CACHE PATH "")
        endif ()
    endif ()
endif (APPLE)