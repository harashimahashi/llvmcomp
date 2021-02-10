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
        using ArgList = std::vector<std::unique_ptr<lexer::Token>>;

        class Node {

        public:

            virtual ~Node();
            virtual llvm::Value* compile() = 0;
        };

        class Expr : public Node {

        public:

            Expr(std::unique_ptr<lexer::Token>) noexcept;

            const std::unique_ptr<const lexer::Token> op_;
        };

        using ArrList = std::vector<std::unique_ptr<Expr>>;
        
        class Id : public Expr {

            llvm::Value* var_;

        protected:
        
            Id(std::unique_ptr<lexer::Token>, llvm::Value*);

        public:

            static std::shared_ptr<Id> 
                get_id(std::unique_ptr<lexer::Token>);
            llvm::Value* get_val() const;
            llvm::Value* compile() override;
        };

        class IArray {

        public:

            virtual llvm::Type* get_type() const = 0;
            virtual llvm::Align get_align() const = 0;

            static inline bool is_array(Expr const*);
            static inline const uint64_t kByteSize = 8;
        };

        class Array : public Id, public IArray {

            llvm::Align align_;

        protected:

            Array(std::unique_ptr<lexer::Token>,
                llvm::Value*, size_t, llvm::Align);

        public:

            static std::shared_ptr<Array> 
                get_array(std::unique_ptr<lexer::Token>, IndexList);
            llvm::Value* compile() override;
            llvm::Type* get_type() const override;
            llvm::Align get_align() const override;

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
            llvm::Value* compile() override;
        };

        class Unary : public Op {

            std::unique_ptr<Expr> exp_;

        public:

            Unary(std::unique_ptr<lexer::Token>, 
                std::unique_ptr<Expr>) noexcept;
            llvm::Value* compile() override;
        };

        class Access : public Op {

            llvm::Value* arr_;
            ValList args_;

        public:

            Access(Id*, ValList);
            llvm::Value* compile() override;
        };

        class Load : public Op {

            std::shared_ptr<Id> acc_;

        public:

            Load(std::shared_ptr<Id>) noexcept;
            llvm::Value* compile() override;
        };

        class ArrayLoad : public Op, public IArray {

            std::shared_ptr<Array> acc_;

        public:

            ArrayLoad(std::shared_ptr<Id>) noexcept;
            llvm::Value* compile() override;
            llvm::Type* get_type() const override;
            llvm::Align get_align() const override;
        };

        class Store : public Op {

            std::shared_ptr<Expr> acc_;
            std::unique_ptr<Expr> val_;

        public:

            Store(std::shared_ptr<Expr>, std::unique_ptr<Expr>) noexcept;
            llvm::Value* compile() override;
        };

        class Call : public Op {

            std::string name_;
            ArrList args_;

        public:

            Call(std::unique_ptr<lexer::Token>, ArrList);
            llvm::Value* compile() override;
        };

        class FConstant : public Expr {

        public:

            FConstant(std::unique_ptr<lexer::Token>) noexcept;
            llvm::Value* compile() override;
        };

        class ArrayConstant : public Expr, public IArray {
            
            static inline unsigned cnt_{0};
            llvm::Constant* carr_;
            llvm::Align align_;

        public: 

            ArrayConstant(ArrList);
            llvm::Value* compile() override;
            llvm::Type* get_type() const override;
            llvm::Align get_align() const override;
        };

        class Logical : public Expr {
            
        public:
        
            Logical(std::unique_ptr<lexer::Token>) noexcept;
        };

        class Bool : public Logical {

            std::unique_ptr<Expr> lhs_;
            std::unique_ptr<Expr> rhs_;

        public:

            Bool(std::unique_ptr<lexer::Token>, 
                std::unique_ptr<Expr>, std::unique_ptr<Expr>) noexcept;
            llvm::Value* compile() override;
        };

        class Not : public Logical {

            std::unique_ptr<Expr> exp_;

        public:

            Not(std::unique_ptr<lexer::Token>, 
                std::unique_ptr<Expr>) noexcept;
            llvm::Value* compile() override;
        };

        class Stmt : public Node {

            BBList compute_bb(unsigned);

        public:

            Stmt(unsigned = 0);

            const BBList List;
            static Stmt* enclosing;
        };

        class StmtSeq : public Stmt {

            std::unique_ptr<Stmt> stmt1_;
            std::unique_ptr<Stmt> stmt2_;

        public:

            StmtSeq(std::unique_ptr<Stmt>,
                std::unique_ptr<Stmt>);
            llvm::Value* compile() override;
        };

        class ExprStmt : public Stmt {

            std::unique_ptr<Expr> expr_;

        public:

            ExprStmt(std::unique_ptr<Expr> = nullptr);
            llvm::Value* compile() override;
        };

        class FunStmt : public Stmt {

            std::unique_ptr<Stmt> stmt_;

        public:

            FunStmt(std::unique_ptr<lexer::Token>, ArgList);
            void init(std::unique_ptr<Stmt>);
            llvm::Value* compile() override;
        }; 

        class IfElseBase : public Stmt {
            
            std::unique_ptr<Expr> expr_;
            std::unique_ptr<Stmt> stmt_;

        protected:

            void emit_if() const;
            virtual void emit_else() const = 0;

        public:

            IfElseBase(std::unique_ptr<Expr>,
                std::unique_ptr<Stmt>, unsigned);
            llvm::Value* compile() override;
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

            std::unique_ptr<Expr> expr_;
            std::unique_ptr<Stmt> stmt_;

        protected:

            virtual llvm::Value* emit_preloop() const;
            void emit_cond(llvm::BasicBlock* const,
                llvm::BasicBlock* const) const;
            void emit_body() const;
            virtual void emit_head(llvm::Value*) const = 0;

        public:

            LoopBase(unsigned);
            void init(std::unique_ptr<Expr>, std::unique_ptr<Stmt>);
            llvm::Value* compile() override;
        };

        class While : public LoopBase {

            static const unsigned num_blocks_ = 3;

        protected:

            void emit_head(llvm::Value*) const override;

        public:

            While();
            void init(std::unique_ptr<Expr>, std::unique_ptr<Stmt>);
            llvm::Value* compile() override;
        };

        class RepeatUntil : public LoopBase {

            static const unsigned num_blocks_ = 2;

        protected:

            void emit_head(llvm::Value*) const override;

        public:

            RepeatUntil();
            void init(std::unique_ptr<Expr>, std::unique_ptr<Stmt>);
            llvm::Value* compile() override;
        };

        class For : public LoopBase {

            static const unsigned num_blocks_ = 3;
            std::unique_ptr<Stmt> stmt_;
            bool to_downto_{ true };

        protected:

            llvm::Value* emit_preloop() const override;
            void emit_head(llvm::Value*) const override;

        public:

            For();
            void init(std::unique_ptr<Expr>,
                std::unique_ptr<Stmt>, std::unique_ptr<Stmt>);
            llvm::Value* compile() override;

            void set_to();
            void set_downto();
        };

        class Break : public Stmt {

            Stmt* stmt_;

        public:

            Break();
            llvm::Value* compile() override;
        };

        class Return : public Stmt {

            std::unique_ptr<Expr> expr_;

        public:

            Return(std::unique_ptr<Expr>);
            llvm::Value* compile() override;
        };
    }
}
#endif