#pragma once
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include "token.h"

struct Type;

struct Expr {
  virtual ~Expr()=default;
};
using ExprPtr = std::unique_ptr<Expr>;

struct Stmt {
  virtual ~Stmt()=default;
};
using StmtPtr = std::unique_ptr<Stmt>;

struct EInt : Expr { std::int64_t v; explicit EInt(std::int64_t v):v(v){} };
struct EBool: Expr { bool v; explicit EBool(bool v):v(v){} };
struct EVar : Expr { std::string name; explicit EVar(std::string n):name(std::move(n)){} };
struct EUnary: Expr { TokKind op; ExprPtr rhs; EUnary(TokKind op, ExprPtr e):op(op),rhs(std::move(e)){} };
struct EBin  : Expr { TokKind op; ExprPtr lhs,rhs; EBin(ExprPtr a, TokKind op, ExprPtr b):op(op),lhs(std::move(a)),rhs(std::move(b)){} };
struct ECall : Expr { std::string callee; std::vector<ExprPtr> args; explicit ECall(std::string c):callee(std::move(c)){} };
struct EArrayLit : Expr { std::vector<ExprPtr> elems; explicit EArrayLit(std::vector<ExprPtr> e):elems(std::move(e)){} };
struct EIndex : Expr { ExprPtr arr; ExprPtr idx; EIndex(ExprPtr a, ExprPtr i):arr(std::move(a)),idx(std::move(i)){} };

struct SLet : Stmt {
  std::string name;
  std::unique_ptr<Type> annType; // may be null
  ExprPtr init;
  bool isUnique=false; // unique<T> sugar
  SLet(std::string n, std::unique_ptr<Type> t, ExprPtr e, bool u=false)
    :name(std::move(n)),annType(std::move(t)),init(std::move(e)),isUnique(u){}
};

struct SExpr : Stmt { ExprPtr e; explicit SExpr(ExprPtr e):e(std::move(e)){} };
struct SReturn: Stmt { ExprPtr e; explicit SReturn(ExprPtr e):e(std::move(e)){} };
struct SIf    : Stmt { ExprPtr cond; std::vector<StmtPtr> thenStmts, elseStmts; };
struct SWhile : Stmt { ExprPtr cond; std::vector<StmtPtr> body; };
struct SDefer : Stmt { ExprPtr e; explicit SDefer(ExprPtr e):e(std::move(e)){} };
struct SBreak : Stmt {};
struct SContinue : Stmt {};

struct Param { std::string name; std::unique_ptr<Type> ty; };

struct Func {
  std::string name;
  std::vector<Param> params;
  std::unique_ptr<Type> ret;
  std::vector<StmtPtr> body;
};

struct Program {
  std::vector<std::unique_ptr<Func>> funcs;
};
