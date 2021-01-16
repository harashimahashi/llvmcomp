#include <llvmc/iinter.h>
#include <llvmc/iparser.h>

namespace {

    bool is_array(llvm::Value* V) {

        if(auto A = llvm::dyn_cast<llvm::AllocaInst>(V); A) {

            if(llvm::isa<llvm::ArrayType>(A->getAllocatedType())) 
                return true;

            return false;
        }

        return false;
    }

    llvm::Value* LogErrorV(std::string s) {
        
        llvm::errs() << "Compile error:" << 
            llvmc::lexer::Lexer::line_ << ": " << s << '\n';

        return nullptr;
    }
}

namespace llvmc {

    namespace inter {

        using namespace llvm;
        using namespace lexer;
        using namespace parser;

        Node::~Node() = default;

        Expr::Expr(std::unique_ptr<Token> t) noexcept : op_{ std::move(t) } {}

        Id::Id(std::unique_ptr<Token> t, Value* v) 
            : Expr{ std::move(t) }, var_{ v } {}
        Value* Id::compile() const {

            return var_;
        }

        Array::Array(std::unique_ptr<lexer::Token> t, Value* v, unsigned u) 
            : Id{ std::move(t), v }, dim_{ u }  {}
        Value* Array::compile() const {

            return Id::compile();
        }

        Op::Op(std::unique_ptr<Token> t) noexcept : Expr{ std::move(t) } {}

        Arith::Arith(std::unique_ptr<Token> t, std::unique_ptr<Expr> e1,
            std::unique_ptr<Expr> e2) noexcept : Op{ std::move(t) },
            lhs_{ std::move(e1) }, rhs_{ std::move(e2) } {}
        Value* Arith::compile() const {

            Value* L = lhs_->compile();
            Value* R = rhs_->compile();

            if(!is_array(L) && !is_array(R)) {

                switch(*op_) {
                    
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
            
            return LogErrorV("invalid operand type");
        }

        Unary::Unary(std::unique_ptr<Token> t, std::unique_ptr<Expr> e) noexcept
            : Op{ std::move(t) }, exp_{ std::move(e) } {}
        Value* Unary::compile() const {

            Value* E = exp_->compile();

            if(!is_array(E)) {
                return Parser::Builder.CreateFNeg(E, "subneg");
            }

            return LogErrorV("invalid operand type");
        }

        Access::Access(Id* id, ValList vec = {}) : Op{ nullptr }, 
            arr_{ id->compile() } {

            if(auto I = dynamic_cast<Array*>(id); I) {
                
                if(!vec.size()) {
                    for(unsigned i = 0; i < I->dim_ + 1; i++) 
                        args_.emplace_back(Parser::Builder.getInt32(0));
                }
                else
                    args_ = std::move(vec);

            }
        }

        Value* Access::compile() const {
            
            return Parser::Builder.CreateGEP(arr_, args_);
        }

        Constant::Constant(double d) noexcept : Expr{ std::make_unique<Num>(d) } {}
        Value* Constant::compile() const {

            return ConstantFP::get(Parser::Context, 
                APFloat(static_cast<double>(*dynamic_cast<const Num*>(op_.get()))));
        }

        Logical::Logical(std::unique_ptr<Token> t, std::unique_ptr<Expr> e1,
            std::unique_ptr<Expr> e2) noexcept : Expr{ std::move(t) },
            lhs_{ std::move(e1) }, rhs_{ std::move(e2) } {}

        Bool::Bool(std::unique_ptr<Token> t, std::unique_ptr<Expr> e1, 
            std::unique_ptr<Expr> e2) noexcept : Logical{ std::move(t),
            std::move(e1), std::move(e2) } {}
        Value* Bool::compile() const {

            Value* L = lhs_->compile();
            Value* R = rhs_->compile();

            if(!is_array(L) && !is_array(R)) {

                if(auto W = dynamic_cast<const Word*>(op_.get()); W) {

                    if(*W == Word::Or) 
                        L = Parser::Builder.CreateOr(L, R, "subor");
                    else if(*W == Word::And) 
                        L = Parser::Builder.CreateAnd(L, R, "suband");
                    else if(*W == Word::le) 
                        L = Parser::Builder.CreateFCmpULE(L, R, "subcmp");
                    else if(*W == Word::ge) 
                        L = Parser::Builder.CreateFCmpUGE(L, R, "subcmp");
                    else if(*W == Word::eq) 
                        L = Parser::Builder.CreateFCmpUEQ(L, R, "subcmp");
                    else if(*W == Word::ne) 
                        L = Parser::Builder.CreateFCmpUNE(L, R, "subcmp");

                    return Parser::Builder.CreateUIToFP(L,
                        Parser::Builder.getDoubleTy(), "booltmp");
                }
                else {

                    switch(*op_) {

                        case '<':
                            L = Parser::Builder.CreateFCmpULT(L, R, "subcmp");
                            break;
                        case '>':
                            L = Parser::Builder.CreateFCmpUGT(L, R, "subcmp");
                            break;
                    }

                    return Parser::Builder.CreateUIToFP(L,
                        Parser::Builder.getDoubleTy(), "booltmp");
                }
            }

            return LogErrorV("invalid operand type");
        }

        Not::Not(std::unique_ptr<Token> t, std::unique_ptr<Expr> e1) noexcept 
            : Logical{ std::move(t), std::move(e1), nullptr } {}
        Value* Not::compile() const {

            Value* E = lhs_->compile();

            if(!is_array(E)) {

                E = Parser::Builder.CreateNot(E, "subnot");

                return Parser::Builder.CreateUIToFP(E,
                        Parser::Builder.getDoubleTy(), "booltmp");
            }

            return LogErrorV("invalid operand type");
        }

        BBList Stmt::compute_bb() {
            
            BBList temp{};

            if(expr_) {

                Function* par = Parser::Builder.GetInsertBlock()->getParent();
                unsigned blocks = stmt2_ ? 3 : 2;

                for(unsigned i = 0; i < blocks; i++) {
                    temp.emplace_back(BasicBlock::Create(Parser::Context, "", par));
                }
            }

            return temp;
        }
        Stmt::Stmt(std::unique_ptr<Stmt> s1, std::unique_ptr<Expr> e = nullptr,
            std::unique_ptr<Stmt> s2 = nullptr) : stmt1_{ std::move(s1) },
            expr_{ std::move(e) }, stmt2_{ std::move(s2) }, List{ compute_bb() } {}
        Stmt::Stmt const* const empty = nullptr;

        IfElse::IfElse(std::unique_ptr<Stmt> s1,
            std::unique_ptr<Expr> e = nullptr, std::unique_ptr<Stmt> s2 = nullptr)
            : Stmt{ std::move(s1), std::move(e), std::move(s2)} {}
        Value* IfElse::compile() const {

            Value* E = Parser::Builder.CreateFPToUI(
                expr_->compile(), Parser::Builder.getInt1Ty());
            
            Parser::Builder.CreateCondBr(E, List[0], List[1]);

            Parser::Builder.SetInsertPoint(List[0]);
            stmt1_->compile();
            if(stmt2_) {

                Parser::Builder.CreateBr(List[1]);
                Parser::Builder.SetInsertPoint(List[1]);
            }
            else {

                Parser::Builder.SetInsertPoint(List[1]);
                stmt2_->compile();
                Parser::Builder.CreateBr(List[2]);
                Parser::Builder.SetInsertPoint(List[2]);
            }
            
            return List[2];
        }
    }
}