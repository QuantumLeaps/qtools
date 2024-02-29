# QTools/qfsgen
message(STATUS "`qtools/qfsgen` - see \"https://www.state-machine.com/qtools/qfsgen.html\" for further details.")

# the main target
target_sources(qfsgen PRIVATE
    source/main.c
    ${PLATFORM_DIR}/filesearch.c
)

# add include dirs to target (-I...)
target_include_directories(qfsgen PRIVATE include)
