#pragma once

#include <grpcpp/impl/codegen/status.h>

//! Stolen from grpc, and tweaked a bit

namespace p2pop {
  using status_code = grpc::StatusCode;

  class status {
   public:
    /// Construct an OK instance.
    status() : code_(status_code::OK) {
      // Static assertions to make sure that the C++ API value correctly
      // maps to the core surface API value
      static_assert(status_code::OK == static_cast<status_code>(GRPC_STATUS_OK),
                    "Mismatched status code");
      static_assert(
          status_code::CANCELLED == static_cast<status_code>(GRPC_STATUS_CANCELLED),
          "Mismatched status code");
      static_assert(
          status_code::UNKNOWN == static_cast<status_code>(GRPC_STATUS_UNKNOWN),
          "Mismatched status code");
      static_assert(status_code::INVALID_ARGUMENT ==
                        static_cast<status_code>(GRPC_STATUS_INVALID_ARGUMENT),
                    "Mismatched status code");
      static_assert(status_code::DEADLINE_EXCEEDED ==
                        static_cast<status_code>(GRPC_STATUS_DEADLINE_EXCEEDED),
                    "Mismatched status code");
      static_assert(
          status_code::NOT_FOUND == static_cast<status_code>(GRPC_STATUS_NOT_FOUND),
          "Mismatched status code");
      static_assert(status_code::ALREADY_EXISTS ==
                        static_cast<status_code>(GRPC_STATUS_ALREADY_EXISTS),
                    "Mismatched status code");
      static_assert(status_code::PERMISSION_DENIED ==
                        static_cast<status_code>(GRPC_STATUS_PERMISSION_DENIED),
                    "Mismatched status code");
      static_assert(status_code::UNAUTHENTICATED ==
                        static_cast<status_code>(GRPC_STATUS_UNAUTHENTICATED),
                    "Mismatched status code");
      static_assert(status_code::RESOURCE_EXHAUSTED ==
                        static_cast<status_code>(GRPC_STATUS_RESOURCE_EXHAUSTED),
                    "Mismatched status code");
      static_assert(status_code::FAILED_PRECONDITION ==
                        static_cast<status_code>(GRPC_STATUS_FAILED_PRECONDITION),
                    "Mismatched status code");
      static_assert(
          status_code::ABORTED == static_cast<status_code>(GRPC_STATUS_ABORTED),
          "Mismatched status code");
      static_assert(status_code::OUT_OF_RANGE ==
                        static_cast<status_code>(GRPC_STATUS_OUT_OF_RANGE),
                    "Mismatched status code");
      static_assert(status_code::UNIMPLEMENTED ==
                        static_cast<status_code>(GRPC_STATUS_UNIMPLEMENTED),
                    "Mismatched status code");
      static_assert(
          status_code::INTERNAL == static_cast<status_code>(GRPC_STATUS_INTERNAL),
          "Mismatched status code");
      static_assert(status_code::UNAVAILABLE ==
                        static_cast<status_code>(GRPC_STATUS_UNAVAILABLE),
                    "Mismatched status code");
      static_assert(
          status_code::DATA_LOSS == static_cast<status_code>(GRPC_STATUS_DATA_LOSS),
          "Mismatched status code");
    }

    /// Construct an instance with associated \a code and \a error_message.
    /// It is an error to construct an OK status with non-empty \a error_message.
    status(status_code code, const std::string& error_message)
        : code_(code), error_message_(error_message) {}

    /// Construct an instance with \a code,  \a error_message and
    /// \a error_details. It is an error to construct an OK status with non-empty
    /// \a error_message and/or \a error_details.
    status(status_code code, const std::string& error_message,
           const std::string& error_details)
        : code_(code),
          error_message_(error_message),
          binary_error_details_(error_details) {}

    // Pre-defined special status objects.
    /// An OK pre-defined instance.
    static const status& OK;
    /// A CANCELLED pre-defined instance.
    static const status& CANCELLED;

    /// Return the instance's error code.
    status_code error_code() const { return code_; }
    /// Return the instance's error message.
    std::string error_message() const { return error_message_; }
    /// Return the (binary) error details.
    // Usually it contains a serialized google.rpc.Status proto.
    std::string error_details() const { return binary_error_details_; }

    /// Is the status OK?
    bool ok() const { return code_ == status_code::OK; }

    // Ignores any errors. This method does nothing except potentially suppress
    // complaints from any tools that are checking that errors are not dropped on
    // the floor.
    void IgnoreError() const {}

   private:
    status_code code_;
    std::string error_message_;
    std::string binary_error_details_;
  };
}
