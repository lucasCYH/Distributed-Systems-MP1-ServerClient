#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

using namespace std;

void random_log_generator(string *logLevels, string *modules,
                          string *httpAction, string *apiPath,
                          string *stateCode, string *IPadress,
                          string *randomLogFile) {
  for (int i = 0; i < 4000; i++) {
    randomLogFile[i] =
        "[" + logLevels[rand() % 6] + "] [" + modules[rand() % 15] + "] " +
        httpAction[rand() % 6] + " " + apiPath[rand() % 6] + " " +
        stateCode[rand() % 11] + " " + IPadress[rand() % 4] + " - ";
  }
}

string timeToString(time_t timeValue) {
  tm *timeInfo = localtime(&timeValue);

  ostringstream oss;
  oss << put_time(timeInfo, "%Y-%m-%d %H:%M:%S");

  return oss.str();
}

int main(int argc, char** argv) { // machine_id, unittest for which percent
  if (argc < 2 || argc > 3) {
    cerr << "Usage: LogGenerator [machine_id] [somewhatFrequentValue (optional)]" << endl;
    return 1;
  }

  srand(time(NULL));
  string logLevels[6] = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "TRACE"};
  string modules[15] = {
      "HTTP-Server",   "APIGateway",       "ReverseProxy",
      "AuthService",   "PaymentProcessor", "UserAccountManager",
      "DBPool",        "CacheStore",       "SessionManager",
      "WorkerThread",  "TaskQueue",        "DataIngestionPipeline",
      "SystemMonitor", "HealthCheck",      "MemoryManager"};
  string httpAction[6] = {"GET", "POST", "PUT", "DELETE", "PATCH", "HEAD"};
  string apiPath[6] = {"/api/v1/auth/login",  "/api/v1/users/profile",
                       "/static/bundle.js",   "/index.html",
                       "/api/v2/stream/sync", "/health"};
  string stateCode[11] = {"200", "201", "204", "304", "400", "401",
                          "403", "404", "500", "502", "503"};
  string IPadress[4] = {"192.168.1.123", "10.0.4.147", "172.16.10.34",
                        "127.0.0.1"};
  string messages[19] = {
      "Incoming request payload validated successfully",
      "Static asset served from edge cache",
      "Client connection established via HTTP/1.1 protocol",
      "Request processing completed with status code",
      "Worker assigned background job from priority queue",
      "Cache lookup hit for resource key",
      "Batch transaction committed to storage buffer",
      "Scheduled maintenance worker routine executed",
      "Service health check responded normal status",
      "Resource utilization steady within defined thresholds",
      "Periodic telemetry heartbeat emitted",
      "task_id=7f3b89a1-04cd-4e89-9a2c-e58f0012bc44",
      "job_uuid=c9f1a238-d102-4b71-bc01-9a7c816e0012",
      "batch_id=0x92cf1a",
      "key_hash=4a20fe981bc09312",
      "payload_checksum=8a9f3b21",
      "latency=14ms",
      "mem_free=4192MB",
      "active_conn=14/50"};

  string PATTERN_RARE = "FATAL_CORE_DUMP_CORRUPT_BUFFER_9999";
  string PATTERN_REGEX_TARGET =
      "USER_SESSION_ERR_AUTH_CODE_"; // + to_string(rand() % 9000 + 1000) + "\n"
  string PATTERN_SOMEWHAT_FREQUENT = "DATABASE_TRANSACTION_TIMEOUT_WARN";
  string PATTERN_FREQUENT = "HTTP_REQUEST_GET_INDEX_SUCCESS_200";
  string randomLogFile[4000];
  random_log_generator(logLevels, modules, httpAction, apiPath, stateCode,
                       IPadress, randomLogFile);
  int numberOfLine = 500000;

  /*
  rare pattern: 1 line on one machine
  somewhat frequent pattern: 50-100 lines on 2-3 machine
  frequent pattern: above thousand lines on  all machine
  */

  int RareLine = 100014;
  int frequentRegex = 2000; // 0.05% (300 lines)
  int offsetRegex = 83;
  int offsetSomewhatFrequent = 52;
  int SomewhatFrequent = 100; // TODO: adjustable (default: 6000 lines)
  if (argc == 3) SomewhatFrequent = stoi(argv[2]);
  int frequent = 5; // 20% (120000 lines)
  int offsetFrequent = 1;
  time_t curTime = 1000000000;
  string now = timeToString(curTime);
  string line;
  string filename = "./logs/machine." + string(argv[1]) + ".log";
  ofstream MyFile(filename);
  if (!MyFile.is_open())
    cout << "file open is failed" << endl;
  else {
    cout << filename << " is opened" << endl;
  }

  for (int i = 0; i < numberOfLine; i++) {
    if (i == RareLine && string(argv[1]) == "7") {
      line = "[" + now + "] " + randomLogFile[i % 4000] + PATTERN_RARE + "\n";
    } else if (i % SomewhatFrequent == offsetSomewhatFrequent &&
               (string(argv[1]) == "3" || string(argv[1]) == "8" ||
                string(argv[1]) == "9")) {
      line = "[" + now + "] " + randomLogFile[i % 4000] +
             PATTERN_SOMEWHAT_FREQUENT + "\n";
    } else if (i % frequentRegex == offsetRegex) {
      line = "[" + now + "] " + randomLogFile[i % 4000] + PATTERN_REGEX_TARGET +
             to_string(rand() % 9000 + 1000) + "\n";
    } else if (i % frequent == offsetFrequent) {
      line =
          "[" + now + "] " + randomLogFile[i % 4000] + PATTERN_FREQUENT + "\n";
    } else {
      line =
          "[" + now + "] " + randomLogFile[i % 4000] + messages[i % 19] + "\n";
    }

    MyFile << line;

    if (i % 30 == 0) {
      curTime++;
      now = timeToString(curTime);
    }
  }

  MyFile.close();
}
