// sema.cpp
#include "sema.h"
#include "diagnostics.h"
#include <string>  


static inline bool isVoid(const Type& t) { return t.k == TyKind::Void; }
static inline void requireNonVoid(const Type& t, const char* where) {
  if (isVoid(t)) fatal(std::string("void value not allowed in ") + where);
}

void Sema::primaries(){
  // Builtins: i64 print/read, malloc/free
  {
    FnSig s; s.params.push_back(Type::i64()); s.ret = Type::i64();
    fns["print_i64"] = std::move(s);
  }
  {
    FnSig s; s.ret = Type::i64();
    fns["read_i64"] = std::move(s);
  }
  {
    FnSig s; s.params.push_back(Type::i64());
    s.ret = Type::ptr(Type::i64()); // treat as ptr<i64> for MVP
    fns["malloc"] = std::move(s);
  }
  {
    // free now returns void
    FnSig s; s.params.push_back(Type::ptr(Type::i64()));
    s.ret = Type::voidty();
    fns["free"] = std::move(s);
  }
}

std::unique_ptr<Type> Sema::infer(Expr& e){
  if (auto *x = dynamic_cast<EInt*>(&e))  { (void)x; return Type::i64(); }
  if (auto *b = dynamic_cast<EBool*>(&e)) { (void)b; return Type::boolean(); }

  if (auto *v = dynamic_cast<EVar*>(&e)){
    auto vi = scope.lookup(v->name);
    if (!vi) fatal("unknown variable: "+v->name);
    return vi->ty->clone();
  }

  if (auto *u = dynamic_cast<EUnary*>(&e)){
    auto t = infer(*u->rhs);
    requireNonVoid(*t, "unary operator");
    return t->clone();
  }

  if (auto *bin = dynamic_cast<EBin*>(&e)){
    if (bin->op==TokKind::Eq){
      auto tL = infer(*bin->lhs);
      auto tR = infer(*bin->rhs);
      if (isVoid(*tR)) fatal("cannot assign a void value");
      if (!tL->equals(*tR)) fatal("type mismatch in assignment: "+tL->str()+" vs "+tR->str());
      return tL;
    }

    // arithmetic => i64 (operands must be non-void)
    if (bin->op==TokKind::Plus || bin->op==TokKind::Minus || bin->op==TokKind::Star ||
        bin->op==TokKind::Slash || bin->op==TokKind::Percent){
      auto lt = infer(*bin->lhs), rt = infer(*bin->rhs);
      requireNonVoid(*lt, "arithmetic operator");
      requireNonVoid(*rt, "arithmetic operator");
      return Type::i64();
    }

    // comparisons => bool
    if (bin->op==TokKind::EqEq || bin->op==TokKind::BangEq || bin->op==TokKind::Lt ||
        bin->op==TokKind::Le   || bin->op==TokKind::Gt    || bin->op==TokKind::Ge){
      auto lt = infer(*bin->lhs), rt = infer(*bin->rhs);
      requireNonVoid(*lt, "comparison");
      requireNonVoid(*rt, "comparison");
      return Type::boolean();
    }

    // logical -> bool
    if (bin->op==TokKind::AmpAmp || bin->op==TokKind::PipePipe){
      auto lt = infer(*bin->lhs), rt = infer(*bin->rhs);
      requireNonVoid(*lt, "logical operator");
      requireNonVoid(*rt, "logical operator");
      return Type::boolean();
    }
  }

  if (auto *c = dynamic_cast<ECall*>(&e)){
    auto it = fns.find(c->callee);
    if (it==fns.end()) fatal("unknown function: "+c->callee);

    // Arity + argument type checks
    auto &sig = it->second;
    if (c->args.size() != sig.params.size())
      fatal("wrong number of arguments to "+c->callee);

    for (size_t k=0; k<c->args.size(); ++k){
      auto at = infer(*c->args[k]);
      if (isVoid(*at)) fatal("argument "+std::to_string(k+1)+" to "+c->callee+" is void");
      if (!at->equals(*sig.params[k]))
        fatal("argument "+std::to_string(k+1)+" type mismatch in "+c->callee);
    }
    return sig.ret->clone(); // may be void
  }

  if (auto *a = dynamic_cast<EArrayLit*>(&e)){
    if (a->elems.empty()) fatal("cannot infer type of empty array literal");
    auto elemType = infer(*a->elems[0]);
    // Check all elements have same type
    for (size_t i = 1; i < a->elems.size(); ++i){
      auto t = infer(*a->elems[i]);
      if (!t->equals(*elemType)) fatal("array literal has mixed types");
    }
    return Type::array(std::move(elemType), a->elems.size());
  }

  if (auto *idx = dynamic_cast<EIndex*>(&e)){
    auto arrType = infer(*idx->arr);
    // Allow indexing on both arrays and pointers
    if (arrType->k != TyKind::Array && arrType->k != TyKind::Ptr) {
      fatal("indexing requires array or pointer type, got: "+arrType->str());
    }
    auto idxType = infer(*idx->idx);
    // (void is rejected implicitly here; require i64/i32 as you had)
    if (idxType->k != TyKind::I64 && idxType->k != TyKind::I32) fatal("array index must be integer");
    return arrType->elem->clone();
  }

  fatal("cannot infer expression");
}

