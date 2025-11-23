#include "installer.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <sys/select.h>
#include <unistd.h>

namespace ansi {
constexpr const char *reset = "\033[0m";
constexpr const char *green = "\033[32m";
constexpr const char *yellow = "\033[33m";
constexpr const char *blue = "\033[34m";
constexpr const char *magenta = "\033[35m";
constexpr const char *cyan = "\033[36m";
}  // namespace ansi

namespace {

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

}  // namespace

void drawProgress(const std::string &label, int percent);

class ProgressBar {
 public:
  ProgressBar(int minStep, int maxStep, int delayMs)
      : minStep_(std::max(1, minStep)),
        maxStep_(std::max(minStep_, maxStep)),
        delayMs_(std::max(5, delayMs)) {}

  bool run(const std::string &label, std::mt19937 &rng) const {
    std::uniform_int_distribution<int> step(minStep_, maxStep_);
    int percent = 0;
    while (percent < 100) {
      percent = std::min(100, percent + step(rng));
      drawProgress(label, percent);
      if (escPressed()) return false;
      std::this_thread::sleep_for(std::chrono::milliseconds(delayMs_));
    }
    std::cout << "\n";
    return true;
  }

 private:
  int minStep_;
  int maxStep_;
  int delayMs_;
};

bool promptRootLogin(const std::string &user) {
  std::cout << ansi::blue << "[sudo] password for " << user << ": "
            << ansi::reset << std::flush;
  std::string captured;
  while (true) {
    char c;
    const ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n <= 0) continue;
    if (c == 27) {
      std::cout << "\n" << ansi::yellow << "Aborted." << ansi::reset << "\n";
      return false;
    }
    if (c == '\n' || c == '\r') {
      std::cout << "\n\n";
      break;
    }
    captured.push_back(c);
    std::cout << "*" << std::flush;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(400));
  std::cout << ansi::green << "Accepted. Welcome, " << user << "."
            << ansi::reset << "\n\n";
  return true;
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

bool noiseBurst(const std::string &pkg, std::mt19937 &rng) {
  std::uniform_int_distribution<int> lines(12, 34);
  std::uniform_int_distribution<int> wordLen(3, 7);
  std::uniform_int_distribution<int> hexVal(0, 0xFFFFF);
  std::uniform_int_distribution<int> delay(12, 35);
  std::uniform_int_distribution<int> codeDist(100, 999);
  const int count = lines(rng);
  for (int i = 0; i < count; ++i) {
    if (escPressed()) return false;
    const int len = wordLen(rng);
    std::string token;
    for (int j = 0; j < len; ++j) {
      token.push_back(static_cast<char>('a' + (rng() % 26)));
    }
    std::cout << "[DBG] " << pkg << " @" << std::hex << hexVal(rng) << std::dec
              << " | opcode:" << codeDist(rng) << " | tag:" << token
              << " | entropy:" << std::fixed << std::setprecision(2)
              << (rng() % 100) / 10.0 << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(delay(rng)));
  }
  return true;
}

bool dotInstall(const std::string &item, const std::string &path,
                std::mt19937 &rng) {
  std::uniform_real_distribution<double> sizeDist(4.0, 120.0);
  std::uniform_int_distribution<int> dotCount(6, 14);
  const double sizeMb = sizeDist(rng);
  std::cout << "Installing " << item << " (" << std::fixed
            << std::setprecision(1) << sizeMb << " MB) -> " << path << " ... "
            << std::flush;
  const int dots = dotCount(rng);
  for (int i = 0; i < dots; ++i) {
    if (escPressed()) return false;
    std::cout << "." << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
  }
  std::cout << " done\n";
  return true;
}

void printIntro() {
  std::cout << ansi::cyan << "===> fake-install v0.1\n" << ansi::reset;
  std::cout << ansi::yellow << "Press ESC to abort at any time.\n\n"
            << ansi::reset;
}

std::vector<std::string> loadInstallNames(const std::string &path) {
  std::ifstream file(path);
  std::vector<std::string> names;
  if (!file.is_open()) return names;
  std::string line;
  while (std::getline(file, line)) {
    if (!line.empty()) names.push_back(line);
  }
  return names;
}

std::vector<std::string> collectRealPaths(const std::string &user) {
  std::vector<std::string> paths;
  auto tryAdd = [&paths](const std::filesystem::path &p) {
    if (!std::filesystem::exists(p)) return;
    for (const auto &entry : std::filesystem::directory_iterator(
             p, std::filesystem::directory_options::skip_permission_denied)) {
      if (entry.is_directory()) {
        paths.push_back(entry.path().string());
      }
    }
  };
  tryAdd(std::filesystem::path("/home") / user);
  tryAdd("/usr");
  tryAdd("/opt");
  tryAdd("/var");
  return paths;
}

