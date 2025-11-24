#include "hacker.h"

#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

namespace {

const std::array<const char *, 16> handles = {
    "voidlink",  "ghost",    "payload",  "cipher",
    "stacktrace","buffer",   "nullbyte", "overflow",
    "phantom",   "zero",     "specter",  "xfer",
    "drift",     "glitch",   "kernel",   "siphon"};

}  // namespace

Hacker::Hacker(std::mt19937 rng) : rng_(rng) {}

std::string Hacker::randomHandle() {
  std::uniform_int_distribution<int> idx(0, static_cast<int>(handles.size()) - 1);
  return handles[static_cast<size_t>(idx(rng_))];
}

std::string Hacker::randomHexTag() {
  std::uniform_int_distribution<int> hex(0x1000, 0xFFFF);
  std::ostringstream out;
  out << std::hex << std::uppercase << hex(rng_);
  return out.str();
}

std::string Hacker::generateAlias() {
  std::ostringstream out;
  std::uniform_int_distribution<int> suffix(0, 99);
  out << randomHandle() << "-" << randomHexTag() << suffix(rng_);
  return out.str();
}

std::string Hacker::craftProbe(const std::string &target) {
  std::ostringstream out;
  out << "[" << randomHexTag() << "] ping " << target << " via "
      << generateAlias();
  return out.str();
}

TechnoScanner::TechnoScanner(std::mt19937 &rng) : rng_(rng) {}

std::string TechnoScanner::randomTarget() {
  std::uniform_int_distribution<int> octet(1, 254);
  std::ostringstream ip;
  ip << octet(rng_) << "." << octet(rng_) << "." << octet(rng_) << "."
     << octet(rng_);
  return ip.str();
}

void TechnoScanner::runFastScan(const std::string &target) {
  const std::string host = target.empty() ? randomTarget() : target;
  std::uniform_int_distribution<int> latency(2, 9);
  std::uniform_int_distribution<int> portNoise(0, 12);

  std::cout << "\n[nmap] Starting Nmap 7.94 ( https://nmap.org ) at "
            << "now" << std::endl;
  std::cout << "[nmap] Initiating SYN Stealth Scan against " << host
            << " (fast)" << std::endl;

  struct Port {
    int number;
    const char *service;
  };
  const std::vector<Port> ports = {{22, "ssh"},   {53, "domain"},
                                   {80, "http"},  {139, "netbios-ssn"},
                                   {443, "https"}, {445, "microsoft-ds"},
                                   {8080, "http-proxy"}};

  for (const auto &p : ports) {
    std::cout << "PORT    STATE  SERVICE  LATENCY " << p.number << "/tcp"
              << "  open   " << p.service << "  "
              << latency(rng_) + portNoise(rng_) << "ms" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(55));
  }

  std::cout << "[nmap] Host " << host << " appears up. Aggressive timing used."
            << std::endl;
  std::cout << "[nmap] Fast scan complete. 1 host up." << std::endl;
}

bool runHacker(std::mt19937 &rng, const std::string &target) {
  Hacker hacker(rng);
  TechnoScanner scanner(rng);
  std::uniform_int_distribution<int> pulseCount(6, 12);
  std::uniform_int_distribution<int> delay(70, 160);
  std::uniform_int_distribution<int> barWidth(18, 42);

  std::cout << "===> hacker mode" << std::endl;
  std::cout << "[handshake] alias: " << hacker.generateAlias() << std::endl;

  const int pulses = pulseCount(rng);
  for (int i = 0; i < pulses; ++i) {
    const int width = barWidth(rng);
    std::cout << hacker.craftProbe(target) << " | ";
    for (int j = 0; j < width; ++j) {
      std::cout << ((j % 2 == 0) ? "#" : "=");
    }
    std::cout << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(delay(rng)));
  }

  scanner.runFastScan(scanner.randomTarget());
  std::cout << "[link] channel stable. awaiting further ops..." << std::endl;
  return true;
}
