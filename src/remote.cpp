/*
 * #include "p2pop/remote.hpp"

namespace p2pop {
  status remote_node::dispatch(puid_t puid, mid_t mid,
                                       const std::string_view in,
                                       std::string &out) {
    proto::Packet req;
    req.set_puid(puid);
    req.set_mid(mid);
    req.set_params(in.data(), in.size());
    auto res = client->request(std::move(req));

    out = res.params();
    return deserialise(res.return_().status());
  }
}
*/
