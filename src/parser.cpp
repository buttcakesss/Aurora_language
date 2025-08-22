// parser.cpp
#include "parser.h"
#include "diagnostics.h"

void Parser::expect(TokKind k, const char* msg){
  if (!accept(k)) fatal(std::string("expected ")+msg);
}

std::unique_ptr<Type> Parser::parseType(){
  std::unique_ptr<Type> baseType;
  if (accept(TokKind::KwI32)) baseType = Type::i32();
  else if (accept(TokKind::KwI64)) baseType = Type::i64();
  else if (accept(TokKind::KwBool)) baseType = Type::boolean();
  else if (accept(TokKind::KwVoid)) baseType = Type::voidty();
  else if (accept(TokKind::KwPtr)) {
    expect(TokKind::Lt, "'<'");
    auto t=parseType();
    expect(TokKind::Gt, "'>'");
    baseType = Type::ptr(std::move(t));
  }
  else fatal("unknown type");
  
  // Check for array syntax T[size]
  if (peek().kind == TokKind::LBracket) {
    get(); // consume '['
    if (peek().kind != TokKind::IntLit) fatal("expected integer size for array");
    int64_t size = get().intValue;
    expect(TokKind::RBracket, "']'");
    return Type::array(std::move(baseType), size);
  }
  
  return baseType;
}

std::vector<StmtPtr> Parser::parseBlock(){
  expect(TokKind::LBrace,"'{'");
  std::vector<StmtPtr> stmts;
  while (peek().kind!=TokKind::RBrace && peek().kind!=TokKind::Eof){
    stmts.push_back(parseStmt());
  }
  expect(TokKind::RBrace,"'}'");
  return stmts;
}

StmtPtr Parser::parseStmt(){
  if (accept(TokKind::KwLet)){
    bool isUnique=false;
    if (accept(TokKind::KwUnique)) { expect(TokKind::Lt,"'<'"); parseType(); expect(TokKind::Gt,"'>'"); isUnique=true; }
    if (peek().kind!=TokKind::Ident) fatal("expected identifier after 'let'");
    std::string name = get().lexeme;
    std::unique_ptr<Type> ann;
    if (accept(TokKind::Colon)) ann = parseType();
    expect(TokKind::Eq,"'='");
    auto init = parseExpr();
    expect(TokKind::Semicolon,"';'");
    return std::make_unique<SLet>(name, std::move(ann), std::move(init), isUnique);
  }
  if (accept(TokKind::KwReturn)){
    auto e = (peek().kind==TokKind::Semicolon)? ExprPtr{} : parseExpr();
    expect(TokKind::Semicolon,"';'");
    return std::make_unique<SReturn>(std::move(e));
  }
  if (accept(TokKind::KwIf)){
    auto s = std::make_unique<SIf>();
    expect(TokKind::LParen,"'('");
    s->cond = parseExpr();
    expect(TokKind::RParen,"')'");
    s->thenStmts = parseBlock();
    if (accept(TokKind::KwElse)) s->elseStmts = parseBlock();
    return s;
  }
  if (accept(TokKind::KwWhile)){
    auto s = std::make_unique<SWhile>();
    expect(TokKind::LParen,"'('");
    s->cond = parseExpr();
    expect(TokKind::RParen,"')'");
    s->body = parseBlock();
    return s;
  }
  if (accept(TokKind::KwDefer)){
    auto e=parseExpr();
    expect(TokKind::Semicolon,"';'");
    return std::make_unique<SDefer>(std::move(e));
  }
  if (accept(TokKind::KwBreak)){
    expect(TokKind::Semicolon,"';'");
    return std::make_unique<SBreak>();
  }
  if (accept(TokKind::KwContinue)){
    expect(TokKind::Semicolon,"';'");
    return std::make_unique<SContinue>();
  }
  // expr;
  auto e = parseExpr();
  expect(TokKind::Semicolon,"';'");
  return std::make_unique<SExpr>(std::move(e));
}

