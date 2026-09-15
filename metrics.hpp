#pragma once
#include <chrono>
#include <cstdint>
#include <string>

namespace Metrics {

void init(const char *role, int id);

void set_enabled(bool on);

void record(const char *name, int64_t value_us, const std::string &extra = "");

void resolve_request(std::string &req, bool drop);

// RAII timer that auto records the value when it is dropped.
struct Timer {
  Timer(const char *n, std::string e = "")
      : name(n), extra(std::move(e)), t0(std::chrono::steady_clock::now()) {}
  ~Timer() {
    record(name,
           std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::steady_clock::now() - t0)
               .count(),
           extra);
  }
  const char *name;
  std::string extra;
  std::chrono::steady_clock::time_point t0;
};

} // namespace Metrics
