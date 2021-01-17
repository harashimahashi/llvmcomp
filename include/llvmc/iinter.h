#ifndef LLVMC_IINTER_H_
#define LLVMC_IINTER_H_
#include <llvmc/ilex.h>
#include "llvm/IR/Value.h"
#include "llvm/ADT/SmallVector.h"

namespace llvmc {

    namespace inter {

        using BBList = llvm::SmallVector<llvm::BasicBlock*, 4>;
        using IndexList = llvm::SmallVector<uint64_t, 8>;
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

        protected:
        
            Id(std::unique_ptr<lexer::Token>, llvm::Value*);

        public:

            static std::unique_ptr<Id> 
                get_id(std::unique_ptr<lexer::Token>);
            llvm::Value* compile() const override;
        };

        class Array : public Id {

        protected:

            Array(std::unique_ptr<lexer::Token>, llvm::Value*, size_t);

        public:

            static std::unique_ptr<Array> 
                get_array(std::unique_ptr<lexer::Token>, IndexList);
            llvm::Value* compile() const override;

            const size_t dim_;
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

        class Load : public Op {

            std::unique_ptr<Expr> acc_;

        public:

            Load(std::unique_ptr<Expr>) noexcept;
            llvm::Value* compile() const override;
        };

        class Store : public Op {

            std::unique_ptr<Expr> acc_;
            std::unique_ptr<Expr> val_;

        public:

            Store(std::unique_ptr<Expr>, std::unique_ptr<Expr>) noexcept;
            llvm::Value* compile() const override;
        };

        class Constant : public Expr {

        public:

            Constant(double) noexcept;
            llvm::Value* compile() const override;
        };

        class Logical : public Expr {
            
        public:
        
            Logical(std::unique_ptr<lexer::Token>, 
                std::unique_ptr<Expr>, std::unique_ptr<Expr>) noexcept;

            const std::unique_ptr<const Expr> lhs_, rhs_;
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

        class Stmt : public Node {

            BBList compute_bb(unsigned);
            std::unique_ptr<Expr> expr_;

        public:

            Stmt(std::unique_ptr<Expr> = nullptr, unsigned = 0);
            llvm::Value* compile() const override;

            const BBList List;
            static const Stmt empty;
        };

        class IfElseBase : public Stmt {

            std::unique_ptr<Stmt> stmt_;

        protected:

            void emit_if() const;
            virtual void emit_else() const = 0;

        public:

            IfElseBase(std::unique_ptr<Expr>,
                std::unique_ptr<Stmt>, unsigned);
            llvm::Value* compile() const override;
        };

        class If : public IfElseBase {

            static const unsigned num_blocks_ = 2;

        protected:

            void emit_else() const override;

        public:

            If(std::unique_ptr<Expr>, 
                std::unique_ptr<Stmt>);
        };

        class IfElse : public IfElseBase {

            static const unsigned num_blocks_ = 3;
            std::unique_ptr<Stmt> stmt_;

        protected: 

            void emit_else() const override;

        public:

            IfElse(std::unique_ptr<Expr>, 
                std::unique_ptr<Stmt>, std::unique_ptr<Stmt>);
        };

        class LoopBase : public Stmt {

            std::unique_ptr<Stmt> stmt_;

        protected:

            virtual llvm::Value* emit_preloop() const;
            void emit_cond(llvm::BasicBlock* const,
                llvm::BasicBlock* const) const;
            void emit_body() const;
            virtual void emit_head(llvm::Value*) const = 0;

        public:

            LoopBase(std::unique_ptr<Expr>, 
                std::unique_ptr<Stmt>, unsigned);
            llvm::Value* compile() const override;
        };

        class While : public LoopBase {

            static const unsigned num_blocks_ = 3;

        protected:

            void emit_head(llvm::Value*) const override;

        public:

            While(std::unique_ptr<Expr>, std::unique_ptr<Stmt>);
            llvm::Value* compile() const override;
        };

        class RepeatUntil : public LoopBase {

            static const unsigned num_blocks_ = 2;

        protected:

            void emit_head(llvm::Value*) const override;

        public:

            RepeatUntil(std::unique_ptr<Expr>, std::unique_ptr<Stmt>);
            llvm::Value* compile() const override;
        };

        class For : public LoopBase {

            static const unsigned num_blocks_ = 3;
            std::unique_ptr<Stmt> stmt_;
            bool to_downto_{ true };

        protected:

            llvm::Value* emit_preloop() const override;
            void emit_head(llvm::Value*) const override;

        public:

            For(std::unique_ptr<Expr>, 
                std::unique_ptr<Stmt>, std::unique_ptr<Stmt>);
            llvm::Value* compile() const override;

            void set_to();
            void set_downto();
        };
    }
}
#endif