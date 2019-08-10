find_path(XXHASH_INCLUDE_DIRS xxhash.h)
find_library(XXHASH_LIBRARIES xxhash)

find_package_handle_standard_args(xxhash DEFAULT_MSG
  XXHASH_INCLUDE_DIRS XXHASH_LIBRARIES
)
