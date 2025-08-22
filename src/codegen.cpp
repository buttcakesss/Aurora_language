// codegen.cpp
#include "codegen.h"
#include "diagnostics.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>


struct BuilderWrap : llvm::IRBuilder<> { explicit BuilderWrap(llvm::LLVMContext& C):llvm::IRBuilder<>(C){} };

CodeGen::CodeGen(const std::string& name){
  ctx = std::make_unique<llvm::LLVMContext>();
  mod = std::make_unique<llvm::Module>(name, *ctx);
  builder = std::make_unique<BuilderWrap>(*ctx);

  // declare libc functions used by runtime/builtins
  auto i32 = llvm::Type::getInt32Ty(*ctx);
  auto i64 = llvm::Type::getInt64Ty(*ctx);
  auto i8p = llvm::Type::getInt8PtrTy(*ctx);
  declareBuiltin("printf",{i8p}, i32, /*vararg*/true);
  declareBuiltin("malloc",{i64}, i8p, false);
  declareBuiltin("free",{i8p}, llvm::Type::getVoidTy(*ctx), false);
  
  // declare Aurora runtime functions
  declareBuiltin("print_i64",{i64}, i64, false);
  declareBuiltin("read_i64",{}, i64, false);
}

CodeGen::~CodeGen() = default;  // Destructor definition

llvm::Function* CodeGen::declareBuiltin(const char* name, std::vector<llvm::Type*> params, llvm::Type* ret, bool vararg){
  auto FT = llvm::FunctionType::get(ret, params, vararg);
  auto F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name, mod.get());
  return F;
}

llvm::Type* CodeGen::tyLLVM(const ::Type& t){
  switch(t.k){
    case TyKind::I32: return llvm::Type::getInt32Ty(*ctx);
    case TyKind::I64: return llvm::Type::getInt64Ty(*ctx);
    case TyKind::Bool: return llvm::Type::getInt1Ty(*ctx);
    case TyKind::Void: return llvm::Type::getVoidTy(*ctx);
    case TyKind::Ptr:  return llvm::PointerType::getUnqual(tyLLVM(*t.elem));
    case TyKind::Array: return llvm::ArrayType::get(tyLLVM(*t.elem), t.arraySize);
  }
  return llvm::Type::getVoidTy(*ctx);
}

