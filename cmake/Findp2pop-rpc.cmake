# Shamelessly hacked from https://github.com/IvanSafonov/grpc-cmake-example, and tweaked by Cyclic3
function(P2POP_GENERATE_CPP HDRS)
  set(DEST ${CMAKE_CURRENT_BINARY_DIR})

  if(NOT ARGN)
    message(SEND_ERROR "Error: GRPC_GENERATE_CPP() called without any proto files")
    return()
  endif()

  if(TRUE) #GRPC_GENERATE_CPP_APPEND_PATH)
    # Create an include path for each file specified
    foreach(FIL ${ARGN})
      get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
      get_filename_component(ABS_PATH ${ABS_FIL} PATH)
      list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
      if(${_contains_already} EQUAL -1)
          list(APPEND _protobuf_include_path -I ${ABS_PATH})
      endif()
    endforeach()
  else()
    set(_protobuf_include_path -I ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  if(DEFINED PROTOBUF_IMPORT_DIRS)
    foreach(DIR ${PROTOBUF_IMPORT_DIRS})
      get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
      list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
      if(${_contains_already} EQUAL -1)
          list(APPEND _protobuf_include_path -I ${ABS_PATH})
      endif()
    endforeach()
  endif()

  set(${HDRS})
  foreach(FIL ${ARGN})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    get_filename_component(FIL_NAM ${FIL} NAME)

    list(APPEND ${HDRS} "${DEST}/${FIL_NAM}.p2pop.hpp")

    add_custom_command(
      OUTPUT "${DEST}/${FIL_NAM}.p2pop.hpp"
      COMMAND protobuf::protoc
      ARGS --custom_out ${DEST} ${_protobuf_include_path} --plugin=protoc-gen-custom=protoc-gen-p2pop ${ABS_FIL}
      DEPENDS ${ABS_FIL} protobuf::protoc protoc-gen-p2pop
      COMMENT "Running protoc-gen-p2pop on ${FIL}"
      VERBATIM)
  endforeach()

  set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
  set(${SRCS} ${${SRCS}} PARENT_SCOPE)
  set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(p2pop-rpc DEFAULT_MSG)
