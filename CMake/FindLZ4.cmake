
# - Find Lz4
# Find the lz4 compression library and includes
#
# LZ4_FOUND - True if lz4 found.
# LZ4_LIBRARIES - List of libraries when using lz4.
# LZ4_INCLUDE_DIR - where to find lz4.h, etc.

find_path(LZ4_INCLUDE_DIR
    NAMES lz4.h
)

find_library(LZ4_LIBRARY
    NAMES lz4
)

set(LZ4_LIBRARIES ${LZ4_LIBRARY} )
set(LZ4_INCLUDE_DIRS ${LZ4_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(lz4 DEFAULT_MSG LZ4_LIBRARY LZ4_INCLUDE_DIR)

mark_as_advanced(
    LZ4_LIBRARY
    LZ4_INCLUDE_DIR
)
