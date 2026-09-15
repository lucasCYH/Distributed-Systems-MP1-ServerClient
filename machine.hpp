#include <string>
#include <vector>

struct machine_config {
  int id;
  std::string ip;
  std::string port;
};
const std::string REMOTE_MACHINES_PATH = "config/machines-remote.txt";

std::vector<machine_config> read_all_machine_config(const std::string &path);

machine_config read_machine_config(int id, const std::string &path);