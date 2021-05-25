find_path(AGIOS_INCLUDE_DIR
    NAMES agios.h
)

find_library(AGIOS_LIBRARY
    NAMES agios
)

set(AGIOS_INCLUDE_DIRS ${AGIOS_INCLUDE_DIR})
set(AGIOS_LIBRARIES ${AGIOS_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AGIOS DEFAULT_MSG AGIOS_LIBRARIES AGIOS_INCLUDE_DIRS)

mark_as_advanced(
    AGIOS_LIBRARY
    AGIOS_INCLUDE_DIR
)