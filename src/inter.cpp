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

        DataLayout Node::layout{ Parser::Module.get() };
        Node::~Node() = default;

        Expr::Expr(std::unique_ptr<Token> t) noexcept : op_{ std::move(t) } {}

        Id::Id(std::unique_ptr<Token> t, Value* v) 
            : Expr{ std::move(t) }, var_{ v } {}
        std::unique_ptr<Id> Id::get_id(std::unique_ptr<Token> t) {

            Value* V = Parser::Builder.CreateAlloca(
                        Parser::Builder.getDoubleTy(), nullptr);

            return std::unique_ptr<Id>{ new Id{ std::move(t), V } };
        }
        Value* Id::compile() const {

            return var_;
        }

        Array::Array(std::unique_ptr<lexer::Token> t, Value* v, size_t u) 
            : Id{ std::move(t), v }, dim_{ u }  {}
        std::unique_ptr<Array> Array::get_array(
            std::unique_ptr<lexer::Token> t, IndexList L) {

            size_t sz = L.size();
            Type* T = ArrayType::get(
                        Parser::Builder.getDoubleTy(), L.pop_back_val());

            while(L.size()) {

                T = ArrayType::get(T, L.pop_back_val());
            }

            Value* V = Parser::Builder.CreateAlloca(T, nullptr);

            return std::unique_ptr<Array>{ new Array{ std::move(t), V, sz} };
        }
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

            if(auto I = dynamic_cast<Array*>(id)) {
                
                if(!vec.size()) {
                    for(size_t i = 0; i < I->dim_ + 1; i++) 
                        args_.emplace_back(Parser::Builder.getInt32(0));
                }
                else
                    args_ = std::move(vec);

            }
        }
        Value* Access::compile() const {
            
            return Parser::Builder.CreateGEP(arr_, args_);
        }

        Load::Load(std::shared_ptr<Expr> e) noexcept 
            : Op{ nullptr }, acc_{ e } {}
        Value* Load::compile() const {

            return Parser::Builder.CreateLoad(acc_->compile());
        }

        Store::Store(std::shared_ptr<Expr> e, std::unique_ptr<Expr> s) noexcept
            : Op{ nullptr }, acc_{ e }, val_{ std::move(s) } {}
        Value* Store::compile() const {

            Value* V = acc_->compile();
            Parser::Builder.CreateStore(val_->compile(), V);

            return V;
        }

        Constant::Constant(std::unique_ptr<Token> t) noexcept : Expr{ std::move(t) } {}
        Value* Constant::compile() const {

            return ConstantFP::get(Parser::Context, 
                APFloat(static_cast<double>(*dynamic_cast<const Num*>(op_.get()))));
        }

        ArrayConstant::ArrayConstant(ArrList lst) : Expr{ nullptr } {

            SmallVector<llvm::Constant*, 16> carr;

            if(auto A = dynamic_cast<ArrayConstant*>(lst.begin()->get())) {

                std::transform(lst.begin(), lst.end(),
                    carr.begin(), [](auto const& el) {
                        return dynamic_cast<const ArrayConstant*>(el.get())->carr_;
                });

                carr_ = ConstantArray::get(
                        ArrayType::get(A->carr_->getType(), lst.size()), carr);
            }
            else {

                std::transform(lst.begin(), lst.end(),
                    carr.begin(), [](auto const& el) {
                        return cast<llvm::Constant>(el.get()->compile());
                    });
            }
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

        BBList Stmt::compute_bb(unsigned cnt) {
            
            BBList temp{};
            Function* par = Parser::Builder.GetInsertBlock()->getParent();

            for(unsigned i = 0; i < cnt; i++) {
                temp.emplace_back(BasicBlock::Create(Parser::Context, "", par));
            }

            return temp;
        }
        Stmt::Stmt(unsigned cnt) : List{ compute_bb(cnt) } {}
        const Stmt& Stmt::empty = ExprStmt{};
        Stmt* Stmt::enclosing = nullptr;

        ExprStmt::ExprStmt(std::unique_ptr<Expr> e, unsigned u) 
            : Stmt{ u }, expr_{ std::move(e) } {}
        Value* ExprStmt::compile() const {

            if(expr_) 
                return expr_->compile();

            return nullptr;
        }

        IfElseBase::IfElseBase(std::unique_ptr<Expr> e, 
            std::unique_ptr<Stmt> s, unsigned cnt) 
            : ExprStmt{ std::move(e), cnt }, stmt_{ std::move(s) } {}
        void IfElseBase::emit_if() const {
            
            Value* E = Parser::Builder.CreateFPToUI(
                ExprStmt::compile(), Parser::Builder.getInt1Ty());

            Parser::Builder.CreateCondBr(E, List[0], List[1]);

            Parser::Builder.SetInsertPoint(List[0]);
            stmt_->compile();

            Parser::Builder.CreateBr(List.back());
        }
        llvm::Value* IfElseBase::compile() const { 

            emit_if();
            emit_else();

            Parser::Builder.SetInsertPoint(List.back());

            return nullptr;
        }

        If::If(std::unique_ptr<Expr> e, std::unique_ptr<Stmt> s)
            : IfElseBase{ std::move(e), std::move(s), num_blocks_ } {}
        void If::emit_else() const {}

        IfElse::IfElse(std::unique_ptr<Expr> e, 
            std::unique_ptr<Stmt> s1, std::unique_ptr<Stmt> s2) 
            : IfElseBase{ std::move(e), std::move(s1), num_blocks_ },
            stmt_{ std::move(s2) } {}
        void IfElse::emit_else() const {

            Parser::Builder.SetInsertPoint(List[1]);
            stmt_->compile();

            Parser::Builder.CreateBr(List.back());
        }

        LoopBase::LoopBase(std::unique_ptr<Expr> e,
            std::unique_ptr<Stmt> s, unsigned cnt) 
            : ExprStmt{ std::move(e), cnt } {}
        Value* LoopBase::emit_preloop() const {

            return nullptr;
        }
        void LoopBase::emit_cond(
            llvm::BasicBlock* const b1, llvm::BasicBlock* const b2) const {

            Value* E = Parser::Builder.CreateFPToUI(
                Stmt::compile(), Parser::Builder.getInt1Ty());

            Parser::Builder.CreateCondBr(E, b1, b2);
        }
        void LoopBase::emit_body() const {

            stmt_->compile();
        }
        Value* LoopBase::compile() const {

            Parser::Builder.CreateBr(List[0]);
            Parser::Builder.SetInsertPoint(List[0]);

            return nullptr;
        }

        While::While(std::unique_ptr<Expr> e, std::unique_ptr<Stmt> s) 
            : LoopBase{ std::move(e), std::move(s), num_blocks_ } {}
        void While::emit_head(Value*) const {}
        Value* While::compile() const {

            LoopBase::compile();

            emit_cond(List[1], List[2]);

            Parser::Builder.SetInsertPoint(List[1]);
            emit_body();
            Parser::Builder.CreateBr(List[0]);

            Parser::Builder.SetInsertPoint(List[2]);

            return nullptr;
        }

        RepeatUntil::RepeatUntil(std::unique_ptr<Expr> e, std::unique_ptr<Stmt> s)
            : LoopBase{ std::move(e), std::move(s), num_blocks_ } {}
        void RepeatUntil::emit_head(Value*) const {}
        Value* RepeatUntil::compile() const {

            LoopBase::compile();

            emit_body();
            emit_cond(List[0], List[1]);

            Parser::Builder.SetInsertPoint(List[1]);

            return nullptr;
        }

        For::For(std::unique_ptr<Expr> e,
            std::unique_ptr<Stmt> s1, std::unique_ptr<Stmt> s2) 
            : LoopBase{ std::move(e), std::move(s1), num_blocks_ }, 
            stmt_{ std::move(s2) } {}
        void For::set_to() {
            
            to_downto_ = true;
        }
        void For::set_downto() {

            to_downto_ = false;
        }
        Value* For::emit_preloop() const {

            return stmt_->compile();
        }
        void For::emit_head(Value* V) const {

            Value* L = Parser::Builder.CreateLoad(V);

            Value* Step;

            if(auto change = std::make_unique<Num>(1.0); to_downto_) 
                Step = Parser::Builder.CreateFAdd(L, 
                    Constant{ std::move(change) }.compile());
            else
                Step = Parser::Builder.CreateFSub(L, 
                    Constant{ std::move(change) }.compile());
            
            Parser::Builder.CreateStore(Step, V);
        }
        Value* For::compile() const {

            Value* V = emit_preloop();

            LoopBase::compile();

            emit_cond(List[1], List[2]);

            Parser::Builder.SetInsertPoint(List[1]);
            emit_body();
            emit_head(V);
            Parser::Builder.CreateBr(List[0]);

            Parser::Builder.SetInsertPoint(List[2]);

            return nullptr;
        }

        Break::Break() {

            stmt_ = Stmt::enclosing;
        }
        Value* Break::compile() const {

            Parser::Builder.CreateBr(stmt_->List.back());

            return nullptr;
        }

        Return::Return(std::unique_ptr<Expr> e) : ExprStmt{ std::move(e) } {}
        Value* Return::compile() const {
            
            Parser::Builder.CreateRet(Stmt::compile());

            return nullptr;
        }
    }
}