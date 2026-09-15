#include "metrics.hpp"

#include <sys/stat.h>

#include <cstdio>

static std::string g_path, g_tag;
static bool g_enabled = false;

void Metrics::init(const char *role, int id) {
  mkdir("metrics", 0755);
  g_tag = std::string(role) + "," + std::to_string(id);
  g_path = "metrics/" + std::string(role) + "." + std::to_string(id) +
           ".metrics.log";
  if (FILE *f = fopen(g_path.c_str(), "a"))
    fclose(f);
}

// If this is set to false any where in the program, then nothing would be
// recorded ever since.
void Metrics::set_enabled(bool on) { g_enabled = on; }

void Metrics::resolve_request(std::string &req, bool drop) {
  const std::string prefix = "metrics|";
  if (req.rfind(prefix, 0) == 0) {
    if (drop)
      req = req.substr(prefix.size());
    Metrics::set_enabled(true);
  }
}

// Record the metrics to metrics folder.
void Metrics::record(const char *name, int64_t value_us,
                     const std::string &extra) {
  if (!g_enabled || g_path.empty())
    return;
  FILE *f = fopen(g_path.c_str(), "a");
  if (!f)
    return;
  fprintf(f, "%lld,%s,%s,%lld,%s\n",
          (long long)std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count(),
          g_tag.c_str(), name, (long long)value_us, extra.c_str());
  fclose(f);
}
