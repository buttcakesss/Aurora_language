#pragma once
#include <string>
#include <cstdint>

enum class TokKind {
  Eof, Ident, IntLit, True, False,
  KwLet, KwFn, KwIf, KwElse, KwWhile, KwReturn, KwDefer, KwBreak, KwContinue,
  KwI32, KwI64, KwBool, KwPtr, KwUnique, KwVoid,
  LParen, RParen, LBrace, RBrace, LBracket, RBracket, Comma, Colon, Semicolon, Arrow,
  Plus, Minus, Star, Slash, Percent,
  Bang, AmpAmp, PipePipe,
  Eq, EqEq, BangEq, Lt, Le, Gt, Ge,
  PlusEq, MinusEq, StarEq, SlashEq, PercentEq
};

struct Token {
  TokKind kind;
  std::string lexeme;
  std::int64_t intValue = 0;
  int line=1, col=1;
};
