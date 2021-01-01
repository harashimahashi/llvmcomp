/* #ifndef LLVMC_IINTER_H_
#define LLVMC_IINTER_H_
#include "ilex.h"
#include "llvm/IR/Value.h"

namespace inter {

class Node {

    void error(std::string);

public:

    Node() noexcept;
    virtual ~Node();
    virtual llvm::Value* compile() = 0;

};

class Expr : public Node {};

class Id : public Expr {

    std::string name_;

public:

    Id(lexer::Word);
    llvm::Value* compile() override;
};

class Op : public Expr {

public:

    Op(lexer::Token) noexcept;
};

class Arith : public Op {

    std::unique_ptr<Expr> lhs_, rhs_;

public:

    Arith(lexer::Token, std::unique_ptr<Expr>, std::unique_ptr<Expr>) noexcept;
    llvm::Value* compile() override;
};

class Unary : public Op {

    std::unique_ptr<Expr> exp_;

public:

    Unary(lexer::Token, std::unique_ptr<Expr>) noexcept;
    llvm::Value* compile() override;
};

class Constant : public Expr {

public:

    Constant(double) noexcept;
    llvm::Value* compile() override;
};

}
#endif */