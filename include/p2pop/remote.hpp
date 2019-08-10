#pragma once

#include "p2pop/base.hpp"

#include "p2pop/plugin.hpp"
#include "p2pop/network.hpp"

#include <map>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <atomic>

namespace p2pop {
  class net_client;
  /*

  class remote_node {
  private:
    std::unique_ptr<net::NetClient> client;
    std::atomic<nuid_t> last_nuid;

  public:
    inline plugin::peer_info get_info() {
      return {client->get_remote_nuid(), client->get_remote_address()};
    }
    status dispatch(puid_t puid, mid_t mid,
                    const std::string_view in, std::string& out);
  };*/
}
