#pragma once

#include "p2pop/link.hpp"

#include <tbb/concurrent_hash_map.h>

#include <xxhash.h>

#include <map>
#include <functional>
#include <future>

namespace p2pop::net::detail {
  // And now, some useful things
  struct msg_desc {
    nuid_t nuid;
    cid_t cid;

    inline static msg_desc from_recv(const proto::Packet& packet) {
      return { deserialise(packet.source()), packet.cid()};
    }
    inline static msg_desc from_send(const proto::Packet& packet) {
      return { deserialise(packet.dest()), packet.cid()};
    }

    inline bool operator==(const msg_desc other) const {
      return nuid == other.nuid && cid == other.cid;
    }

    struct hasher {
      inline static size_t hash(const msg_desc& k) noexcept {
        if constexpr (sizeof(size_t) == sizeof(uint64_t)) {
          ::XXH64_state_t* state = ::XXH64_createState();
          ::XXH64_reset(state, 0);
          ::XXH64_update(state, k.nuid.data(), std::tuple_size_v<nuid_t>);
          ::XXH64_update(state, &k.cid, sizeof(k.cid));
          auto ret = ::XXH64_digest(state);
          ::XXH64_freeState(state);
          return static_cast<size_t>(ret);
        }
        else {
          ::XXH32_state_t* state = ::XXH32_createState();
          ::XXH32_reset(state, 0);
          ::XXH32_update(state, k.nuid.data(), std::tuple_size_v<nuid_t>);
          ::XXH32_update(state, &k.cid, sizeof(k.cid));
          auto ret = ::XXH32_digest(state);
          ::XXH32_freeState(state);
          return static_cast<size_t>(ret);
        }
      }

      inline static bool equal(const msg_desc& a, const msg_desc& b) { return a == b; }
    };
  };

  struct postbox {
  private:
    tbb::concurrent_hash_map<msg_desc, std::promise<proto::Packet>, msg_desc::hasher> box;

  public:
    class reservation {
      friend postbox;
    private:
      movable_ptr<postbox> parent;
      std::future<proto::Packet> future;
      msg_desc desc;

    private:
      inline reservation(postbox* p, decltype(future) f, decltype(desc) d) :
        parent{p}, future{std::move(f)}, desc{std::move(d)} {}
    public:
      inline ~reservation() { if (parent) parent->cancel(desc); }
    };

  public:
    inline reservation reserve(const msg_desc& desc) {
      decltype(box)::accessor handle;
      box.insert(handle, desc);
      return { this, handle->second.get_future(), desc};
    }
    inline proto::Packet wait(reservation&& r, age_t timeout) {
      if (r.future.wait_for(timeout) != std::future_status::ready)
        throw timed_out{};

      // The destructor of reservation will clean up after us
      return r.future.get();
    }
    inline void cancel(const msg_desc& desc) {
      box.erase(desc);
    }
    inline void cancel(reservation&& r) {
      cancel(r.desc);
    }

    inline bool post(proto::Packet&& packet) {
      decltype(box)::accessor handle;
      msg_desc desc = msg_desc::from_recv(packet);
      if (!box.find(handle, desc))
        return false;

      handle->second.set_value(std::move(packet));
      return true;
    }
  };
}


