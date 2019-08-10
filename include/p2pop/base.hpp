#pragma once

#include <cstdint>

#include <array>
#include <chrono>

#include "p2pop/status.hpp"

#include "gsl/gsl-lite.hpp"

#include "base.pb.h"
#include "msg.pb.h"

namespace p2pop {
  // * unique ids are non-ambiguous
  // Protocol unique id
  using puid_t = uint64_t;
  // Call id
  using cid_t = uint64_t;
  // Node unique id
  using nuid_t = std::array<uint8_t, 20>;
  // Method id
  using mid_t = uint32_t;
  // Link unique id
  using luid_t = uint64_t;

  // This would be 6 fragments under UDP/IPv*/Ethernet MTU,
  // so see this as a hard limit, not a target.
  //
  // By my calculations, this would provide a ~0.6% failure chance
  // on UDP/IPv*/Internet
  //
  // Use channels for data of any significant size
  constexpr size_t max_buf_size = 8192;

  /// Should be accepted as well as the node's nuid
  ///
  /// This allows a node to initiate a handshake with the nearest node,
  /// without knowing it's nuid
  constexpr nuid_t all_nuids = {0};

  using age_t = std::chrono::microseconds;
  class timed_out : public std::exception {
    const char* what() const noexcept override {
      return "The operation timed out";
    }
  };

  using data_ref = gsl::span<uint8_t>;
  using data_const_ref = gsl::span<const uint8_t>;

  inline data_const_ref from_protobuf(const std::string& s) {
    data_const_ref ret {reinterpret_cast<const uint8_t*>(s.data()), s.size()};
    return ret;
  }

  nuid_t generate_random_nuid();

  proto::Nuid serialise(nuid_t);
  nuid_t deserialise(const proto::Nuid&);

  proto::Status serialise(const status&);
  status deserialise(const proto::Status&);

  template<luid_t Luid>
  constexpr age_t default_timeout() { return std::chrono::seconds{3}; }

  template<typename T>
  class movable_ptr {
  private:
    T* ptr;

  public:
    constexpr T* get() { return ptr; }
    constexpr const T* get() const { return ptr; }

    constexpr T* reset(T* p = nullptr) { T* ret = ptr; ptr = p; return ret; }

    constexpr operator T*() { return ptr; }
    constexpr operator const T*() const { return ptr; }
    constexpr T& operator*() { return *ptr; }
    constexpr const T& operator*() const { return *ptr; }
    constexpr T* operator->() { return ptr; }
    constexpr const T* operator->() const { return ptr; }
    constexpr operator bool() const { return ptr != nullptr; }

  public:
    constexpr movable_ptr(T* ptr) : ptr{ptr} {}

    movable_ptr(const movable_ptr&) = delete;
    movable_ptr& operator=(const movable_ptr&) = delete;

    constexpr movable_ptr(movable_ptr&& other) {
      ptr = other.ptr;
      other.ptr = nullptr;
    }
    constexpr movable_ptr& operator=(movable_ptr&& other) {
      new (this) movable_ptr(std::move(other));
      return *this;
    }

  public:
    constexpr bool operator==(const movable_ptr& other) { return ptr == other.ptr; }
    constexpr bool operator==(const T* other)           { return ptr == other; }
    constexpr bool operator< (const movable_ptr& other) { return ptr <  other.ptr; }
    constexpr bool operator< (const T* other)           { return ptr <  other; }
    constexpr bool operator> (const movable_ptr& other) { return ptr >  other.ptr; }
    constexpr bool operator> (const T* other)           { return ptr >  other; }
    constexpr bool operator<=(const movable_ptr& other) { return ptr <= other.ptr; }
    constexpr bool operator<=(const T* other)           { return ptr <= other; }
    constexpr bool operator>=(const movable_ptr& other) { return ptr >= other.ptr; }
    constexpr bool operator>=(const T* other)           { return ptr >= other; }
  };
}

