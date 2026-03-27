#pragma once

#include "apps/services/remote_control/remote_server_metrics_gateway.h"
#include <atomic>
#include <chrono>
#include <thread>

namespace ocudu {
namespace app_services {

class isac_metrics_publisher
{
public:
  isac_metrics_publisher(remote_server_metrics_gateway& gw, std::chrono::milliseconds p) :
    gateway(gw), period(p) {}

  ~isac_metrics_publisher() { stop(); }

  void start();
  void stop();

private:
  void run();

  remote_server_metrics_gateway& gateway;
  std::chrono::milliseconds      period;

  std::thread       th;
  std::atomic<bool> running{false};
};

} // namespace app_services
} // namespace ocudu