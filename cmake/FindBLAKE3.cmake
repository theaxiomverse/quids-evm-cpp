find_path(BLAKE3_INCLUDE_DIR
    NAMES blake3.h
    PATHS
        /usr/local/include
        /usr/include
)

find_library(BLAKE3_LIBRARY
    NAMES blake3 libblake3
    PATHS
        /usr/local/lib
        /usr/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BLAKE3
    REQUIRED_VARS
        BLAKE3_LIBRARY
        BLAKE3_INCLUDE_DIR
)

if(BLAKE3_FOUND AND NOT TARGET BLAKE3::BLAKE3)
    add_library(BLAKE3::BLAKE3 UNKNOWN IMPORTED)
    set_target_properties(BLAKE3::BLAKE3 PROPERTIES
        IMPORTED_LOCATION "${BLAKE3_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${BLAKE3_INCLUDE_DIR}"
    )
endif() 