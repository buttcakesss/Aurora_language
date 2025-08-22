// types.cpp
#include "types.h"
std::string Type::str() const {
  switch(k){
    case TyKind::I32: return "i32";
    case TyKind::I64: return "i64";
    case TyKind::Bool: return "bool";
    case TyKind::Void: return "void";
    case TyKind::Ptr: return "ptr<"+ (elem? elem->str() : "?") +">";
    case TyKind::Array: return (elem? elem->str() : "?") + "[" + std::to_string(arraySize) + "]";
  }
  return "?";
}
bool Type::equals(const Type& o) const {
  if (k!=o.k) return false;
  if (k==TyKind::Ptr) return elem && o.elem && elem->equals(*o.elem);
  if (k==TyKind::Array) return arraySize==o.arraySize && elem && o.elem && elem->equals(*o.elem);
  return true;
}

std::unique_ptr<Type> Type::clone() const {
  auto t = std::make_unique<Type>(k);
  if (elem) t->elem = elem->clone();
  t->arraySize = arraySize;
  return t;
}