llvm::Value* CodeGen::genExpr(Expr& e){
  auto& B = *static_cast<BuilderWrap*>(builder.get());
  if (auto *x = dynamic_cast<EInt*>(&e)) return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*ctx), x->v, true);
  if (auto *b = dynamic_cast<EBool*>(&e)) return llvm::ConstantInt::get(llvm::Type::getInt1Ty(*ctx), b->v);
  if (auto *v = dynamic_cast<EVar*>(&e)){
    auto it = namedValues.find(v->name); if (it==namedValues.end()) fatal("unknown var: "+v->name);
    auto typeIt = namedTypes.find(v->name); if (typeIt==namedTypes.end()) fatal("unknown var type: "+v->name);
    // Arrays should never be loaded as values - always return the pointer
    if (typeIt->second->isArrayTy()) {
      return it->second;  // Return alloca pointer
    }
    // For non-arrays, load the value
    auto load = B.CreateLoad(typeIt->second, it->second);
    // Set a name for debugging
    load->setName(v->name);
    return load;
  }
  if (auto *u = dynamic_cast<EUnary*>(&e)){
    auto r = genExpr(*u->rhs);
    if (u->op==TokKind::Minus) return B.CreateNeg(r);
    if (u->op==TokKind::Bang)  return B.CreateNot(r);
    fatal("unary op");
  }
  if (auto *bin = dynamic_cast<EBin*>(&e)){
    if (bin->op==TokKind::Eq){
      // Check if LHS is array/pointer indexing
      if (auto *idx = dynamic_cast<EIndex*>(bin->lhs.get())){
        auto arrVar = dynamic_cast<EVar*>(idx->arr.get());
        if (!arrVar) fatal("array/pointer indexing on non-variable");
        
        auto it = namedValues.find(arrVar->name);
        if (it == namedValues.end()) fatal("unknown array/pointer variable in assignment");
        
        auto typeIt = namedTypes.find(arrVar->name);
        if (typeIt == namedTypes.end()) fatal("unknown array/pointer type");
        
        auto arrPtr = it->second;  // Pointer to array or pointer value
        auto index = genExpr(*idx->idx);
        
        // Convert index to i32 if needed
        if (index->getType() != llvm::Type::getInt32Ty(*ctx)) {
          index = B.CreateTrunc(index, llvm::Type::getInt32Ty(*ctx));
        }
        
        llvm::Value* ptr;
        llvm::Type* elemType;
        
        if (typeIt->second->isArrayTy()) {
          // Array case: need to index with [0, index]
          auto zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*ctx), 0);
          auto arrayType = llvm::cast<llvm::ArrayType>(typeIt->second);
          elemType = arrayType->getElementType();
          ptr = B.CreateInBoundsGEP(typeIt->second, arrPtr, {zero, index});
        } else {
          // Pointer case: need to load the pointer value first, then index
          // For now assume i64 elements
          elemType = llvm::Type::getInt64Ty(*ctx);
          auto ptrValue = B.CreateLoad(llvm::PointerType::getUnqual(*ctx), arrPtr, "ptr_load");
          ptr = B.CreateInBoundsGEP(elemType, ptrValue, index);
        }
        
        auto rv = genExpr(*bin->rhs);
        auto store = B.CreateStore(rv, ptr);
        store->setAlignment(llvm::Align(8)); // 8-byte alignment
        return rv;
      }
      
      auto lhs = dynamic_cast<EVar*>(bin->lhs.get());
      if (!lhs) fatal("assignment target must be a variable");
      auto it = namedValues.find(lhs->name); if (it==namedValues.end()) fatal("unknown var in assign");
      auto rv = genExpr(*bin->rhs);
      B.CreateStore(rv, it->second);
      return rv;
    }
    auto a = genExpr(*bin->lhs);
    auto b = genExpr(*bin->rhs);
    switch (bin->op){
      case TokKind::Plus: return B.CreateAdd(a,b);
      case TokKind::Minus: return B.CreateSub(a,b);
      case TokKind::Star: return B.CreateMul(a,b);
      case TokKind::Slash: return B.CreateSDiv(a,b);
      case TokKind::Percent: return B.CreateSRem(a,b);
      case TokKind::EqEq: return B.CreateICmpEQ(a,b);
      case TokKind::BangEq: return B.CreateICmpNE(a,b);
      case TokKind::Lt: return B.CreateICmpSLT(a,b);
      case TokKind::Le: return B.CreateICmpSLE(a,b);
      case TokKind::Gt: return B.CreateICmpSGT(a,b);
      case TokKind::Ge: return B.CreateICmpSGE(a,b);
      case TokKind::AmpAmp: return B.CreateAnd(a,b);
      case TokKind::PipePipe: return B.CreateOr(a,b);
      default: break;
    }
    fatal("binary op");
  }
  if (auto *c = dynamic_cast<ECall*>(&e)){
    auto F = mod->getFunction(c->callee);
    if (!F) fatal("unknown callee: "+c->callee);
    std::vector<llvm::Value*> argv;
    for (auto& a : c->args) argv.push_back(genExpr(*a));
    return builder->CreateCall(F, argv, c->callee=="print_i64"?"print_ret":"");
  }
  if (auto *a = dynamic_cast<EArrayLit*>(&e)){
    // Arrays are stack allocated - create alloca and initialize
    if (a->elems.empty()) fatal("empty array literal");
    auto elemType = genExpr(*a->elems[0])->getType();
    auto arrayType = llvm::ArrayType::get(elemType, a->elems.size());
    auto alloca = B.CreateAlloca(arrayType, nullptr, "array_lit");
    
    // Initialize array elements
    for (size_t i = 0; i < a->elems.size(); ++i){
      auto idx = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*ctx), i);
      auto zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*ctx), 0);
      auto ptr = B.CreateInBoundsGEP(arrayType, alloca, {zero, idx});
      auto store = B.CreateStore(genExpr(*a->elems[i]), ptr);
      store->setAlignment(llvm::Align(8)); // 8-byte alignment
    }
    return alloca;
  }
  if (auto *idx = dynamic_cast<EIndex*>(&e)){
    // Get the array/pointer expression - this should return a pointer
    llvm::Value* arrExpr = genExpr(*idx->arr);
    
    // The result of genExpr for an array or pointer should be a pointer
    if (!arrExpr->getType()->isPointerTy()) {
      fatal("array/pointer expression must be a pointer");
    }
    
    // For LLVM 15+, we need to track the pointee type separately
    // since opaque pointers don't carry type information
    llvm::Type* pointeeType = nullptr;
    bool isArrayType = false;
    
    // If the expression is a variable, get its type from our map
    if (auto *arrVar = dynamic_cast<EVar*>(idx->arr.get())) {
      auto typeIt = namedTypes.find(arrVar->name);
      if (typeIt != namedTypes.end()) {
        if (typeIt->second->isArrayTy()) {
          pointeeType = typeIt->second;
          isArrayType = true;
        } else if (typeIt->second->isPointerTy()) {
          // For pointer types, the pointee is what we're pointing to
          // We need to track this separately - for now use i64 as default
          pointeeType = llvm::Type::getInt64Ty(*ctx);
          isArrayType = false;
        }
      }
    }
    
    if (!pointeeType) {
      // Default to i64 for pointer indexing if we can't determine the type
      pointeeType = llvm::Type::getInt64Ty(*ctx);
      isArrayType = false;
    }
    
    llvm::Type* elemType;
    if (isArrayType) {
      auto arrayType = llvm::cast<llvm::ArrayType>(pointeeType);
      elemType = arrayType->getElementType();
    } else {
      // For pointers, the element type is the pointee type
      elemType = pointeeType;
    }
    
    // Generate the index expression
    auto index = genExpr(*idx->idx);
    
    // Convert index to i32 (LLVM prefers i32 for GEP indices)
    if (index->getType()->isIntegerTy(64)) {
      index = B.CreateTrunc(index, llvm::Type::getInt32Ty(*ctx), "idx_trunc");
    }
    
    llvm::Value* gep;
    if (isArrayType) {
      // For arrays: getelementptr inbounds [N x T], ptr %arr, i32 0, i32 %index
      auto zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*ctx), 0);
      gep = B.CreateInBoundsGEP(pointeeType, arrExpr, {zero, index}, "arrayidx");
    } else {
      // For pointers: getelementptr inbounds T, ptr %ptr, i32 %index
      gep = B.CreateInBoundsGEP(elemType, arrExpr, index, "ptridx");
    }
    
    // Load the element
    auto load = B.CreateLoad(elemType, gep, "elem");
    load->setAlignment(llvm::Align(8));
    
    return load;
  }
  fatal("expr codegen");
}

