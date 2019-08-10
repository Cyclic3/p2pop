cmake_minimum_required(VERSION 3.9)

project(p2pop VERSION 0.0.1)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)
find_package(TBB REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(Protobuf REQUIRED)
find_package(p2pop-rpc REQUIRED)
find_package(xxhash REQUIRED)
find_package(Botan2 REQUIRED)

file(GLOB_RECURSE source src/*.cpp)
file(GLOB_RECURSE plugin_source plugin/*.cpp)
file(GLOB_RECURSE proto proto/*.proto)

protobuf_generate_cpp(proto_source proto_headers ${proto})
p2pop_generate_cpp(rpc_headers ${proto})

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/fmt/include)
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/gsl-lite/include)
include_directories(SYSTEM ${TBB_INCLUDE_DIRS})
include_directories(SYSTEM ${BOOST_INCLUDE_DIRS})
include_directories(SYSTEM ${PROTOBUF_INCLUDE_DIRS})
include_directories(SYSTEM ${XXHASH_INCLUDE_DIRS})
include_directories(SYSTEM ${BOTAN2_INCLUDE_DIRS})
include_directories(SYSTEM ${CMAKE_CURRENT_BINARY_DIR}) # HACK: actually include properly

set(libs ${CMAKE_THREAD_LIBS_INIT} ${PROTOBUF_LIBRARIES} ${TBB_LIBRARIES} ${Boost_LIBRARIES} ${XXHASH_LIBRARIES} ${BOTAN2_LIBRARIES})

if(WIN32)
  file(GLOB_RECURSE platform_source_tmp src/*.WINDOWS.cxx)
  set(platform_source ${platform_source} ${platform_source_tmp})
endif()

# Handle all unicies (Linux, OSX, etc)
if(UNIX)
  file(GLOB_RECURSE platform_source_tmp src/*.UNIX.cxx)
  set(platform_source ${platform_source} ${platform_source_tmp})
endif()

if(APPLE)
  file(GLOB_RECURSE platform_source_tmp src/*.APPLE.cxx)
  set(platform_source ${platform_source} ${platform_source_tmp})
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
  file(GLOB_RECURSE platform_source_tmp src/*.LINUX.cxx)
  set(platform_source ${platform_source} ${platform_source_tmp})
endif()

add_executable(protoc-gen-${PROJECT_NAME} ${plugin_source} ${proto_source})
target_link_libraries(protoc-gen-${PROJECT_NAME} ${libs} ${Protobuf_PROTOC_LIBRARIES})

add_library(${PROJECT_NAME} ${source} ${platform_source} ${proto_source} ${rpc_source} ${rpc_headers})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${PROJECT_NAME} ${libs})



file(GLOB_RECURSE tests tests/*.cxx)

foreach(test ${tests})
  file(RELATIVE_PATH test_rel ${CMAKE_SOURCE_DIR}/tests/ ${test})
  get_filename_component(test_fname ${test_rel} NAME_WE)
  get_filename_component(test_dir ${test_rel} DIRECTORY)
  #string(REGEX REPLACE "/" "_" test_dir ${test_dir})

  set(test_name ${test_dir}_${test_fname})

  add_executable(${test_name} ${test})

  target_link_libraries(${test_name} ${PROJECT_NAME})

  add_test(${test_name} ${test_name})
endforeach()

enable_testing()
