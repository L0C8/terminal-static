#pragma once

#include <random>
#include <string>

void printIntro();
bool promptRootLogin(const std::string &user);
bool runFakeInstaller(std::mt19937 &rng, const std::string &user);