void CodeGen::runDefers(std::vector<Expr*>& defers){
  // Insert in reverse order to match LIFO semantics
  for (int i=(int)defers.size()-1;i>=0;--i) (void)genExpr(*defers[i]);
}

void CodeGen::genStmt(Stmt& s, llvm::Function* fn){
  auto& B = *static_cast<BuilderWrap*>(builder.get());
  if (auto *sl = dynamic_cast<SLet*>(&s)){
    // infer LLVM type from init (simplified: i64 or bool or pointer or array via annotation)
    llvm::Type* ty = nullptr;
    if (sl->annType) ty = tyLLVM(*sl->annType);
    else {
      if (dynamic_cast<EInt*>(sl->init.get())) ty = llvm::Type::getInt64Ty(*ctx);
      else if (dynamic_cast<EBool*>(sl->init.get())) ty = llvm::Type::getInt1Ty(*ctx);
      else if (auto *arr = dynamic_cast<EArrayLit*>(sl->init.get())) {
        if (!arr->elems.empty()) {
          auto elemType = genExpr(*arr->elems[0])->getType();
          ty = llvm::ArrayType::get(elemType, arr->elems.size());
        } else ty = llvm::Type::getInt64Ty(*ctx);
      }
      else ty = llvm::Type::getInt64Ty(*ctx);
    }
    auto alloca = B.CreateAlloca(ty, nullptr, sl->name);
    
    // Handle array literal initialization differently
    if (auto *arr = dynamic_cast<EArrayLit*>(sl->init.get())) {
      // Get element type for array initialization
      llvm::Type* elemType = nullptr;
      if (!arr->elems.empty()) {
        elemType = genExpr(*arr->elems[0])->getType();
      }
      
      // Initialize array elements
      for (size_t i = 0; i < arr->elems.size(); ++i){
        auto idx = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*ctx), i);
        auto zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*ctx), 0);
        auto ptr = B.CreateInBoundsGEP(ty, alloca, {zero, idx});
        auto store = B.CreateStore(genExpr(*arr->elems[i]), ptr);
        store->setAlignment(llvm::Align(8)); // 8-byte alignment for i64
      }
    } else {
      auto val = genExpr(*sl->init);
      // cast bool to i64 for storage if mismatched
      if (val->getType()!=ty){
        if (val->getType()->isIntegerTy(1) && ty->isIntegerTy(64))
          val = B.CreateZExt(val, ty);
        else if (val->getType()->isIntegerTy(64) && ty->isIntegerTy(1))
          val = B.CreateICmpNE(val, llvm::ConstantInt::get(val->getType(), 0));
      }
      B.CreateStore(val, alloca);
    }
    namedValues[sl->name]=alloca;
    namedTypes[sl->name]=ty;

    if (sl->isUnique){
      // implicit RAII: defer free(name)
      auto v = std::make_unique<EVar>(sl->name);
      auto call = std::make_unique<ECall>("free");
      call->args.push_back(std::move(v));
      auto tmp = call.release(); // leaked into defers; owned by AST in real impl
      // naive: attach to function-level list on the side — handled in Sema; here we just ignore since we also support explicit `defer`.
      (void)tmp;
    }
    return;
  }
  if (auto *se = dynamic_cast<SExpr*>(&s)){ (void)genExpr(*se->e); return; }
  if (auto *sr = dynamic_cast<SReturn*>(&s)){ 
    if (sr->e) {
      auto rv = genExpr(*sr->e); 
      B.CreateRet(rv); 
    } else {
      B.CreateRetVoid();
    }
    return; 
  }
  if (auto *si = dynamic_cast<SIf*>(&s)){
    auto cond = genExpr(*si->cond);
    cond = B.CreateICmpNE(cond, cond->getType()->isIntegerTy(1) ? llvm::ConstantInt::get(cond->getType(), 0) : llvm::ConstantInt::get(cond->getType(), 0));
    auto TheFunction = B.GetInsertBlock()->getParent();
    auto ThenBB = llvm::BasicBlock::Create(*ctx, "then", TheFunction);
    auto ElseBB = llvm::BasicBlock::Create(*ctx, "else");
    auto MergeBB= llvm::BasicBlock::Create(*ctx, "ifend");
    B.CreateCondBr(cond, ThenBB, ElseBB);
    B.SetInsertPoint(ThenBB); 
    for (auto& st : si->thenStmts) genStmt(*st, fn); 
    if (!B.GetInsertBlock()->getTerminator()) B.CreateBr(MergeBB);
    
    TheFunction->insert(TheFunction->end(), ElseBB);
    B.SetInsertPoint(ElseBB); 
    for (auto& st : si->elseStmts) genStmt(*st, fn); 
    if (!B.GetInsertBlock()->getTerminator()) B.CreateBr(MergeBB);
    
    TheFunction->insert(TheFunction->end(), MergeBB);
    B.SetInsertPoint(MergeBB);
    return;
  }
  if (auto *sw = dynamic_cast<SWhile*>(&s)){
    auto TheFunction = B.GetInsertBlock()->getParent();
    auto CondBB = llvm::BasicBlock::Create(*ctx, "while.cond", TheFunction);
    auto BodyBB = llvm::BasicBlock::Create(*ctx, "while.body");
    auto EndBB  = llvm::BasicBlock::Create(*ctx, "while.end");
    
    // Push loop blocks onto stacks for break/continue
    loopExitStack.push_back(EndBB);
    loopContinueStack.push_back(CondBB);
    
    B.CreateBr(CondBB);
    B.SetInsertPoint(CondBB);
    auto c = genExpr(*sw->cond);
    c = B.CreateICmpNE(c, c->getType()->isIntegerTy(1) ? llvm::ConstantInt::get(c->getType(), 0) : llvm::ConstantInt::get(c->getType(), 0));
    B.CreateCondBr(c, BodyBB, EndBB);
    TheFunction->insert(TheFunction->end(), BodyBB);
    B.SetInsertPoint(BodyBB);
    for (auto& st : sw->body) genStmt(*st, fn);
    if (!B.GetInsertBlock()->getTerminator()) B.CreateBr(CondBB);
    
    // Pop loop blocks from stacks
    loopExitStack.pop_back();
    loopContinueStack.pop_back();
    
    TheFunction->insert(TheFunction->end(), EndBB);
    B.SetInsertPoint(EndBB);
    return;
  }
  if (auto *sd = dynamic_cast<SDefer*>(&s)){
    (void)sd; /* in this MVP, explicit 'defer' can be inlined earlier by Sema; for brevity omitted */
    return;
  }
  
  if (dynamic_cast<SBreak*>(&s)){
    if (loopExitStack.empty()) fatal("break statement outside of loop");
    B.CreateBr(loopExitStack.back());
    // Create a new block after break (unreachable code)
    auto TheFunction = B.GetInsertBlock()->getParent();
    auto AfterBreak = llvm::BasicBlock::Create(*ctx, "after.break", TheFunction);
    B.SetInsertPoint(AfterBreak);
    return;
  }
  
  if (dynamic_cast<SContinue*>(&s)){
    if (loopContinueStack.empty()) fatal("continue statement outside of loop");
    B.CreateBr(loopContinueStack.back());
    // Create a new block after continue (unreachable code)
    auto TheFunction = B.GetInsertBlock()->getParent();
    auto AfterContinue = llvm::BasicBlock::Create(*ctx, "after.continue", TheFunction);
    B.SetInsertPoint(AfterContinue);
    return;
  }
}

