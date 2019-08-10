#include "p2pop/status.hpp"

namespace p2pop {
  static const status OK_v;
  static const status CANCELLED_v(status_code::CANCELLED, "Cancelled");

  /// An OK pre-defined instance.
  const status& status::OK = OK_v;
  /// A CANCELLED pre-defined instance.
  const status& status::CANCELLED = CANCELLED_v;
}
