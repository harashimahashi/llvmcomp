#ifndef LLVMC_IPARSER_H_
#define LLVMC_IPARSER_H_
#include <llvmc/ilex.h>
#include <llvmc/isymbols.h>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

namespace llvmc {

    namespace parser {

        class Parser {

        lexer::Lexer lex_;
        std::unique_ptr<lexer::Token> tok_;
        class EnvGuard;

        auto LogErrorV(std::string);
        void move();
        auto match(lexer::Tag);
        void program_preinit();
        void fun_stmts();
        void fun_def();
        void fun_call();
        
        public:

            static inline llvm::LLVMContext Context;
            static inline llvm::IRBuilder Builder{ Context };
            static inline std::unique_ptr<llvm::Module> Module{ 
                std::make_unique<llvm::Module>("module", Context) };
            static inline llvm::DataLayout layout{ Module.get() };
            static inline std::shared_ptr<symbols::Env> top = nullptr;

            Parser(lexer::Lexer);

            void program();
        };
    }
}
#endif