void CodeGen::emit(Program& p){
  // declare functions
  for (auto& fn : p.funcs){
    std::vector<llvm::Type*> params;
    for (auto& pr : fn->params) params.push_back(tyLLVM(*pr.ty));
    auto FT = llvm::FunctionType::get(tyLLVM(*fn->ret), params, false);
    auto F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, fn->name, mod.get());
    functions[fn->name]=F;

    // name params
    unsigned idx=0; for (auto &arg : F->args()) arg.setName(fn->params[idx++].name);
  }

  // define functions
  for (auto& fn : p.funcs){
    auto F = functions[fn->name];
    auto entry = llvm::BasicBlock::Create(*ctx, "entry", F);
    builder->SetInsertPoint(entry);
    namedValues.clear();
    namedTypes.clear();
    // allocate params on stack
    unsigned idx=0; for (auto &arg : F->args()){
      auto alloca = builder->CreateAlloca(arg.getType(), nullptr, arg.getName());
      builder->CreateStore(&arg, alloca);
      namedValues[std::string(arg.getName())]=alloca;
      namedTypes[std::string(arg.getName())]=arg.getType();
      idx++;
    }
    for (auto& st : fn->body) genStmt(*st, F);
    // Add implicit return if the current block has no terminator
    if (!builder->GetInsertBlock()->getTerminator()){
      if (fn->ret->k==TyKind::I64) builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*ctx), 0));
      else if (fn->ret->k==TyKind::Bool) builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt1Ty(*ctx), 0));
      else if (fn->ret->k==TyKind::Void) builder->CreateRetVoid();
      else builder->CreateRetVoid();
    }
    if (llvm::verifyFunction(*F, &llvm::errs())) fatal("invalid function IR");
  }
}

