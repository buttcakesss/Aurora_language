// parser.h
#pragma once
#include "lexer.h"
#include "ast.h"
#include "types.h"
#include <vector>

class Parser {
  const std::vector<Token> toks;
  size_t i=0;
public:
  explicit Parser(std::vector<Token> t):toks(std::move(t)){}
  std::unique_ptr<Program> parseProgram();
private:
  const Token& peek() const { return toks[i]; }
  const Token& get(){ return toks[i++]; }
  bool accept(TokKind k){ if (peek().kind==k){ ++i; return true; } return false; }
  void expect(TokKind k, const char* msg);
  std::unique_ptr<Func> parseFunc();
  std::unique_ptr<Type> parseType();
  std::vector<StmtPtr> parseBlock();
  StmtPtr parseStmt();
  ExprPtr parseExpr();
  ExprPtr parseAssign();
  ExprPtr parseOr();
  ExprPtr parseAnd();
  ExprPtr parseEq();
  ExprPtr parseRel();
  ExprPtr parseAdd();
  ExprPtr parseMul();
  ExprPtr parseUnary();
  ExprPtr parsePostfix();
};
