#include "../link.hxx"

#include <boost/asio.hpp>
#include <tbb/concurrent_hash_map.h>

#include "node.pb.h"
#include "node.proto.p2pop.hpp"

#include <regex>

using boost::asio::ip::udp;

namespace p2pop::net::detail {
  constexpr age_t default_timeout = std::chrono::hours{3};

  udp::endpoint boost_endpoint(const epv4& ep) {
    return {boost::asio::ip::address_v4{ep.first}, ep.second};
  }
  udp::endpoint boost_endpoint(const epv6& ep) {
    return {boost::asio::ip::address_v6{ep.first}, ep.second};
  }

  class udp_c;

  class udp_d : public Dispatcher {
    friend class udp_c;

  private:
    udp_c* parent;
    peer_info inf;
    proto::Nuid nuid_serialised = serialise(inf.nuid);
    udp::endpoint dest;

  public:
    const peer_info& remote() const override { return inf; }
    proto::Packet dispatch(proto::Packet&& packet, age_t timeout) override;
    proto::Packet dispatch(proto::Packet&& packet) override {
      return dispatch(std::move(packet), default_timeout);
    }

  public:
    udp_d(udp_c* parent, peer_info inf);
    udp_d(udp_c* parent, proto::Location loc);
  };

  class udp_c : public Controller {
  private:
    class node_impl : public proto::Node::Service {
    private:
      udp_c* parent;

    public:
      ::p2pop::status ping(const ::p2pop::net::peer_info&,
                           const ::p2pop::proto::PingRequest&,
                           ::p2pop::proto::PingResponse&) override {
        return status::OK;
      }
      ::p2pop::status check_support(const ::p2pop::net::peer_info& /*inf*/,
                                   const ::p2pop::proto::CheckSupportRequest& req,
                                   ::p2pop::proto::CheckSupportResponse& res) override {
        res.set_supported(parent->check_support(req.puid()));
        return status::OK;
      }
      ::p2pop::status contact(const ::p2pop::net::peer_info& /*inf*/,
                              const ::p2pop::proto::ContactRequest& /*req*/,
                              ::p2pop::proto::ContactResponse& res) override {
        *res.mutable_me() = parent->me_serialised();
        return status::OK;
      }

    public:
      node_impl(udp_c* parent) : parent{parent} {}
    };

  private:
    boost::asio::io_context io_ctx;
    udp::socket sock;
    nuid_t nuid;
    proto::Nuid nuid_serialised = serialise(nuid);
    luid_t luid;
    tbb::concurrent_hash_map<puid_t, plugin::Service*> services;
    cid_generator gen;
    postbox p;

    boost::asio::thread_pool threads;

    std::atomic<bool> keep_pumping = true;
    std::thread pump;
    node_impl node_srv = {this};

  private:
    void pump_body() {
      std::array<uint8_t, max_buf_size> buf;
      udp::endpoint ep;
      auto b = boost::asio::buffer(buf.data(), buf.size());

      sock.async_receive_from(b, ep, [&](const boost::system::error_code& err, size_t len) {
        if (err)
          return;
        proto::Packet packet;
        if (!packet.ParseFromArray(buf.data(), len))
          return;

        auto dest = deserialise(packet.dest());
        if (dest != all_nuids && dest != nuid)
          return;

        switch (packet.type_case()) {
          case proto::Packet::kCall: {
            boost::asio::post(threads, [this, in{std::move(packet)}, ep{std::move(ep)}]() {
              decltype(services)::const_accessor handle;
              services.find(handle, in.puid());
              net::peer_info inf;
              inf.nuid = deserialise(in.source());
              auto addr = ep.address();
              if (addr.is_v4())
                inf.location = render_location<Udp_Ipv4>(epv4{addr.to_v4().to_bytes(), ep.port()});
              else
                inf.location = render_location<Udp_Ipv6>(epv6{addr.to_v6().to_bytes(), ep.port()});

              proto::Packet out;
              auto status = handle->second->run_method(in.mid(), inf, from_protobuf(in.params()), *out.mutable_params());
              // To allow fingerprinting, and becaause we checked it
              *out.mutable_source() = in.dest();
              *out.mutable_dest() = in.source();
              out.set_puid(in.puid());
              out.set_cid(in.cid());
              out.set_mid(in.mid());
              *out.mutable_return_()->mutable_status() = serialise(status);

              auto out_b = out.SerializeAsString();
              auto b = boost::asio::buffer(out_b.data(), out_b.size());

              sock.send_to(b, ep);
            });
          } break;
          case proto::Packet::kReturn: {
            p.post(std::move(packet));
          } break;
          default:
            // Sp00ked
            return;
        }
      });

      while (keep_pumping)
        io_ctx.run_one();
    }

