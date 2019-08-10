#pragma once

#include "base.hpp"

#include "network.hpp"
#include "location.pb.h"

namespace p2pop::net {
  enum common_luid : luid_t {
    Udp_Ipv4 = 0,
    Udp_Ipv6 = 1,
  };

  class Listener {
  public:
    virtual std::unique_ptr<Controller> our_listen(data_const_ref location_dat,
                                                   nuid_t nuid = generate_random_nuid(),
                                                   data_const_ref nuid_privkey = { nullptr, 0 }) const = 0;

  private:
    // XXX: No thread safety of any kind. Be very careful
    static void register_listener(luid_t, const Listener*);
  public:
    static std::unique_ptr<Controller> listen(luid_t luid,
                                              data_const_ref location_dat,
                                              nuid_t nuid = generate_random_nuid(),
                                              data_const_ref nuid_privkey = { nullptr, 0 });
    inline static std::unique_ptr<Controller> listen(const proto::Location& location,
                                                     nuid_t nuid = generate_random_nuid(),
                                                     data_const_ref nuid_privkey = { nullptr, 0 }) {
      return listen(location.luid(), from_protobuf(location.data()), nuid, nuid_privkey);
    }

  public:
    /// XXX: Use P2POP_NET_REG_LISTENER* in a non-header source file. This is non-negotiable.
    ///
    /// Whatever 'cool hack' you've just thought of, please don't do it.
    ///
    /// Just use a custom interface or something.
    ///
    /// Please I'm begging you.
    inline Listener(luid_t our_luid) {
      register_listener(our_luid, this);
    }
    virtual ~Listener() = default;

    Listener(const Listener&) = delete;
    Listener(Listener&&) = delete;

    Listener& operator=(const Listener&) = delete;
    Listener& operator=(Listener&&) = delete;
  };

  // Because I can't get templates to work
  template<luid_t luid>
  struct luid_tag {};
//  template<common_luid luid>
//  using common_luid_tag = luid_tag<static_cast<luid_t>(luid)>;

#define _P2POP_NET_LINK_LISTENER_REG_NAME(A) \
  __CONCAT(_p2pop_net_link_listener_instance_, A)

#define P2POP_NET_LINK_LISTENER_REG(LISTENER_TYPE) \
  static_assert(std::is_base_of_v<::p2pop::net::Listener, LISTENER_TYPE>, \
                "P2POP_NET_REG_LISTENER only registers ::p2pop::net::Listeners. "\
                "I thought that was obvious, but apparently not."); \
  static const LISTENER_TYPE _P2POP_NET_LINK_LISTENER_REG_NAME(__COUNTER__);

  using ipv4_address = std::array<uint8_t, 4>;
  using ipv6_address = std::array<uint8_t, 16>;
  using epv4 = std::pair<ipv4_address, uint16_t>;
  using epv6 = std::pair<ipv6_address, uint16_t>;


  template<luid_t luid, typename... Args>
  proto::Location render_location(Args... args) {
    proto::Location ret;
    ret.set_luid(luid);
    ret.set_data(render_location_internal(luid_tag<luid>(), std::forward<Args>(args)...));
    return ret;
  }

  // TODO: plumb this
//  proto::Location render_location(luid_t luid, std::string args) {
//    proto::Location ret;
//    ret.set_luid(luid);
//    ret.set_data(render_location_internal<luid>(std::forward<Args>(args)...));
//  }

  epv4 parse_location(luid_tag<common_luid::Udp_Ipv4>, data_const_ref);
  epv6 parse_location(luid_tag<common_luid::Udp_Ipv6>, data_const_ref);
  std::string render_location_internal(luid_tag<common_luid::Udp_Ipv4>, const epv4&);
  std::string render_location_internal(luid_tag<common_luid::Udp_Ipv6>, const epv6&);
  std::string render_location_internal(luid_tag<common_luid::Udp_Ipv4>, std::string_view);
  std::string render_location_internal(luid_tag<common_luid::Udp_Ipv6>, std::string_view);
}
