find_path(BOTAN2_INCLUDE_DIRS botan/botan.h)
find_path(BOTAN2_INCLUDE_DIRS botan/botan.h PATH_SUFFIXES botan-2)
find_library(BOTAN2_LIBRARIES botan-2)

find_package_handle_standard_args(Botan2 DEFAULT_MSG
  BOTAN2_INCLUDE_DIRS BOTAN2_LIBRARIES
)
