// types.h
#pragma once
#include <memory>
#include <string>
#include <vector>

enum class TyKind { I32, I64, Bool, Ptr, Array, Void };

struct Type {
  TyKind k;
  std::unique_ptr<Type> elem; // for Ptr<T> and Array<T>
  int64_t arraySize = 0; // for Array types
  explicit Type(TyKind k):k(k){}
  static std::unique_ptr<Type> i32(){ return std::make_unique<Type>(TyKind::I32); }
  static std::unique_ptr<Type> i64(){ return std::make_unique<Type>(TyKind::I64); }
  static std::unique_ptr<Type> boolean(){ return std::make_unique<Type>(TyKind::Bool); }
  static std::unique_ptr<Type> voidty(){ return std::make_unique<Type>(TyKind::Void); }
  static std::unique_ptr<Type> ptr(std::unique_ptr<Type> t){ auto p=std::make_unique<Type>(TyKind::Ptr); p->elem=std::move(t); return p; }
  static std::unique_ptr<Type> array(std::unique_ptr<Type> t, int64_t size){ auto a=std::make_unique<Type>(TyKind::Array); a->elem=std::move(t); a->arraySize=size; return a; }
  std::string str() const;
  bool equals(const Type& o) const;
  std::unique_ptr<Type> clone() const;  // Deep copy method
};