std::string fabricatePath(const std::string &user,
                          const std::vector<std::string> &tokens,
                          std::mt19937 &rng) {
  const std::vector<std::string> bases = {"/home", "/opt", "/var", "/usr/local",
                                          "/srv", "/mnt"};
  std::uniform_int_distribution<int> baseDist(
      0, static_cast<int>(bases.size()) - 1);
  std::uniform_int_distribution<int> tokenDist(
      0, static_cast<int>(tokens.size()) - 1);
  const std::string base = bases[static_cast<size_t>(baseDist(rng))];
  const std::string t1 = tokens[static_cast<size_t>(tokenDist(rng))];
  const std::string t2 = tokens[static_cast<size_t>(tokenDist(rng))];
  if (base == "/home") {
    return base + "/" + user + "/" + t1 + "/" + t2;
  }
  return base + "/" + t1 + "/" + t2;
}

std::string pickInstallPath(const std::vector<std::string> &realPaths,
                            const std::string &user,
                            const std::vector<std::string> &tokens,
                            std::mt19937 &rng) {
  std::uniform_int_distribution<int> chance(0, 99);
  if (!realPaths.empty() && chance(rng) < 35) {
    std::uniform_int_distribution<int> idx(
        0, static_cast<int>(realPaths.size()) - 1);
    return realPaths[static_cast<size_t>(idx(rng))];
  }
  return fabricatePath(user, tokens, rng);
}

class NameBuilder {
 public:
  NameBuilder(const std::vector<std::string> &tokens, std::mt19937 &rng)
      : tokens_(tokens), rng_(rng) {}

  std::string randomToken() {
    if (tokens_.empty()) return "pkg";
    std::uniform_int_distribution<int> idx(
        0, static_cast<int>(tokens_.size()) - 1);
    return tokens_[static_cast<size_t>(idx(rng_))];
  }

  std::string randomVersion() {
    std::uniform_int_distribution<int> v(0, 19);
    return std::to_string(v(rng_)) + "." + std::to_string(v(rng_)) + "." +
           std::to_string(v(rng_));
  }

  std::string build() {
    std::uniform_int_distribution<int> pattern(0, 4);
    switch (pattern(rng_)) {
      case 0:
        return randomToken() + "-" + randomToken();
      case 1:
        return randomToken() + randomToken() + "-" + randomVersion();
      case 2:
        return randomToken() + ":" + randomToken() + "@" + randomVersion();
      case 3:
        return randomToken() + "." + randomToken() + "." + randomVersion();
      case 4:
      default:
        return randomToken() + "-" + randomToken() + "-" + randomToken();
    }
  }

 private:
  const std::vector<std::string> &tokens_;
  std::mt19937 &rng_;
};

struct DependencyList {
  std::vector<std::string> names;
  std::vector<double> sizesMb;
};

bool promptDependencies(const std::string &pkg, std::mt19937 &rng,
                        DependencyList &out) {
  static const std::vector<std::string> deps = {
      "libcrypto",   "libuv",      "libhttp3",   "libvector",
      "libneon",     "libsol",     "libzstd",    "libinput",
      "libfreetype", "libcairo",   "libpulse",   "libsystemd",
      "libfirefly",  "libmesh",    "libqt6core", "libqt6gui",
      "libxrender",  "libx264",    "libwayland", "libssl",
      "libcurl",     "libarch",    "libpipeline"};

  std::uniform_int_distribution<int> countDist(4, 9);
  std::uniform_int_distribution<int> ver(0, 9);
  std::uniform_real_distribution<double> sizeMb(0.5, 45.0);
  const int count = countDist(rng);

  out.names.clear();
  out.sizesMb.clear();

  std::cout << ansi::yellow << "[deps] Additional requirements for " << pkg
            << ":" << ansi::reset << "\n";
  for (int i = 0; i < count; ++i) {
    const std::string &name = deps[static_cast<size_t>(rng() % deps.size())];
    const double sz = sizeMb(rng);
    out.names.push_back(name + " >= " + std::to_string(ver(rng)) + "." +
                        std::to_string(ver(rng)) + "." +
                        std::to_string(ver(rng)));
    out.sizesMb.push_back(sz);
    std::cout << "  - " << out.names.back() << " (" << std::fixed
              << std::setprecision(1) << sz << " MB)\n";
  }

  std::cout << ansi::blue << "Continue installing " << pkg << "? [y/N]: "
            << ansi::reset << std::flush;

  std::string response;
  while (true) {
    char c;
    const ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n <= 0) continue;
    if (c == 27) {
      std::cout << "\n";
      return false;
    }
    if (c == '\n' || c == '\r') {
      std::cout << "\n";
      break;
    }
    response.push_back(c);
    std::cout << c << std::flush;
  }

  if (!response.empty() && (response[0] == 'y' || response[0] == 'Y')) {
    std::cout << ansi::green << "Proceeding." << ansi::reset << "\n";
    return true;
  }
  std::cout << ansi::yellow << "Cancelled by user." << ansi::reset << "\n";
  return false;
}

