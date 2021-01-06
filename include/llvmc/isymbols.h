#ifndef LLVMC_ISYMBOLS_H_
#define LLVMC_ISYMBOLS_H_
#include <llvmc/ilex.h>
#include "llvm/IR/Value.h"

namespace symbols {

class Env {

    std::unordered_map<std::unique_ptr<lexer::Word>, llvm::Value*> table_;

protected:

    std::unique_ptr<Env> prev_;

public:

    Env(Env*);

    void insert(std::unique_ptr<lexer::Word>, llvm::Value*);
    llvm::Value* get(std::unique_ptr<lexer::Word>);

};

}
#endif