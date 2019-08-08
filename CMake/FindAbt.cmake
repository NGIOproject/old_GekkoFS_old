find_path(ABT_INCLUDE_DIR
    NAMES abt.h
)

find_library(ABT_LIBRARY
    NAMES abt
)

set(ABT_INCLUDE_DIRS ${ABT_INCLUDE_DIR})
set(ABT_LIBRARIES ${ABT_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Abt DEFAULT_MSG ABT_LIBRARIES ABT_INCLUDE_DIRS)

mark_as_advanced(
        ABT_LIBRARY
        ABT_INCLUDE_DIR
)