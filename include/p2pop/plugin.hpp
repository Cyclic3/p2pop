#pragma once

#include "p2pop/base.hpp"

#include "p2pop/network_prelude.hpp"
#include "msg.pb.h"

namespace p2pop::plugin {
  struct Service {
    virtual status run_method(mid_t mid, const net::peer_info& peer,
                              data_const_ref in,
                              std::string& out) = 0;
    virtual puid_t get_puid() const noexcept = 0;
    virtual ~Service() = default;
  };
}
