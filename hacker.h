#pragma once

#include <random>
#include <string>

class Hacker {
 public:
  explicit Hacker(std::mt19937 rng);

  std::string generateAlias();
  std::string craftProbe(const std::string &target);

 private:
  std::string randomHandle();
  std::string randomHexTag();

  std::mt19937 rng_;
};

bool runHacker(std::mt19937 &rng, const std::string &target);

class TechnoScanner {
 public:
  explicit TechnoScanner(std::mt19937 &rng);
  std::string randomTarget();
  void runFastScan(const std::string &target);

 private:
  std::mt19937 &rng_;
};
