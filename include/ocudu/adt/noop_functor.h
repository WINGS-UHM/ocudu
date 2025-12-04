
#pragma once

namespace ocudu {

/// Do nothing operation functor.
struct noop_operation {
  template <typename T>
  void operator()(T&& t) const
  {
    // Do nothing.
  }
  void operator()() const
  {
    // Do nothing.
  }
};

} // namespace ocudu
