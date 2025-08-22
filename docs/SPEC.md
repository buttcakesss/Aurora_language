Aurora Spec (v0.1)

Design goals
- Manual memory like C/C++ with RAII-style cleanup (defer/unique<T> today; full destructors next)
- Readability like Python (minimal punctuation, block bracing but concise)
- Static typing with local inference (Rust-like 'let' inference)
- Competitive programming: tiny runtime, instant builds, direct x86-64

Types
- i64, i32, bool
- ptr<T>, unique<T>
- Functions are first-class (surface: user declares; backend lowers to LLVM function)

Bindings
let name[: Type] = expr;

Control
if (e) { ... } else { ... }
while (e) { ... }
return e;
defer expr;  // executed on scope exit, LIFO

Memory
- malloc(size: i64) -> ptr<i8>
- free(p: ptr<i8>)
- unique<T> variable injects implicit 'defer free(var)' upon initialization (MVP sugar)

Semantics
- '=' assigns; types must match
- integers are signed; division is truncating
- nonzero is true; zero false (codegen normalizes where needed)

Compilation pipeline
- Lex/Parse -> AST
- Semantic analysis: scope, symbol table, type inference for locals, check returns
- IR Generation: LLVM IR
- Codegen: TargetMachine -> ELF/COFF object; link with clang + tiny C runtime

