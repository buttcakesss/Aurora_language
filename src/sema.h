// sema.h
#pragma once
#include "ast.h"
#include "scope.h"

struct FnSig { std::vector<std::unique_ptr<Type>> params; std::unique_ptr<Type> ret; };

struct Sema {
  Scope scope;
  std::unordered_map<std::string, FnSig> fns;
  int loopDepth = 0;  // Track loop nesting for break/continue validation
  void primaries(); // install builtins
  void analyze(Program& p);
private:
  std::unique_ptr<Type> infer(Expr& e);
  void checkStmt(Stmt& s, const Type* currentRet, std::vector<Expr*>& defers);
};
