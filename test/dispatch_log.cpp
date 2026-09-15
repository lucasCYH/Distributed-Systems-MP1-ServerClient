# include <string>
# include <sys/socket.h>
# include <iostream>

int NUM_OF_MACHINE = 10;
using namespace std;


int main(int argc, char** argv) {
  if (argc > 2) {
    cout << "Usage: ./bins/dispatch_log [somehowFrequency]" << endl;
    return 1;
  }

  system("mkdir -p ./logs");
  // generate log file from local
  for (int i = 1; i <= NUM_OF_MACHINE; i++) {
    string path = "./bins/LogGenerator " + to_string(i);
    if (argc == 2) path = path + " " + argv[1];
    const char *command = path.c_str();
    int result = system(command);
    if (result == 0)
      cout << "file" << to_string(i) << " is generated." << endl;
  }

  system("./remote.sh push_logs");
  system("rm -r ./logs");
}