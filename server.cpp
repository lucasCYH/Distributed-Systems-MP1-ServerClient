#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>

#include "machine.hpp"
#include "metrics.hpp"
#include "protocol.hpp"

#define PORT 9080
#define BACKLOG 20

bool MID_INTERRPUT_TEST = false;

// The users assign a single machine id to this program.
int main(int argc, char **argv) {
  int machine_id = -1;
  std::string machine_path = REMOTE_MACHINES_PATH;

  // Parse the input for assigned machine.
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-i") == 0) {
      if (i + 1 >= argc) {
        printf("-i requires a machine id\n");
        return 1;
      }
      machine_id = atoi(argv[++i]);
    } else {
      printf("Unknown argument: %s\n", argv[i]);
      return 1;
    }
  }
  if (machine_id < 0) {
    printf("Usage: %s -i <machine_id> [--remote]\n", argv[0]);
    return 1;
  }

  // Metrics setup
  Metrics::init("server", machine_id);

  // machine_config: returns the id, ip, port for each machine_id
  struct machine_config cfg = read_machine_config(machine_id, machine_path);

  int sockfd, target_fd, bytes;
  struct addrinfo hints;
  struct addrinfo *res;
  struct sockaddr_storage their_addr;
  socklen_t addr_size = sizeof(their_addr);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  // would be assigned to 0.0.0.0 this allows all connection sources. ex.
  // network card, devices.
  hints.ai_flags = AI_PASSIVE;
  // NULL + AI_PASSIVE => 0.0.0.0:port
  int rv = getaddrinfo(NULL, cfg.port.c_str(), &hints, &res);
  if (rv != 0) {
    printf("getaddrinfo fails: %d\n", rv);
    return 1;
  }

  // The kernel allocates a new socket data structure in kernel space and
  // returns you an fd
  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  timeval to{2, 0};
  setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &to, sizeof(to));

  if (sockfd == -1)
    printf("Socket creation fails: %d\n", errno);

  // The kernel takes that existing socket and writes a local (IP, port) into
  // its structure, then records in an internal table
  if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
    printf("Socket bind fails: %d\n", errno);
  }
  freeaddrinfo(res);

  // Open the listening process for kernel.
  // 1. Mark the socket as PASSIVE
  // 2. Creates a queue for letting new connections arrive. Waiting in the queue
  // for accept.
  listen(sockfd, BACKLOG);
  if (fcntl(sockfd, F_SETFD, FD_CLOEXEC) == -1)
    printf("fcntl FD_CLOEXEC fails: %d\n", errno);

  while (true) {
    // Returns an active / connected socket (target_fd).
    target_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
    char command[1024];
    char buffer[65536];
    int n = recv(target_fd, command, 1024, 0);
    // NOTICE: The ending character of the command is not '\0'
    if (n <= 0) {
      printf("error %d", errno);
      close(target_fd);
      continue;
    }

    std::string req(command, n);
    Metrics::resolve_request(req, true);

    {
      Metrics::Timer t(req.c_str());
      long long total = 0;
      MID_INTERRPUT_TEST = Protocol::is_delay(req);
      req += " ./logs/machine." + std::to_string(machine_id) + ".log";
      // execute the command directory, may cause safety issues but we ignore
      // that for now.
      FILE *result = popen(req.c_str(), "r");
      while ((bytes = fread(buffer, 1, sizeof(buffer), result)) > 0) {
        send(target_fd, buffer, bytes, 0);
        total += bytes;
      }
      pclose(result);
      if (MID_INTERRPUT_TEST) {
        if (machine_id == 8) sleep(1);
        if (machine_id == 5) sleep(5);
      };
      send(target_fd, "\n__STREAM_COMPLETE__\n", sizeof("\n__STREAM_COMPLETE__\n") - 1, 0);
      t.extra = "bytes=" + std::to_string(total);
    }
    printf("Machine %d complete.\n", machine_id);
    close(target_fd);
  }

  return 0;
}