  public:
    proto::Packet dispatch(proto::Packet&& packet, age_t timeout, const udp_d& remote) {
      packet.set_cid(gen());
      *packet.mutable_source() = nuid_serialised;
      *packet.mutable_dest() = remote.nuid_serialised;
      msg_desc desc = msg_desc::from_send(packet);

      auto r = p.reserve(desc);
      {
        auto out = packet.SerializeAsString();
        auto b = boost::asio::buffer(out.data(), out.size());
        sock.send_to(b, remote.dest);
      }
      return p.wait(std::move(r), timeout);
    }
    proto::Nuid me_serialised() const { return nuid_serialised; }
    bool check_support(puid_t puid) {
      return services.count(puid);
    }

  public:
    luid_t link() const noexcept override { return luid; }
    nuid_t me() const noexcept override { return nuid; }
    void attach(plugin::Service* srv) override {
      decltype(services)::accessor a;
      if (!services.insert(a, srv->get_puid()))
        throw std::runtime_error("Could not attach service");
      a->second = srv;
    }
    /// XXX: using the dispatcher after the deconstruction of the parent Controller
    /// will result in undefined behaviour
    std::unique_ptr<Dispatcher> contact(const proto::Location& loc) override {
      return std::make_unique<udp_d>(this, loc);
    }
    /// The default definition just forwards to contact. You probably don't want that
    std::unique_ptr<Dispatcher> dial(const peer_info& inf) override {
       return std::make_unique<udp_d>(this, inf);
    }

  public:
    udp_c(epv4 ep, nuid_t nuid) :
      sock{io_ctx, udp::socket::protocol_type::v4()},
      nuid{std::move(nuid)},
      luid{common_luid::Udp_Ipv4} {
      sock.bind(boost_endpoint(ep));
      attach(&node_srv);
      pump = std::thread{ &udp_c::pump_body, this };
    }

    udp_c(epv6 ep, nuid_t nuid) :
      sock{io_ctx, udp::socket::protocol_type::v6()},
      nuid{std::move(nuid)},
      luid{common_luid::Udp_Ipv6} {
      sock.bind(boost_endpoint(ep));
      attach(&node_srv);
      pump = std::thread{ &udp_c::pump_body, this };
    }

    ~udp_c() {
      keep_pumping = false;
      pump.join();
      threads.join();
    }
  };

  class udp4_l : public Listener {
  public:
    std::unique_ptr<Controller> our_listen(data_const_ref location_dat,
                                           nuid_t nuid,
                                           data_const_ref /*nuid_privkey*/) const override {
      return std::make_unique<udp_c>(parse_location(luid_tag<common_luid::Udp_Ipv4>(),
                                                    location_dat),
                                     std::move(nuid));
    }

  public:
    udp4_l() : Listener(common_luid::Udp_Ipv4) {}
  };

  class udp6_l : public Listener {
  public:
    std::unique_ptr<Controller> our_listen(data_const_ref location_dat,
                                           nuid_t nuid,
                                           data_const_ref /*nuid_privkey*/) const override {
      return std::make_unique<udp_c>(parse_location(luid_tag<common_luid::Udp_Ipv6>(),
                                                    location_dat),
                                     std::move(nuid));
    }

  public:
    udp6_l() : Listener(common_luid::Udp_Ipv6) {}
  };

  proto::Packet udp_d::dispatch(proto::Packet&& packet, age_t timeout) {
    return parent->dispatch(std::move(packet), timeout, *this);
  }

  udp_d::udp_d(udp_c* parent, peer_info inf_) : parent{parent}, inf{std::move(inf_)} {
    // Yes, I am a bad person.
    //
    // TODO: make static and plumb whole thing through
    if (parent->link() == common_luid::Udp_Ipv4)
      dest = boost_endpoint(parse_location(luid_tag<common_luid::Udp_Ipv4>(),
                                           from_protobuf(inf.location.data())));
    else
      dest = boost_endpoint(parse_location(luid_tag<common_luid::Udp_Ipv6>(),
                                           from_protobuf(inf.location.data())));
  }

  udp_d::udp_d(udp_c* parent, proto::Location loc_) : udp_d{parent, {all_nuids, std::move(loc_)}} {
    proto::Node n(this);
    proto::ContactRequest req;
    proto::ContactResponse res;
    *req.mutable_me() = parent->me_serialised();
    auto s = n.contact(req, res);
    if (!s.ok())
      throw std::runtime_error(s.error_message());
    inf.nuid = deserialise(res.me());
    // And just set up again
    new (this) udp_d{parent, std::move(inf)};
  }

