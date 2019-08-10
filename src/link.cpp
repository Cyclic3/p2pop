#include "link.hxx"

namespace p2pop::net {
  namespace detail {
    static std::map<luid_t, const Listener*>& get_controller_maps() {
      static std::map<luid_t, const Listener*> isocpp_plz_fix_static_init_order_thx;
      return isocpp_plz_fix_static_init_order_thx;
    }
  }

  void Listener::register_listener(luid_t luid, const Listener * listener) {
    detail::get_controller_maps()[luid] = listener;
  }

  std::unique_ptr<Controller> Listener::listen(luid_t luid,
                                               data_const_ref location_dat,
                                               nuid_t nuid,
                                               data_const_ref nuid_privkey) {
    return
        detail::get_controller_maps()
        .at(luid)
        ->our_listen(location_dat, nuid, nuid_privkey);
  }
}
