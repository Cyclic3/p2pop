#pragma once

#include "p2pop/base.hpp"

#include "p2pop/network_prelude.hpp"
#include "p2pop/plugin.hpp"
#include "msg.pb.h"
#include "location.pb.h"

#include <functional>

namespace p2pop::net {
  // Thread safe cid generator
  class cid_generator {
  private:
    std::atomic<cid_t> next_cid;

  public:
    inline cid_t reserve_cid() { return ++next_cid; }
    inline cid_t operator()() { return reserve_cid(); }

  public:
    cid_generator();
  };

  // Sets cid, source and destination
  //
  // Cids are unique for each request/response pair, regardless of content
  struct Dispatcher {
    /// MUST NEVER change after construction
    virtual const peer_info& remote() const = 0;
    /// Dispatch with the default timeout for this link
    virtual proto::Packet dispatch(proto::Packet&& packet) = 0;
    /// Dispatch with the given timeout
    virtual proto::Packet dispatch(proto::Packet&& packet, age_t age) = 0;
    virtual ~Dispatcher() = default;
  };

  struct Controller {
    // Get the link id of this controller
    virtual luid_t link() const noexcept = 0;
    virtual nuid_t me() const noexcept = 0;
    virtual void attach(plugin::Service*) = 0;
    /// XXX: using the dispatcher after the deconstruction of the parent Controller
    /// will result in undefined behaviour
    virtual std::unique_ptr<Dispatcher> contact(const proto::Location& loc) = 0;
    /// The default definition just forwards to contact. You probably don't want that
    virtual std::unique_ptr<Dispatcher> dial(const peer_info& inf) {
      return contact(inf.location);
    }
    virtual ~Controller() = default;
  };
}
