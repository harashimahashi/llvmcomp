#ifndef LLVMC_IINTER_H_
#define LLVMC_IINTER_H_
#include <llvmc/ilex.h>
#include <llvmc/isymbols.h>
#include "llvm/IR/Value.h"

namespace llvmc {

    namespace inter {

        class Node {

            void error(std::string);

        public:

            Node() noexcept;
            virtual ~Node();
            virtual llvm::Value* compile() = 0;

        };

        class Expr : public Node {

        protected:

            std::unique_ptr<lexer::Token> op_;

        public:

            Expr(std::unique_ptr<lexer::Token>) noexcept;
        };

        class Id : public Expr {
            
            symbols::Type type_;
            llvm::Value* val_;

        public:

            Id(llvm::Value*, symbols::Type);
        };

        class Op : public Expr {

        public:

            Op(std::unique_ptr<lexer::Token>) noexcept;
        };

        class Arith : public Op {

            std::unique_ptr<Expr> lhs_, rhs_;

        public:

            Arith(std::unique_ptr<lexer::Token>, std::unique_ptr<Expr>, std::unique_ptr<Expr>) noexcept;
            llvm::Value* compile() override;
        };

        class Unary : public Op {

            std::unique_ptr<Expr> exp_;

        public:

            Unary(std::unique_ptr<lexer::Token>, std::unique_ptr<Expr>) noexcept;
            llvm::Value* compile() override;
        };

        /* class Constant : public Expr {

        public:

            Constant(double) noexcept;
            llvm::Value* compile() override;
        }; */
    }
}
#endif