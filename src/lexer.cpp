// lexer.cpp
#include "lexer.h"
#include <cctype>

char Lexer::get(){ char c=peek(); if(c=='\0') return c; ++i; if(c=='\n'){++line; col=1;} else ++col; return c; }
void Lexer::skipWS(){
  for(;;){
    char c=peek();
    if (isspace((unsigned char)c)) { get(); continue; }
    if (c=='/' && i+1<src.size() && src[i+1]=='/'){ while(peek() && peek()!='\n') get(); continue; }
    if (c=='/' && i+1<src.size() && src[i+1]=='*'){ get(); get(); while(peek() && !(peek()=='*' && i+1<src.size() && src[i+1]=='/')) get(); if (peek()){get();get();} continue; }
    break;
  }
}

Token Lexer::identOrKw(){
  int L=line,C=col;
  std::string s;
  while (std::isalnum((unsigned char)peek()) || peek()=='_') s.push_back(get());
  TokKind k = TokKind::Ident;
  if (s=="let") k=TokKind::KwLet;
  else if (s=="fn") k=TokKind::KwFn;
  else if (s=="if") k=TokKind::KwIf;
  else if (s=="else") k=TokKind::KwElse;
  else if (s=="while") k=TokKind::KwWhile;
  else if (s=="return") k=TokKind::KwReturn;
  else if (s=="defer") k=TokKind::KwDefer;
  else if (s=="break") k=TokKind::KwBreak;
  else if (s=="continue") k=TokKind::KwContinue;
  else if (s=="true") k=TokKind::True;
  else if (s=="false") k=TokKind::False;
  else if (s=="i32") k=TokKind::KwI32;
  else if (s=="i64") k=TokKind::KwI64;
  else if (s=="bool") k=TokKind::KwBool;
  else if (s=="void") k=TokKind::KwVoid;
  else if (s=="ptr") k=TokKind::KwPtr;
  else if (s=="unique") k=TokKind::KwUnique;
  return Token{k,s,0,L,C};
}

Token Lexer::number(){
  int L=line,C=col; std::string s;
  while (std::isdigit((unsigned char)peek())) s.push_back(get());
  return Token{TokKind::IntLit,s,std::stoll(s),L,C};
}

std::vector<Token> Lexer::lex(){
  std::vector<Token> v;
  for(;;){
    skipWS();
    int L=line,C=col;
    char c=peek();
    if (!c){ v.push_back(Token{TokKind::Eof,"",0,L,C}); break; }
    if (std::isalpha((unsigned char)c) || c=='_') { v.push_back(identOrKw()); continue; }
    if (std::isdigit((unsigned char)c)) { v.push_back(number()); continue; }
    auto two=[&](TokKind k)->void{ get(); get(); v.push_back(Token{k,"",0,L,C}); };
    auto one=[&](TokKind k)->void{ get(); v.push_back(Token{k,"",0,L,C}); };
    if (c=='(') { one(TokKind::LParen); continue; }
    if (c==')') { one(TokKind::RParen); continue; }
    if (c == '['){one(TokKind::LBracket); continue; }
    if (c == ']'){one(TokKind::RBracket); continue; }
    if (c=='{') { one(TokKind::LBrace); continue; }
    if (c=='}') { one(TokKind::RBrace); continue; }
    if (c==',') { one(TokKind::Comma); continue; }
    if (c==':') { one(TokKind::Colon); continue; }
    if (c==';') { one(TokKind::Semicolon); continue; }
    if (c=='+' ) { if (i+1<src.size() && src[i+1]=='=') two(TokKind::PlusEq); else one(TokKind::Plus); continue; }
    if (c=='-' ){ if (i+1<src.size() && src[i+1]=='>') { two(TokKind::Arrow); } else if (i+1<src.size() && src[i+1]=='=') two(TokKind::MinusEq); else one(TokKind::Minus); continue; }
    if (c=='*' ) { if (i+1<src.size() && src[i+1]=='=') two(TokKind::StarEq); else one(TokKind::Star); continue; }
    if (c=='/' ) { if (i+1<src.size() && src[i+1]=='=') two(TokKind::SlashEq); else one(TokKind::Slash); continue; }
    if (c=='%')  { if (i+1<src.size() && src[i+1]=='=') two(TokKind::PercentEq); else one(TokKind::Percent); continue; }
    if (c=='!'){ if (i+1<src.size() && src[i+1]=='=') two(TokKind::BangEq); else one(TokKind::Bang); continue; }
    if (c=='&' && i+1<src.size() && src[i+1]=='&'){ two(TokKind::AmpAmp); continue; }
    if (c=='|' && i+1<src.size() && src[i+1]=='|'){ two(TokKind::PipePipe); continue; }
    if (c=='='){ if (i+1<src.size() && src[i+1]=='=') two(TokKind::EqEq); else one(TokKind::Eq); continue; }
    if (c=='<'){ if (i+1<src.size() && src[i+1]=='=') two(TokKind::Le); else one(TokKind::Lt); continue; }
    if (c=='>'){ if (i+1<src.size() && src[i+1]=='=') two(TokKind::Ge); else one(TokKind::Gt); continue; }
    // Unknown
    one(TokKind::Semicolon); // soft-landing
  }
  return v;
}