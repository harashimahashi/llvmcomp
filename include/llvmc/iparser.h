#ifndef LLVMC_IPARSER_H_
#define LLVMC_IPARSER_H_
#include <llvmc/ilex.h>
#include <llvmc/isymbols.h>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

namespace llvmc {

    namespace parser {

        class Parser {

        static inline unsigned err_num_ = 0;
        static inline unsigned ret_num_ = 0;

        std::string path_;
        lexer::Lexer lex_;
        std::unique_ptr<lexer::Token> tok_;
        class EnvGuard;

        std::string get_output_name() const;
        void check_end();
        void move();
        std::unique_ptr<lexer::Token> match(lexer::Tag);
        void program_preinit();
        void program_postinit();
        void fun_stmts();
        void fun_def();
        std::unique_ptr<inter::Expr> fun_call();
        std::unique_ptr<inter::Stmt> stmts();
        std::unique_ptr<inter::Stmt> stmt();
        std::unique_ptr<inter::Stmt> decls();
        std::unique_ptr<inter::Stmt> assign();
        std::unique_ptr<inter::Expr> pbool();
        std::unique_ptr<inter::Expr> join();
        std::unique_ptr<inter::Expr> equality();
        std::unique_ptr<inter::Expr> rel();
        std::unique_ptr<inter::Expr> expr();
        std::unique_ptr<inter::Expr> term();
        std::unique_ptr<inter::Expr> unary();
        std::unique_ptr<inter::Expr> factor();
        std::unique_ptr<inter::Expr> access(
            std::shared_ptr<inter::Id>);
        inter::ArrList expr_seq();
        
        public:

            static inline llvm::LLVMContext Context;
            static inline llvm::IRBuilder Builder{ Context };
            static inline std::unique_ptr<llvm::Module> Module{ 
                std::make_unique<llvm::Module>("module", Context) };
            static inline llvm::DataLayout layout{ Module.get() };
            static inline std::shared_ptr<symbols::Env> top = nullptr;

            Parser(lexer::Lexer, std::string);

            void program();
            
            static std::nullptr_t LogErrorV(std::string);
        };
    }
}
#endif