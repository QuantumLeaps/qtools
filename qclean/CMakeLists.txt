message(STATUS "`qtools/qclean` - see \"https://www.state-machine.com/qtools/qclean.html\" for further details.")

# the main target
target_sources(qclean PRIVATE
    source/main.c
    source/getopt.c
    ${PLATFORM_DIR}/filesearch.c
)

# add include dirs to target (-I...)
target_include_directories(qclean PRIVATE include)
