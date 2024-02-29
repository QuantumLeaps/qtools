# use a recent CMake version
cmake_minimum_required(VERSION 3.23 FATAL_ERROR)
cmake_policy(VERSION 3.23)
cmake_policy(SET CMP0083 NEW)

# first of all protect against in-source builds
file(REAL_PATH "${CMAKE_SOURCE_DIR}" _srcdir)
file(REAL_PATH "${CMAKE_BINARY_DIR}" _bindir)

if(${_srcdir} STREQUAL ${_bindir})
    message(FATAL_ERROR "  FATAL: In-source builds are not allowed!
        You should create a separate directory for build files.")
endif()
unset(_srcdir)
unset(_bindir)

# update CMAKE_MODULE_PATH
list(PREPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/source/cmake ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_SOURCE_DIR} ${HOME_PATH}/cmake)

if(NOT SW_VERSION)
    set(SW_VERSION "7.3.3" CACHE STRING "Software Version")
endif()

message(STATUS "`qtools` - see \"https://www.state-machine.com/qtools/\" for further details.")

# the `qtools` project, consisting of the tools listed below
# qtools -  https://www.state-machine.com/qtools/
project(
    qtools
    VERSION ${SW_VERSION}
    DESCRIPTION "QTools - https://www.state-machine.com/qtools/"
    HOMEPAGE_URL "https://www.state-machine.com/qtools/"
    LANGUAGES C
)

# select target platform specific source directory
if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL Windows)
    set(PLATFORM_DIR win32)
else()
    set(PLATFORM_DIR posix)
endif()

# the qclean tool -  https://www.state-machine.com/qtools/qclean.html
add_executable(qclean)
add_subdirectory(qclean)

# the qfsgen tool -  https://www.state-machine.com/qtools/qfsgen.html
add_executable(qfsgen)
add_subdirectory(qfsgen)

# the qspy tool -  https://www.state-machine.com/qtools/qspy.html
add_executable(qspy)
add_subdirectory(qspy)

# the `install` targets
set(QDATAROOT share/qtools)
set(QTARGETS qclean qfsgen qspy)
set(QDIRS qcalc qutest qview qwin)
set(QBIN cmock generate_test_runner python3 qcalc qspy_exit sha256)
list(TRANSFORM QBIN APPEND .bat)
list(APPEND QBIN Termite.ini)
list(TRANSFORM QBIN PREPEND bin/)

install(TARGETS ${QTARGETS})
install(PROGRAMS ${BIN_FILES} TYPE BIN)
install(DIRECTORY doc LICENSES DESTINATION ${QDATAROOT}/doc)
install(FILES README.md DESTINATION ${QDATAROOT}/doc)
install(DIRECTORY ${QDIRS} DESTINATION ${QDATAROOT}/data)
