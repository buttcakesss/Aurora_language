// lexer.h
#pragma once
#include "token.h"
#include <string>
#include <vector>

class Lexer {
  const std::string src;
  size_t i=0;
  int line=1, col=1;
public:
  explicit Lexer(std::string s):src(std::move(s)){}
  std::vector<Token> lex();
private:
  char peek() const { return i<src.size()? src[i] : '\0'; }
  char get();
  void skipWS();
  Token identOrKw();
  Token number();
};
