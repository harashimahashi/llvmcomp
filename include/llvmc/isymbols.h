#ifndef LLVMC_ISYMBOLS_H_
#define LLVMC_ISYMBOLS_H_
#include "ilex.h"
#include "llvm/IR/Value.h"

namespace symbols {

class Env {

    std::unordered_map<lexer::Token, llvm::Value*> table_;

protected:

    std::unique_ptr<Env> prev_;

public:

    Env(Env*);

    void insert(lexer::Token, llvm::Value*);
    llvm::Value* get(lexer::Token);

};

}
#endif