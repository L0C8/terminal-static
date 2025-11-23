#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

namespace ansi {
constexpr const char *reset = "\033[0m";
constexpr const char *green = "\033[32m";
constexpr const char *yellow = "\033[33m";
constexpr const char *blue = "\033[34m";
constexpr const char *magenta = "\033[35m";
constexpr const char *cyan = "\033[36m";
}  

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

bool keyPressed() {
  fd_set set;
  FD_ZERO(&set);
  FD_SET(STDIN_FILENO, &set);
  timeval tv{0, 0};
  return select(STDIN_FILENO + 1, &set, nullptr, nullptr, &tv) > 0;
}

bool escPressed() {
  if (!keyPressed()) return false;
  char c;
  const ssize_t n = read(STDIN_FILENO, &c, 1);
  return n > 0 && c == 27;
}

void drawProgress(const std::string &label, int percent) {
  constexpr int barWidth = 32;
  const int filled = percent * barWidth / 100;
  std::cout << "\r" << label << " [";
  for (int i = 0; i < barWidth; ++i) {
    std::cout << (i < filled ? "#" : ".");
  }
  std::cout << "] " << percent << "%   " << std::flush;
}

bool runProgressBar(const std::string &label, std::mt19937 &rng) {
  std::uniform_int_distribution<int> step(4, 12);
  int percent = 0;
  while (percent < 100) {
    percent = std::min(100, percent + step(rng));
    drawProgress(label, percent);
    if (escPressed()) return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(90));
  }
  std::cout << "\n";
  return true;
}

bool runSpinner(const std::string &label, int cycles, std::mt19937 &rng) {
  const std::string frames = "-\\|/";
  std::uniform_int_distribution<int> delay(70, 130);
  for (int i = 0; i < cycles; ++i) {
    const char frame = frames[static_cast<size_t>(i) % frames.size()];
    std::cout << "\r" << label << " " << frame << "   " << std::flush;
    if (escPressed()) return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(delay(rng)));
  }
  std::cout << "\r" << label << " done   \n";
  return true;
}

void printIntro() {
  std::cout << ansi::cyan << "===> fake-install v0.1\n" << ansi::reset;
  std::cout << ansi::yellow << "Press ESC to abort at any time.\n\n"
            << ansi::reset;
}

bool runFakeInstaller(std::mt19937 &rng) {
  const std::vector<std::string> packages = {
      "build-essential", "libssl-dev", "curl", "neofetch", "lolcat",
      "cowsay", "fortune-mod", "htop", "tmux", "figlet"};

  for (const auto &pkg : packages) {
    std::cout << ansi::magenta << "[*] Preparing " << pkg << ansi::reset
              << "\n";

    if (!runProgressBar("Downloading " + pkg, rng)) {
      std::cout << "\n" << ansi::yellow << "Aborted by user." << ansi::reset
                << "\n";
      return 0;
    }

    if (!runSpinner("Installing " + pkg, 40, rng)) {
      std::cout << "\n" << ansi::yellow << "Aborted by user." << ansi::reset
                << "\n";
      return 0;
    }

    std::cout << ansi::green << "[+] Installed " << pkg << ansi::reset << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(180));
  }

  std::cout << "\n" << ansi::blue << "All packages up to date." << ansi::reset
            << "\n";
  return true;
}

int main() {
  TerminalMode mode_guard;  
  std::mt19937 rng{std::random_device{}()};

  printIntro();

  runFakeInstaller(rng);
  return 0;
}
