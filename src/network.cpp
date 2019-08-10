#include "p2pop/network.hpp"

#include <random>

namespace p2pop::net {
  cid_generator::cid_generator() {
    thread_local std::random_device rng;
    std::uniform_int_distribution<cid_t> dist;
    next_cid = dist(rng);
  }
}