bool installDependencies(const DependencyList &deps, std::mt19937 &rng,
                         const std::string &user,
                         const std::vector<std::string> &tokens) {
  std::uniform_int_distribution<int> stepMin(3, 5);
  std::uniform_int_distribution<int> stepMax(7, 11);
  std::uniform_int_distribution<int> delay(50, 110);
  for (size_t i = 0; i < deps.names.size(); ++i) {
    const std::string path = fabricatePath(user, tokens, rng);
    const std::string label = "Installing " + deps.names[i];
    ProgressBar bar(stepMin(rng), stepMax(rng), delay(rng));
    if (!bar.run(label, rng)) return false;
    if (!dotInstall(deps.names[i], path, rng)) return false;
  }
  return true;
}

bool runFakeInstaller(std::mt19937 &rng, const std::string &user) {
  std::vector<std::string> items = loadInstallNames("install_names.txt");
  if (items.empty()) {
    items = {"libasset",      "patch",     "bugfix",  "resync",   "texture",
             "fontpack",      "locale",    "driver",  "gamedata", "ui-skin",
             "microcode",     "capsule",   "nanomod", "overdrive","delta-sync",
             "quasar",        "plasma",    "tracer",  "beacon",   "sentinel",
             "flux",          "atlas",     "nebula",  "cascade",  "eclipse",
             "solstice",      "glimmer",   "spectre", "ember",    "frost",
             "tidal",         "obsidian",  "onyx",    "opal",     "citrine",
             "zircon",        "aurora",    "quanta",  "warden"};
  }

  NameBuilder nameGen(items, rng);
  std::unordered_set<std::string> seen;
  std::vector<std::string> packages;
  while (packages.size() < 16) {
    const std::string next = nameGen.build();
    if (seen.insert(next).second) packages.push_back(next);
  }

  std::vector<std::string> realPaths = collectRealPaths(user);
  const std::vector<int> actions = {0, 0, 1, 1, 1, 2, 2, 2,
                                    3};  // more installs, fewer noise
  std::uniform_int_distribution<int> actionDist(
      0, static_cast<int>(actions.size()) - 1);
  std::uniform_int_distribution<int> depsChance(0, 99);
  std::uniform_int_distribution<int> progressDelay(55, 130);
  std::uniform_int_distribution<int> stepMin(3, 6);
  std::uniform_int_distribution<int> stepMax(8, 13);

  for (const auto &pkg : packages) {
    std::cout << ansi::magenta << "[*] Preparing " << pkg << ansi::reset
              << "\n";

    DependencyList depList;
    const bool hasDeps = depsChance(rng) < 25;
    if (hasDeps) {
      if (!promptDependencies(pkg, rng, depList)) {
        std::cout << ansi::yellow << "Aborted by user." << ansi::reset << "\n";
        return false;
      }
      if (!installDependencies(depList, rng, user, items)) {
        std::cout << ansi::yellow << "Aborted by user." << ansi::reset << "\n";
        return false;
      }
    }

    bool ok = true;
    switch (actions[static_cast<size_t>(actionDist(rng))]) {
      case 0:
        ok = ProgressBar(stepMin(rng), stepMax(rng), progressDelay(rng))
                 .run("Downloading " + pkg, rng);
        break;
      case 1: {
        std::uniform_int_distribution<int> cyclesDist(12, 42);
        ok = runSpinner("Installing " + pkg, cyclesDist(rng), rng);
        break;
      }
      case 2: {
        const std::string item = nameGen.build();
        const std::string path = pickInstallPath(realPaths, user, items, rng);
        ok = dotInstall(item, path, rng);
        break;
      }
      case 3:
        ok = noiseBurst(pkg, rng);
        break;
    }
    if (!ok) {
      std::cout << "\n" << ansi::yellow << "Aborted by user." << ansi::reset
                << "\n";
      return false;
    }

    std::cout << ansi::green << "[+] Installed " << pkg << ansi::reset << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(180));
  }

  std::cout << "\n" << ansi::blue << "All packages up to date." << ansi::reset
            << "\n";
  return true;
}
