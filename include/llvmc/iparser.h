/* #ifndef LLVMC_IPARSER_H_
#define LLVMC_IPARSER_H_
#include "isymbols.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

namespace parser {

    class Parser {

    lexer::Lexer lex;
    lexer::Token tok;
    
    public:

        static inline llvm::LLVMContext Context;
        static inline llvm::IRBuilder<> Builder{ Context };
        static inline std::unique_ptr<llvm::Module> Module;
        static inline symbols::Env* top = nullptr;

    };

}

#endif */