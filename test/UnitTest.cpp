#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace std;
int NUM_OF_MACHINE = 10;
string GROUND_TURTH [6] ={"Find lines: 0", "Find lines: 1", "Find lines: 250", "Find lines: 5000", "Find lines: 100000", "Find lines: 500000"};

void teardown() {
  system("./remote.sh stop");
}

bool validtest(string filename, int testnum) {
  int machine_id = 1;
  string machine_line = "machine." + to_string(machine_id) + ".log";
  string line_found = GROUND_TURTH[testnum + 1];
  string line_failed = "connection failed";
  string line_fail_in_mid = "Connection lost mid-query";
  string line_not_found = GROUND_TURTH[0];
  ifstream output(filename);
  string line;
  getline(output, line);
  if (line != "machine count: 10") return false;

  while(getline(output, line)) {
    if (line == machine_line) {
      getline(output, line);
      if (testnum == 0) {
        if (machine_id == 7 && line != line_found) return false;
        else if (machine_id != 7 && line != line_not_found) return false;
      }
      else if (testnum == 1) {
        if (line != line_found) return false;
      }
      else if (testnum == 2) {
        if ((machine_id == 3 || machine_id == 8 || machine_id == 9) && line != line_found) return false;
        else if (machine_id != 3 && machine_id != 8 && machine_id != 9 && line != line_not_found) return false;
      }
      else if (testnum == 3) {
        if (line != line_found) return false;
      }
      else if (testnum ==  4) {
        if (machine_id == 3 && line != line_failed) return false;
        else if ((machine_id == 8  || machine_id == 5) && line != line_fail_in_mid) return false;
        else if (machine_id != 3 && machine_id != 8 && machine_id != 5 && line != line_found) return false;
      }
      
      machine_id++;
      machine_line = "machine." + to_string(machine_id) + ".log";
    }
  }

  return true;
  
}

void envSetup() {
  system("./remote.sh stop");
  system("./remote.sh start");
  sleep(3);
}

void dispatch_logs() {
  system("mkdir -p ./logs");
  // generate log file from local
  for (int i = 1; i <= NUM_OF_MACHINE; i++) {
    string path = "./bins/LogGenerator " + to_string(i);
    const char *command = path.c_str();
    int result = system(command);
    if (result == 0)
      cout << "file" << to_string(i) << " is generated." << endl;
  }
  system("./remote.sh push_logs");
}

int main() {
  envSetup();
  

  string PATTERN[5] = {"FATAL_CORE_DUMP_CORRUPT_BUFFER_9999",
                       "USER_SESSION_ERR_AUTH_CODE_[0-9]{4}",
                       "DATABASE_TRANSACTION_TIMEOUT_WARN",
                       "HTTP_REQUEST_GET_INDEX_SUCCESS_200", ""};

  for (int num = 0; num < 5; num++) {
    string pass_command = "| ./bins/client > test" + to_string(num + 1) + ".txt";
    string command = "echo \"";
    if (num == 0 || num == 2 || num == 3)
      command = command + "grep " + PATTERN[num] + "\" " + pass_command;
    else if (num == 1)
      command = command + "grep -E " + PATTERN[num] + "\" " + pass_command;
    else if (num == 4) {
      system("./remote.sh stop 3");
      command = "(" + command + "grep 20 -slow\"" + pass_command + ") & sleep 0.005; ./remote.sh stop 8; wait";
    }

    system(command.c_str());
    

    bool is_passed = true;
    cout << "Evaluating result " << num + 1 << "..." << endl;

    if (num == 0) { // test for rare patterns
      if (! validtest("test1.txt", num)) {
        is_passed = false;
        break;
      }
    } else if (num == 1) { // test for regex grep
      if (! validtest("test2.txt", num)) {
        is_passed = false;
        break;
      }
    } else if (num == 2) { // test for somehow frequent patterns
      if (! validtest("test3.txt", num)) {
        is_passed = false;
        break;
      }
    } else if (num == 3) { // test for frequent patterns
        if (! validtest("test4.txt", num)) {
          is_passed = false;
          break;
        }
    } else if (num == 4) { // test for frequent patterns with fault-tolerance
        if (! validtest("test5.txt", num)) {
          is_passed = false;
          cout << "Test " << num + 1 << " is not passed." << endl;
          break;
        }
    }
    
    if (is_passed) {
      cout << "Test " << num + 1 << " is passed." << endl << endl;
    } else {
      cout << "Test " << num + 1 << " is not passed." << endl;
    }

    if (num == 4) {
      system("./remote.sh start 3");
      system("./remote.sh start 8");
    }
  }

  teardown();
}
