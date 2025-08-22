#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include "codegen.h"
#include "diagnostics.h"
#include "util.h"
#include <string>
#include <vector>
#include <iostream>

int main(int argc, char** argv){
  if (argc < 3){
    std::cerr << "usage: aurorac <input.aur> -o <out.o> [--emit-ll out.ll]\n";
    return 1;
  }
  std::string in = argv[1];
  std::string outObj, outLL;
  for (int i=2;i<argc;i++){
    std::string a = argv[i];
    if (a=="-o" && i+1<argc) outObj = argv[++i];
    else if (a=="--emit-ll" && i+1<argc) outLL = argv[++i];
  }
  if (outObj.empty()) fatal("missing -o <file.o>");

  auto src = slurp(in);
  Lexer lx(src);
  auto toks = lx.lex();
  Parser ps(std::move(toks));
  auto prog = ps.parseProgram();

  Sema sema; sema.analyze(*prog);
  CodeGen cg("aurora_module");
  cg.emit(*prog);
  if (!outLL.empty()) cg.writeIR(outLL);
  cg.writeObject(outObj);
  return 0;
}
