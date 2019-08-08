#
# - Try to find Facebook zstd library
# This will define
# ZSTD_FOUND
# ZSTD_INCLUDE_DIR
# ZSTD_LIBRARIES
#

find_path(ZSTD_INCLUDE_DIR
        NAMES zstd.h
)

find_library(ZSTD_LIBRARY
        NAMES zstd
)

set(ZSTD_LIBRARIES ${ZSTD_LIBRARY})
set(ZSTD_INCLUDE_DIRS ${ZSTD_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(ZSTD
    DEFAULT_MSG  ZSTD_LIBRARY ZSTD_INCLUDE_DIR
)

mark_as_advanced(
    ZSTD_LIBRARY
    ZSTD_INCLUDE_DIR
)