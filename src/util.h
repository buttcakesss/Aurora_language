#pragma once
#include <fstream>
#include <sstream>
#include <string>
inline std::string slurp(const std::string& path){
  std::ifstream f(path); std::stringstream ss; ss << f.rdbuf(); return ss.str();
}
