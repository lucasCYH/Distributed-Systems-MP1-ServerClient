#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "machine.hpp"
#include "metrics.hpp"

#define PORT 9080

const std::string DELIMITER = "\n__STREAM_COMPLETE__\n";

// line_num: >= 0 success, -1: fails.
struct Log_Query {
  int machine_id = 0;
  int line_num = 0;
  std::string content;
};

Log_Query receivedData(int socket_fd, int machine_id) {
  Log_Query result;
  result.machine_id = machine_id;
  char buffer[65536];
  ssize_t bytes_read;

  while ((bytes_read = recv(socket_fd, buffer, sizeof(buffer), 0)) > 0) {
    result.content.append(buffer, bytes_read);
    result.line_num += std::count(buffer, buffer + bytes_read, '\n');
  }
  int content_size = result.content.size();
  int delimiter_size = DELIMITER.size();

  if (bytes_read < 0 || (content_size >= delimiter_size &&
      result.content.compare(content_size - delimiter_size, delimiter_size,
                             DELIMITER) != 0)) {
    result.line_num = -1;
  } else {
    result.line_num = result.line_num - 2;
    result.content = "machine." + std::to_string(machine_id) + ".log" + "\nFind lines: " + std::to_string(result.line_num) + "\n" + result.content;
    result.content.resize(result.content.size() - delimiter_size);
  }

  return result;
}

void worker_task(int machine_id, const std::string &query_pattern,
                 std::vector<machine_config> &machine_cfg,
                 std::vector<Log_Query> &results) {

  // The kernel allocates a new socket data structure in kernel space and
  // returns you an fd SOCK_STREAM: allow streaming, other file properties.
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return;

  timeval timeout = timeval{2, 0};

  // recv timeout: returns error if not receiving data
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  // send timeout: returns error if cant send the data. ex. Buffer is full.
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

  sockaddr_in server_addr{};
  // This address can utilize UDP, TCP to transport data.
  server_addr.sin_family = AF_INET;
  // host to network short
  server_addr.sin_port = htons(std::stoi(machine_cfg[machine_id].port));
  // IP to binary format
  inet_pton(AF_INET, machine_cfg[machine_id].ip.c_str(), &server_addr.sin_addr);

  // connect
  // connect to remote server via the given fd
  // 1. bind to a socket if haven't.
  // 2. TCP handshake
  // success -> open to send/recv, fail -> return -1, errno
  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      0) {
    std::string request = query_pattern + "\n";
    send(sock, request.c_str(), request.size(), 0);
    // For now, the socket can only read data from the remote server
    shutdown(sock, SHUT_WR);
    // I/O run in parallel
    results[machine_id] = receivedData(sock, machine_id + 1);
    if (results[machine_id].line_num == -1) {
      results[machine_id].content = "machine." +
                                    std::to_string(machine_id + 1) + ".log\n" +
                                    "Connection lost mid-query\n";
    }
  } else {
    perror((machine_cfg[machine_id].ip + " failed\n").c_str());
    results[machine_id] =
        Log_Query{machine_id + 1, 0,
                  "machine." + std::to_string(machine_id + 1) + ".log\n" +
                      "connection failed\n"};
  }

  close(sock);
}

int main() {
  std::string machine_path = REMOTE_MACHINES_PATH;
  Metrics::init("client", -1);
  std::vector<machine_config> machine_cfgs =
      read_all_machine_config(REMOTE_MACHINES_PATH);
  printf("machine count: %zu\n", machine_cfgs.size());
  std::vector<Log_Query> results(machine_cfgs.size());

  std::string command;
  getline(std::cin, command);

  while(command != "end") {
    std::string tag = command;
    std::vector<std::thread> threads;

    for (char &c : tag)
      if (c == ',')
        c = ';'; // for CSV decoding

    // parse the prefix "metrics|" string
    Metrics::resolve_request(command, false);

    for (size_t i = 0; i < machine_cfgs.size(); i++) {
      threads.emplace_back(worker_task, i, command, std::ref(machine_cfgs),
                          std::ref(results));
    }

    {
      // RAII timer
      Metrics::Timer full_timer("full", "cmd=" + tag);
      for (auto &t : threads) {
        if (t.joinable())
          t.join();
      }
    }

    std::vector<int> summary;

    for (const auto &res : results) {
      std::cout << res.content;
      summary.push_back(res.line_num);
    }

    std::cout << "\nSummary:" << std::endl;
    int sum = 0;

    for (size_t i = 0 ; i < summary.size() ; i++) {
      std::cout << "machine." + std::to_string(i + 1) + ".log: " + std::to_string(std::max(summary[i], 0)) << std::endl;
      sum = sum + std::max(0, summary[i]);
    }

    std::cout << "Total: " + std::to_string(sum) << std::endl;
    getline(std::cin, command);
  }

  return 0;
}