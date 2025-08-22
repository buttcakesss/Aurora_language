#include <iostream>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

int main() {
    llvm::LLVMContext ctx;
    llvm::Module mod("test", ctx);
    llvm::IRBuilder<> builder(ctx);
    
    // Create a function
    auto funcType = llvm::FunctionType::get(llvm::Type::getInt64Ty(ctx), false);
    auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", &mod);
    auto entry = llvm::BasicBlock::Create(ctx, "entry", func);
    builder.SetInsertPoint(entry);
    
    // Create an array alloca
    auto arrayType = llvm::ArrayType::get(llvm::Type::getInt64Ty(ctx), 5);
    auto alloca = builder.CreateAlloca(arrayType, nullptr, "arr");
    
    // Show what alloca looks like
    llvm::errs() << "Alloca: ";
    alloca->print(llvm::errs());
    llvm::errs() << "\n";
    llvm::errs() << "Alloca type: ";
    alloca->getType()->print(llvm::errs());
    llvm::errs() << "\n";
    
    // Create GEP
    auto zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), 0);
    auto two = llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), 2);
    auto gep = builder.CreateInBoundsGEP(arrayType, alloca, {zero, two});
    
    llvm::errs() << "GEP: ";
    gep->print(llvm::errs());
    llvm::errs() << "\n";
    
    // Load from GEP
    auto load = builder.CreateLoad(llvm::Type::getInt64Ty(ctx), gep);
    
    builder.CreateRet(load);
    
    // Print the module
    mod.print(llvm::errs(), nullptr);
    
    return 0;
}