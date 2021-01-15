#ifndef LLVMC_IINTER_H_
#define LLVMC_IINTER_H_
#include <llvmc/ilex.h>
#include "llvm/IR/Value.h"

namespace llvmc {

    namespace inter {

        using BBList = llvm::SmallVector<llvm::BasicBlock*, 4>;
        using ValList = llvm::SmallVector<llvm::Value*, 8>;

        class Node {

        public:

            virtual ~Node();
            virtual llvm::Value* compile() const = 0;
        };

        class Expr : public Node {

        public:

            Expr(std::unique_ptr<lexer::Token>) noexcept;

            const std::unique_ptr<const lexer::Token> op_;
        };

        class Id : public Expr {

            llvm::Value* var_;

        public:

            Id(std::unique_ptr<lexer::Token>, llvm::Value*);
            llvm::Value* compile() const override;
        };

        class Array : public Id {

        public:

            Array(std::unique_ptr<lexer::Token>, llvm::Value*, unsigned);
            llvm::Value* compile() const override;

            const unsigned dim_;
        };

        class Op : public Expr {

        public:

            Op(std::unique_ptr<lexer::Token>) noexcept;
        };

        class Arith : public Op {

            std::unique_ptr<Expr> lhs_, rhs_;

        public:

            Arith(std::unique_ptr<lexer::Token>, 
                std::unique_ptr<Expr>, std::unique_ptr<Expr>) noexcept;
            llvm::Value* compile() const override;
        };

        class Unary : public Op {

            std::unique_ptr<Expr> exp_;

        public:

            Unary(std::unique_ptr<lexer::Token>, 
                std::unique_ptr<Expr>) noexcept;
            llvm::Value* compile() const override;
        };

        class Access : public Op {

            llvm::Value* arr_;
            ValList args_;

        public:

            Access(Id*, ValList);
            llvm::Value* compile() const override;
        };

        class Constant : public Expr {

        public:

            Constant(double) noexcept;
            llvm::Value* compile() const override;
        };

        class Logical : public Expr {
            
        public:
        
            const std::unique_ptr<const Expr> lhs_, rhs_;
            Logical(std::unique_ptr<lexer::Token>, 
                std::unique_ptr<Expr>, std::unique_ptr<Expr>) noexcept;
        };

        class Bool : public Logical {

        public:

            Bool(std::unique_ptr<lexer::Token>, 
                std::unique_ptr<Expr>, std::unique_ptr<Expr>) noexcept;
            llvm::Value* compile() const override;
        };

        class Not : public Logical {

        public:

            Not(std::unique_ptr<lexer::Token>, 
                std::unique_ptr<Expr>) noexcept;
            llvm::Value* compile() const override;
        };

        class Stmt : public Node {};

        class IfElse : public Stmt {
            
            std::unique_ptr<Expr> expr_;
            std::unique_ptr<Stmt> stmt1_;
            std::unique_ptr<Stmt> stmt2_;

        public:

            IfElse(std::unique_ptr<Expr>, std::unique_ptr<Stmt>,
                                std::unique_ptr<Stmt>) noexcept;
            llvm::Value* compile() const override;
        };
    }
}
#endif