void CodeGen::writeIR(const std::string& path){
  std::error_code EC; llvm::raw_fd_ostream out(path, EC, llvm::sys::fs::OF_Text);
  if (EC) fatal("cannot write IR");
  mod->print(out, nullptr);
}

void CodeGen::writeObject(const std::string& path){
  llvm::InitializeNativeTarget(); llvm::InitializeNativeTargetAsmPrinter(); llvm::InitializeNativeTargetAsmParser();
  auto targetTriple = llvm::sys::getDefaultTargetTriple();
  mod->setTargetTriple(targetTriple);
  std::string Error; auto Target = llvm::TargetRegistry::lookupTarget(targetTriple, Error);
  if (!Target) fatal(Error);
  llvm::TargetOptions opt; auto RM = std::optional<llvm::Reloc::Model>();
  auto TM = std::unique_ptr<llvm::TargetMachine>(Target->createTargetMachine(targetTriple, "generic", "", opt, RM));
  mod->setDataLayout(TM->createDataLayout());

  std::error_code EC; llvm::raw_fd_ostream dest(path, EC, llvm::sys::fs::OF_None);
  if (EC) fatal("could not open obj file");
  llvm::legacy::PassManager pm;
  if (TM->addPassesToEmitFile(pm, dest, nullptr, llvm::CGFT_ObjectFile)) fatal("TargetMachine can't emit obj");
  pm.run(*mod); dest.flush();
}