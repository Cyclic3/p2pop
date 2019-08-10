#pragma once

#include "p2pop/base.hpp"

namespace p2pop::crypto {
  struct Verifier {
    /// Verifies a ber signature
    bool verify(data_const_ref ber_signature) const;
    /// DER pubkey
    std::string pubkey() const;

    virtual ~Verifier() = default;

    static std::unique_ptr<Verifier> load(data_const_ref ber);
  };

  struct Signer : public Verifier {
    /// DER signature
    std::string sign(data_const_ref) const;
    /// DER privkey
    std::string privkey() const;

    virtual ~Signer() = default;

    static std::unique_ptr<Signer> load(data_const_ref ber);
  };
}
