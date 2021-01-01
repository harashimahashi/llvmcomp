#include "iinter.h"
#include "iparser.h"
/*
namespace inter {

    using namespace llvm;
    using namespace lexer;
    using namespace parser;

    Node::~Node() = default;

    Id::Id(Word w) : Expr{}, name_{ w.lexeme_ } {}
    Value* Id::compile() {

        Value* V = Parser::top->get(op_);
        return V;
    }

    Op::Op(Token t) noexcept : Expr{ t } {}

    Arith::Arith(lexer::Token t, std::unique_ptr<Expr> e1, std::unique_ptr<Expr> e2) noexcept : Op{ t }, lhs_{ std::move(e1) }, rhs_{ std::move(e2) } {}
    Value* Arith::compile() {

        Value* L = lhs_->compile();
        Value* R = rhs_->compile();

        switch(op_) {
            case '+':
                return Parser::Builder.CreateFAdd(L, R, "addtmp");
            case '-':
                return Parser::Builder.CreateFSub(L, R, "subtmp");
            case '*':
                return Parser::Builder.CreateFMul(L, R, "multmp");
            case '/':
                return Parser::Builder.CreateFDiv(L, R, "muldiv");
        }
    }

    Unary::Unary(Token t, std::unique_ptr<Expr> e) noexcept : Op{ t }, exp_{ std::move(e) } {}
    Value* Unary::compile() {

        Value* E = exp_->compile();

        return Parser::Builder.CreateFNeg(E, "subneg");
    }

    Constant::Constant(double i) noexcept : Expr{ Num{ i } } {}
    Value* Constant::compile() {

        return ConstantFP::get(Parser::Context, APFloat(op_));
    }
} */