  P2POP_NET_LINK_LISTENER_REG(udp4_l);
  P2POP_NET_LINK_LISTENER_REG(udp6_l);
}

namespace p2pop::net {
  epv4 parse_location(luid_tag<common_luid::Udp_Ipv4>, data_const_ref b) {
    if (b.size() != 4 + 2)
      throw std::invalid_argument("Bad epv4");

    epv4 ret;
    std::copy(b.begin(), b.begin() + std::tuple_size_v<ipv4_address>, ret.first.begin());
    ret.second = b[4];
    ret.second <<= 8;
    ret.second += b[5];

    return ret;
  }
  epv6 parse_location(luid_tag<common_luid::Udp_Ipv6>, data_const_ref b) {
    if (b.size() != std::tuple_size_v<ipv6_address> + 2)
      throw std::invalid_argument("Bad epv4");

    epv6 ret;
    std::copy(b.begin(), b.begin() + std::tuple_size_v<ipv6_address>, ret.first.begin());
    ret.second = b[16];
    ret.second <<= 8;
    ret.second += b[17];

    return ret;
  }
  std::string render_location_internal(luid_tag<common_luid::Udp_Ipv4>, const epv4& ep) {
    std::string ret;
    ret.resize(std::tuple_size_v<ipv4_address> + 2);
    std::copy(ep.first.begin(), ep.first.end(), ret.begin());
    ret[4] = ep.second >> 8;
    ret[5] = ep.second & 0xFF;
    return ret;
  }
  std::string render_location_internal(luid_tag<common_luid::Udp_Ipv6>, const epv6& ep) {
    std::string ret;
    ret.resize(std::tuple_size_v<ipv6_address> + 2);
    std::copy(ep.first.begin(), ep.first.end(), ret.begin());
    ret[16] = ep.second >> 8;
    ret[17] = ep.second & 0xFF;
    return ret;
  }
  std::string render_location_internal(luid_tag<common_luid::Udp_Ipv4>, std::string_view ep) {
    auto port_delim = ep.find(':');
    if (port_delim == std::string_view::npos)
      throw std::invalid_argument("No port");

    std::string_view host{ep.begin(), port_delim};
    // "1111:22" is size 7, pos 4, length 2 (size - pos - 1)
    std::string_view port{ep.begin() + port_delim + 1, ep.size() - port_delim - 1};

    boost::asio::io_context io_ctx;
    udp::resolver resolver(io_ctx);
    auto res = resolver.resolve(host, port);
    auto iter = res.begin();
    if (iter == res.end())
      throw std::runtime_error("Resolution of udp ep failed");
    auto ep_v = iter->endpoint();
    boost::asio::ip::address_v4 addr = ep_v.address().to_v4();
    return render_location_internal(luid_tag<common_luid::Udp_Ipv4>(),
                                    epv4{addr.to_bytes(), ep_v.port()});
  }
  std::string render_location_internal(luid_tag<common_luid::Udp_Ipv6>, std::string_view ep) {
    std::regex regex{ R"(^\[(.*)\]:(\d+)$)" };
    std::match_results<std::string_view::const_iterator> results;
    if (!std::regex_match(ep.cbegin(), ep.cend(), results, regex) || results.size() != 3)
      throw std::runtime_error("Invalid IPv6 ep");

    auto host = results[1].str();
    auto port = results[2].str();

//    // [::1]:0 = 7
//    if (ep.size() < 7)
//      throw std::invalid_argument("IPv6 ep too short");
//    auto port_delim = ep.find_last_of(':');
//    if (port_delim == std::string_view::npos)
//      throw std::invalid_argument("No port");
//    // [::1]:0 @ 5
//    if (port_delim < 5)
//      throw std::invalid_argument("IPv6 address invalid");
//    std::string_view host{ep.begin() + 1, port_delim - 2};
//    // "1111:22" is size 7, pos 4, length 2 (size - pos - 1)
//    std::string_view port{ep.begin() + port_delim + 1, ep.size() - port_delim - 1};

    boost::asio::io_context io_ctx;
    udp::resolver resolver(io_ctx);
    auto res = resolver.resolve(host, port);
    auto iter = res.begin();
    if (iter == res.end())
      throw std::runtime_error("Resolution of udp ep failed");
    auto ep_v = iter->endpoint();
    boost::asio::ip::address_v6 addr = ep_v.address().to_v6();
    return render_location_internal(luid_tag<common_luid::Udp_Ipv6>(),
                                    epv6{addr.to_bytes(), ep_v.port()});
  }
}
