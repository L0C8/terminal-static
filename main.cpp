#include <cstdlib>
#include <random>
#include <string>

#include <termios.h>
#include <unistd.h>

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

int main() {
  TerminalMode mode_guard;  
  std::mt19937 rng{std::random_device{}()};

  printIntro();
  const char *userEnv = std::getenv("USER");
  const std::string user = userEnv ? userEnv : "root";
  if (!promptRootLogin(user)) return 0;

  runFakeInstaller(rng, user);
  return 0;
}
