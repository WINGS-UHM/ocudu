#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>

namespace ocudu {

struct isac_snapshot {
  static constexpr unsigned MAX_CONST = 512;
  static constexpr unsigned MAX_CSI   = 512;

  uint64_t timestamp_ns = 0;

  unsigned n_const = 0;
  unsigned n_csi   = 0;

  float const_i[MAX_CONST]{};
  float const_q[MAX_CONST]{};

  float csi_mag[MAX_CSI]{};
  float csi_phase[MAX_CSI]{};
};

class isac_sink
{
public:
  // Called from PHY hot path.
  inline void write(const float* ci, const float* cq, unsigned n_const_,
                    const float* cmag, const float* cph, unsigned n_csi_,
                    uint64_t tstamp_ns)
  {
    // Write into non-active buffer, then flip atomically.
    const unsigned wi = (write_idx.load(std::memory_order_relaxed) ^ 1U);

    isac_snapshot& b = buffers[wi];
    b.timestamp_ns = tstamp_ns;

    b.n_const = (n_const_ > isac_snapshot::MAX_CONST) ? isac_snapshot::MAX_CONST : n_const_;
    b.n_csi   = (n_csi_   > isac_snapshot::MAX_CSI)   ? isac_snapshot::MAX_CSI   : n_csi_;

    if (b.n_const) {
      std::memcpy(b.const_i, ci, b.n_const * sizeof(float));
      std::memcpy(b.const_q, cq, b.n_const * sizeof(float));
    }
    if (b.n_csi) {
      std::memcpy(b.csi_mag,   cmag, b.n_csi * sizeof(float));
      std::memcpy(b.csi_phase, cph,  b.n_csi * sizeof(float));
    }

    // Publish.
    seq.fetch_add(1, std::memory_order_release);
    write_idx.store(wi, std::memory_order_release);
  }

  // Called from publisher thread.
  inline bool read_latest(isac_snapshot& out)
  {
    const uint64_t s0 = seq.load(std::memory_order_acquire);
    if (s0 == 0) {
      return false;
    }

    const unsigned ri = write_idx.load(std::memory_order_acquire);
    out = buffers[ri];

    const uint64_t s1 = seq.load(std::memory_order_acquire);
    // If writer updated during copy, try again next period.
    return (s0 == s1);
  }

private:
  isac_snapshot buffers[2];
  std::atomic<unsigned> write_idx{0};
  std::atomic<uint64_t> seq{0};
};

isac_sink& get_isac_sink();

} // namespace ocudu