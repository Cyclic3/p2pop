#include <grpcpp/server.h>

#include "p2pop/link.hpp"

#include "node.pb.h"
#include "node.proto.p2pop.hpp"

using namespace p2pop;

int main() {
  auto a_loc = net::render_location<net::Udp_Ipv6>("[::1]:6921");
  auto a_controller = net::Listener::listen(a_loc);
  auto b_loc = net::render_location<net::Udp_Ipv6>("[::1]:6922");
  auto b_controller = net::Listener::listen(b_loc);

  auto d = a_controller->contact(b_loc);

  auto b_nuid_real = b_controller->me();
  auto b_nuid_recv = d->remote().nuid;

  if (b_nuid_real != b_nuid_recv)
    throw std::runtime_error("nuid mismatch");

  return 0;
}
