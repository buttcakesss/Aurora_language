// codegen.h
#pragma once
#include "ast.h"
#include "types.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace llvm { class LLVMContext; class Module; class IRBuilderBase; class Value; class Function; class TargetMachine; class Type; class BasicBlock; }

struct CodeGen {
  std::unique_ptr<llvm::LLVMContext> ctx;
  std::unique_ptr<llvm::Module> mod;
  std::unique_ptr<llvm::IRBuilderBase> builder; // we’ll actually use IRBuilder<>

  std::unordered_map<std::string, llvm::Value*> namedValues;
  std::unordered_map<std::string, llvm::Type*> namedTypes; // Track variable types for LLVM 17+
  std::unordered_map<std::string, llvm::Function*> functions;
  
  // Stack of loop exit blocks for break/continue
  std::vector<llvm::BasicBlock*> loopExitStack;
  std::vector<llvm::BasicBlock*> loopContinueStack;

  CodeGen(const std::string& moduleName);
  ~CodeGen();  // Destructor needed for unique_ptr with forward declarations
  void emit(Program& p);
  void writeObject(const std::string& path);
  void writeIR(const std::string& path);

private:
  llvm::Value* genExpr(Expr& e);
  void genStmt(Stmt& s, llvm::Function* fn);
  void runDefers(std::vector<Expr*>& defers);
  llvm::Function* declareBuiltin(const char* name, std::vector<llvm::Type*> params, llvm::Type* ret, bool vararg=false);
  llvm::Type* tyLLVM(const Type& t);
};