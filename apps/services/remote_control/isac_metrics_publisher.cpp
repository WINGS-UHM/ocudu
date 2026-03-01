#include "isac_metrics_publisher.h"
#include "ocudu/isac/isac_sink.h"

#include "external/nlohmann/json.hpp"
#include <thread>

using namespace ocudu;
using namespace app_services;

void isac_metrics_publisher::start()
{
  if (running.exchange(true)) {
    return;
  }
  th = std::thread(&isac_metrics_publisher::run, this);
}

void isac_metrics_publisher::stop()
{
  if (!running.exchange(false)) {
    return;
  }
  if (th.joinable()) {
    th.join();
  }
}

void isac_metrics_publisher::run()
{
  while (running.load(std::memory_order_relaxed)) {
    std::this_thread::sleep_for(period);

    ocudu::isac_snapshot snap;
    if (!ocudu::get_isac_sink().read_latest(snap)) {
      continue;
    }

    nlohmann::json j;
    j["type"] = "isac";
    j["timestamp_ns"] = snap.timestamp_ns;

    // Keep arrays modest; Python can plot directly.
    j["constellation"]["i"] = std::vector<float>(snap.const_i, snap.const_i + snap.n_const);
    j["constellation"]["q"] = std::vector<float>(snap.const_q, snap.const_q + snap.n_const);

    j["csi"]["mag"]   = std::vector<float>(snap.csi_mag,   snap.csi_mag   + snap.n_csi);
    j["csi"]["phase"] = std::vector<float>(snap.csi_phase, snap.csi_phase + snap.n_csi);

    gateway.send(j.dump());
  }
}