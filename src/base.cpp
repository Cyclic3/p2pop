#include "p2pop/base.hpp"

#include <random>

namespace p2pop {
  nuid_t generate_random_nuid() {
    /// HACK: not _necessarily_ secure, but almost always is
    static thread_local std::random_device rng;

    nuid_t ret;

    std::generate(ret.begin(), ret.end(), []{ return rng(); });

    return ret;
  }

  proto::Nuid serialise(nuid_t n) {
    proto::Nuid ret;
    ret.set_value(n.data(), n.size());
    return ret;
  }
  nuid_t deserialise(const proto::Nuid& n) {
    if (n.value().size() != std::tuple_size_v<nuid_t>)
      throw std::runtime_error("Invalid nuid");

    nuid_t ret;
    std::copy(n.value().begin(), n.value().end(), ret.begin());
    return ret;
  }

  proto::Status serialise(const status& s) {
    proto::Status ret;
    ret.set_error_code(s.error_code());
    ret.set_msg(s.error_message());
    return ret;
  }
  status deserialise(const proto::Status& p) {
    return {static_cast<status_code>(p.error_code()), p.msg()};
  }
}
