Aurora v0.1 — a tiny compiled language for CP

Build
-----
See top-level CMake instructions.

Compile & run
-------------
./build/aurorac examples/hello.aur -o build/hello.o --emit-ll build/hello.ll
clang -no-pie build/hello.o build/stdlib/aurora_runtime.o -o build/hello
./build/hello

Language
--------
- let with type inference (locals)
- i64/i32/bool/ptr<T>, unique<T> (RAII sugar)
- if/while/return/defer
- user/builtin calls (print_i64, read_i64, malloc, free)
- Expressions with + - * / % && || ! and comparisons

Roadmap
-------
- Structs/classes with deterministic destructors (RAII proper)
- User generics with monomorphization
- Arrays/slices & fast I/O helpers
- SSA-based inliner and constfold (LLVM Passes)
# Aurora
