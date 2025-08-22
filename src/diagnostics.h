#pragma once
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>

[[noreturn]] inline void fatal(const std::string& msg) {
  std::fprintf(stderr, "error: %s\n", msg.c_str());
  std::exit(1);
}
inline void warn(const std::string& msg) {
  std::fprintf(stderr, "warning: %s\n", msg.c_str());
}
