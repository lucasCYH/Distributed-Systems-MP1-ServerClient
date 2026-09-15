#include "machine.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

vector<machine_config> read_all_machine_config(const string &path) {
  ifstream file(path);
  if (!file)
    throw runtime_error("cannot open machine config: " + path);

  string line, ip, port;
  vector<machine_config> machine_list;
  int id;

  while (getline(file, line)) {
    stringstream ss(line);
    if (!(ss >> id >> ip >> port))
      continue; // skip blank/malformed lines
    machine_config cfg;
    cfg.id = id;
    cfg.ip = ip;
    cfg.port = port;
    machine_list.emplace_back(cfg);
  }
  return machine_list;
}

machine_config read_machine_config(int id, const string &path) {
  vector<machine_config> configs = read_all_machine_config(path);
  for (const machine_config &cfg : configs) {
    if (cfg.id == id)
      return cfg;
  }
  throw runtime_error("no machine with id " + to_string(id) + " in " + path);
}