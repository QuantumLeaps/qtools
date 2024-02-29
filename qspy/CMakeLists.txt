# QTools/qspy
message(STATUS "`qtools/qspy` - see \"https://www.state-machine.com/qtools/qspy.html\" for further details.")

# the main target
target_sources(qspy PRIVATE
    source/qspy.c
    source/qspy_main.c
    source/qspy_be.c
    source/qspy_dict.c
    source/qspy_seq.c
    source/qspy_tx.c
    source/getopt.c
    ${PLATFORM_DIR}/qspy_pal.c
)

# add include dirs to target (-I...)
target_include_directories(qspy PRIVATE
    ${PLATFORM_DIR}
    include
)

# set defines (-D...)
target_compile_definitions(qspy PRIVATE QSPY_APP)

# add link libraries (-l...)
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries(qspy PRIVATE ws2_32)
endif()
