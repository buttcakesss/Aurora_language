#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include "types.h"

struct VarInfo { std::unique_ptr<Type> ty; bool isUnique=false; };

struct Scope {
  std::vector<std::unordered_map<std::string,VarInfo>> stack;
  Scope(){ push(); }
  void push(){ stack.emplace_back(); }
  void pop(){ stack.pop_back(); }
  bool declare(const std::string& n, std::unique_ptr<Type> t, bool isUnique=false){
    auto &m = stack.back();
    if (m.count(n)) return false;
    m.emplace(n, VarInfo{std::move(t),isUnique});
    return true;
  }
  const VarInfo* lookup(const std::string& n) const {
    for (int i=(int)stack.size()-1;i>=0;--i){ auto it=stack[i].find(n); if (it!=stack[i].end()) return &it->second; }
    return nullptr;
  }
};