void Sema::checkStmt(Stmt& s, const Type* currentRet, std::vector<Expr*>& defers){
  if (auto *sl = dynamic_cast<SLet*>(&s)){
    auto t = sl->annType ? sl->annType->clone() : infer(*sl->init);
    if (t->k == TyKind::Void)
      fatal("variable '"+sl->name+"' cannot have type void");
    if (!scope.declare(sl->name, std::move(t), sl->isUnique))
      fatal("redeclaration: "+sl->name);
    if (sl->isUnique) {
      // implicit RAII: defer free(name);
      auto call = new ECall("free");
      call->args.push_back(std::make_unique<EVar>(sl->name));
      defers.push_back(call);
    }
    return;
  }

  if (auto *se = dynamic_cast<SExpr*>(&s)){
    (void)infer(*se->e); // may be void; that's allowed when used as a statement
    return;
  }

  if (auto *sr = dynamic_cast<SReturn*>(&s)){
    if (!currentRet) fatal("return outside function");
    if (currentRet->k == TyKind::Void){
      if (sr->e) fatal("void function cannot return a value");
    } else {
      if (!sr->e) fatal("non-void function must return a value");
      auto t = infer(*sr->e);
      if (!t->equals(*currentRet))
        fatal("return type mismatch, expected "+currentRet->str()+" got "+t->str());
    }
    return;
  }

  if (auto *si = dynamic_cast<SIf*>(&s)){
    { auto t = infer(*si->cond); requireNonVoid(*t, "if condition"); }
    scope.push(); {
      std::vector<Expr*> localDefers;
      for(auto& st: si->thenStmts) checkStmt(*st, currentRet, localDefers); /* defers run at scope exit */
    }
    scope.pop();
    scope.push(); {
      std::vector<Expr*> localDefers;
      for(auto& st: si->elseStmts) checkStmt(*st, currentRet, localDefers);
    }
    scope.pop();
    return;
  }

  if (auto *sw = dynamic_cast<SWhile*>(&s)){
    { auto t = infer(*sw->cond); requireNonVoid(*t, "while condition"); }
    scope.push(); {
      std::vector<Expr*> localDefers;
      loopDepth++;  // Enter loop
      for (auto& st: sw->body) checkStmt(*st, currentRet, localDefers);
      loopDepth--;  // Exit loop
    }
    scope.pop();
    return;
  }

  if (auto *sd = dynamic_cast<SDefer*>(&s)){
    // defer accepts void calls or value-producing expressions; type-checked above
    defers.push_back(sd->e.get());
    return;
  }
  
  if (dynamic_cast<SBreak*>(&s)){
    if (loopDepth == 0) fatal("break statement outside of loop");
    return;
  }
  
  if (dynamic_cast<SContinue*>(&s)){
    if (loopDepth == 0) fatal("continue statement outside of loop");
    return;
  }
}

void Sema::analyze(Program& p){
  primaries();

  // gather function signatures
  for (auto& fn : p.funcs){
    FnSig sig;
    for (auto& pr : fn->params){
      if (pr.ty->k == TyKind::Void)
        fatal("parameter '"+pr.name+"' cannot have type void");
      sig.params.push_back(pr.ty->clone());
    }
    sig.ret = fn->ret->clone();
    fns[fn->name] = std::move(sig);
  }

  // type-check bodies
  for (auto& fn : p.funcs){
    scope.push();
    for (auto& pr : fn->params) scope.declare(pr.name, pr.ty->clone());
    std::vector<Expr*> defers;
    for (auto& st : fn->body) checkStmt(*st, fn->ret.get(), defers);
    scope.pop();
  }
}
