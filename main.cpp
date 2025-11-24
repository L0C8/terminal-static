#include <cstdlib>
#include <iostream>
#include <random>
#include <string>

#include <termios.h>
#include <unistd.h>

#include "hacker.h"
#include "installer.h"

class TerminalMode {
 public:
  TerminalMode() {
    tcgetattr(STDIN_FILENO, &original_);
    termios raw = original_;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
  }

  ~TerminalMode() { tcsetattr(STDIN_FILENO, TCSANOW, &original_); }

 private:
  termios original_{};
};

void printUsage(const std::string &binary) {
  std::cout << "Usage: " << binary << " [-i | -h]\n"
            << "  -i   run fake installer\n"
            << "  -h   run hacker mode\n";
}

int main(int argc, char **argv) {
  TerminalMode mode_guard;
  std::mt19937 rng{std::random_device{}()};

  const char *userEnv = std::getenv("USER");
  const std::string user = userEnv ? userEnv : "root";

  if (argc < 2) {
    printUsage("terst");
    return 1;
  }

  const std::string mode = argv[1];
  if (mode == "-i" || mode == "--install") {
    printIntro();
    if (!promptRootLogin(user)) return 0;
    runFakeInstaller(rng, user);
    return 0;
  }

  if (mode == "-h" || mode == "--hacker") {
    runHacker(rng, user);
    return 0;
  }

  printUsage("terst");
  return 1;
}
