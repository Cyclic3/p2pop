#pragma once

#include "base.hpp"

#include "location.pb.h"

namespace p2pop::net {
  struct peer_info {
    nuid_t nuid;
    proto::Location location;
  };
}