ExprPtr Parser::parseExpr(){ return parseAssign(); }
ExprPtr Parser::parseAssign(){
  auto lhs = parseOr();
  if (accept(TokKind::Eq)){
    auto rhs = parseAssign();
    return std::make_unique<EBin>(std::move(lhs), TokKind::Eq, std::move(rhs));
  }
  // Handle compound assignments by lowering them to x = x op y
  // We need to check if it's a simple variable to avoid complex lvalue issues
  if (peek().kind == TokKind::PlusEq || peek().kind == TokKind::MinusEq ||
      peek().kind == TokKind::StarEq || peek().kind == TokKind::SlashEq ||
      peek().kind == TokKind::PercentEq) {
    
    // For now, compound assignments only work with simple variables
    auto* varLhs = dynamic_cast<EVar*>(lhs.get());
    if (!varLhs) {
      fatal("compound assignment requires simple variable on left side");
      return lhs;
    }
    
    std::string varName = varLhs->name;
    TokKind compoundOp = get().kind;
    auto rhs = parseAssign();
    
    // Determine the binary operator from the compound assignment
    TokKind binOp;
    if (compoundOp == TokKind::PlusEq) binOp = TokKind::Plus;
    else if (compoundOp == TokKind::MinusEq) binOp = TokKind::Minus;
    else if (compoundOp == TokKind::StarEq) binOp = TokKind::Star;
    else if (compoundOp == TokKind::SlashEq) binOp = TokKind::Slash;
    else binOp = TokKind::Percent; // PercentEq
    
    // Create a new variable reference for the right side of the binary operation
    auto varRef = std::make_unique<EVar>(varName);
    
    // Create the binary operation: var op rhs
    auto binExpr = std::make_unique<EBin>(std::move(varRef), binOp, std::move(rhs));
    
    // Create the assignment: lhs = (var op rhs)
    return std::make_unique<EBin>(std::move(lhs), TokKind::Eq, std::move(binExpr));
  }
  return lhs;
}
ExprPtr Parser::parseOr(){
  auto e = parseAnd();
  while (accept(TokKind::PipePipe)) e = std::make_unique<EBin>(std::move(e), TokKind::PipePipe, parseAnd());
  return e;
}
ExprPtr Parser::parseAnd(){
  auto e = parseEq();
  while (accept(TokKind::AmpAmp)) e = std::make_unique<EBin>(std::move(e), TokKind::AmpAmp, parseEq());
  return e;
}
ExprPtr Parser::parseEq(){
  auto e = parseRel();
  for(;;){
    if (accept(TokKind::EqEq)) e = std::make_unique<EBin>(std::move(e), TokKind::EqEq, parseRel());
    else if (accept(TokKind::BangEq)) e = std::make_unique<EBin>(std::move(e), TokKind::BangEq, parseRel());
    else break;
  }
  return e;
}
ExprPtr Parser::parseRel(){
  auto e = parseAdd();
  for(;;){
    if (accept(TokKind::Lt)) e = std::make_unique<EBin>(std::move(e), TokKind::Lt, parseAdd());
    else if (accept(TokKind::Le)) e = std::make_unique<EBin>(std::move(e), TokKind::Le, parseAdd());
    else if (accept(TokKind::Gt)) e = std::make_unique<EBin>(std::move(e), TokKind::Gt, parseAdd());
    else if (accept(TokKind::Ge)) e = std::make_unique<EBin>(std::move(e), TokKind::Ge, parseAdd());
    else break;
  }
  return e;
}
ExprPtr Parser::parseAdd(){
  auto e = parseMul();
  for(;;){
    if (accept(TokKind::Plus))  e = std::make_unique<EBin>(std::move(e), TokKind::Plus, parseMul());
    else if (accept(TokKind::Minus)) e = std::make_unique<EBin>(std::move(e), TokKind::Minus, parseMul());
    else break;
  }
  return e;
}
ExprPtr Parser::parseMul(){
  auto e = parseUnary();
  for(;;){
    if (accept(TokKind::Star))  e = std::make_unique<EBin>(std::move(e), TokKind::Star, parseUnary());
    else if (accept(TokKind::Slash)) e = std::make_unique<EBin>(std::move(e), TokKind::Slash, parseUnary());
    else if (accept(TokKind::Percent)) e = std::make_unique<EBin>(std::move(e), TokKind::Percent, parseUnary());
    else break;
  }
  return e;
}
ExprPtr Parser::parseUnary(){
  if (accept(TokKind::Bang)) return std::make_unique<EUnary>(TokKind::Bang, parseUnary());
  if (accept(TokKind::Minus)) return std::make_unique<EUnary>(TokKind::Minus, parseUnary());
  return parsePostfix();
}
ExprPtr Parser::parsePostfix(){
  ExprPtr e;
  
  // Parse primary expression
  if (peek().kind==TokKind::Ident){
    auto id = get().lexeme;
    if (accept(TokKind::LParen)){
      auto call = std::make_unique<ECall>(id);
      if (peek().kind!=TokKind::RParen){
        call->args.push_back(parseExpr());
        while (accept(TokKind::Comma)) call->args.push_back(parseExpr());
      }
      expect(TokKind::RParen,"')'");
      e = std::move(call);
    } else {
      e = std::make_unique<EVar>(id);
    }
  }
  else if (peek().kind==TokKind::IntLit){ auto v=get().intValue; e = std::make_unique<EInt>(v); }
  else if (accept(TokKind::True)) e = std::make_unique<EBool>(true);
  else if (accept(TokKind::False)) e = std::make_unique<EBool>(false);
  else if (accept(TokKind::LBracket)){
    // Array literal [e1, e2, ...]
    std::vector<ExprPtr> elems;
    if (peek().kind != TokKind::RBracket){
      elems.push_back(parseExpr());  
      while (accept(TokKind::Comma)) elems.push_back(parseExpr());
    }
    expect(TokKind::RBracket,"']'");
    e = std::make_unique<EArrayLit>(std::move(elems));
  }
  else if (accept(TokKind::LParen)){ e=parseExpr(); expect(TokKind::RParen,"')'"); }
  else fatal("expected expression");
  
  // Parse postfix operations (array indexing)
  while (peek().kind == TokKind::LBracket) {
    get(); // consume '['
    auto idx = parseExpr();
    expect(TokKind::RBracket, "']'");
    e = std::make_unique<EIndex>(std::move(e), std::move(idx));
  }
  
  return e;
}

std::unique_ptr<Func> Parser::parseFunc(){
  expect(TokKind::KwFn,"'fn'");
  if (peek().kind!=TokKind::Ident) fatal("expected function name");
  auto name = get().lexeme;
  expect(TokKind::LParen,"'('");
  std::vector<Param> params;
  if (peek().kind!=TokKind::RParen){
    while (true){
      if (peek().kind!=TokKind::Ident) fatal("expected parameter name");
      Param p; p.name=get().lexeme;
      expect(TokKind::Colon,"':'");
      p.ty = parseType();
      params.push_back(std::move(p));
      if (!accept(TokKind::Comma)) break;
    }
  }
  expect(TokKind::RParen,"')'");
  expect(TokKind::Arrow,"'->'");
  auto ret = parseType();
  auto body = parseBlock();

  auto fn = std::make_unique<Func>();
  fn->name = std::move(name);
  fn->params = std::move(params);
  fn->ret = std::move(ret);
  fn->body = std::move(body);
  return fn;
}

std::unique_ptr<Program> Parser::parseProgram(){
  auto p = std::make_unique<Program>();
  while (peek().kind!=TokKind::Eof){
    p->funcs.push_back(parseFunc());
  }
  return p;
}