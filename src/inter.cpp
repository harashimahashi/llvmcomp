#include <llvmc/iinter.h>
#include <llvmc/iparser.h>

namespace llvmc {

    namespace inter {

        using namespace llvm;
        using namespace lexer;
        using namespace parser;

        Node::~Node() = default;

        Expr::Expr(std::unique_ptr<Token> t) noexcept : op_{ std::move(t) } {}

        Id::Id(std::unique_ptr<Token> t, Value* V) 
            : Expr{ std::move(t) }, var_{ V } {}
        std::shared_ptr<Id> Id::get_id(std::unique_ptr<Token> t) {

            Value* V = Parser::Builder.CreateAlloca(
                        Parser::Builder.getDoubleTy(), nullptr);
            std::string name = static_cast<Word*>(t.get())->lexeme_;

            if(Parser::top->get_current(name)) 
                return Parser::LogErrorV("redefinition of \'" + name + '\'');
            
            auto sp = std::shared_ptr<Id>{ new Id{ std::move(t), V } };
            Parser::top->insert(name,  sp);

            return sp;
        }
        Value* Id::get_val() const {

            return var_;
        }
        Value* Id::compile() {

            return var_;
        }

        bool IArray::is_array(Expr const* E) {

            if(auto A = dynamic_cast<IArray const*>(E))
                return true;
            
            return false;
        }

        Array::Array(std::unique_ptr<lexer::Token> t, Value* V, size_t u, Align a) 
            : Id{ std::move(t), V }, dim_{ u }, align_{ a } {}
        std::shared_ptr<Array> Array::get_array(
            std::unique_ptr<lexer::Token> t, IndexList L) {

            size_t sz = L.size();
            Type* T = ArrayType::get(
                        Parser::Builder.getDoubleTy(), L.pop_back_val());

            while(L.size()) {

                T = ArrayType::get(T, L.pop_back_val());
            }

            auto V = Parser::Builder.CreateAlloca(T, nullptr);
            auto A = V->getAlign();
            std::string name = static_cast<Word*>(t.get())->lexeme_;

            if(Parser::top->get_current(name)) 
                return Parser::LogErrorV("redefinition of \'" + name + '\'');

            auto sp = std::shared_ptr<Array>{ new Array{ std::move(t), V, sz, A } };
            Parser::top->insert(name, sp);
            
            return sp;
        }
        Value* Array::compile() {

            return Id::compile();
        }
        Type* Array::get_type() const {

            return cast<AllocaInst>(get_val())->getAllocatedType();
        }
        Align Array::get_align() const {

            return align_;
        }

        Op::Op(std::unique_ptr<Token> t) noexcept : Expr{ std::move(t) } {}

        Arith::Arith(std::unique_ptr<Token> t, std::unique_ptr<Expr> e1,
            std::unique_ptr<Expr> e2) noexcept : Op{ std::move(t) },
            lhs_{ std::move(e1) }, rhs_{ std::move(e2) } {}
        Value* Arith::compile() {
            
            if(!IArray::is_array(lhs_.get()) && !IArray::is_array(rhs_.get())) {
   
                Value* L = lhs_->compile();
                Value* R = rhs_->compile();

                switch(*op_) {
                    
                    case Tag{'+'}:
                        return Parser::Builder.CreateFAdd(L, R, "addtmp");
                    case Tag{'-'}:
                        return Parser::Builder.CreateFSub(L, R, "subtmp");
                    case Tag{'*'}:
                        return Parser::Builder.CreateFMul(L, R, "multmp");
                    case Tag{'/'}:
                        return Parser::Builder.CreateFDiv(L, R, "muldiv");
                }
            }
            
            return Parser::LogErrorV("invalid operand type");
        }

        Unary::Unary(std::unique_ptr<Token> t, std::unique_ptr<Expr> e) noexcept
            : Op{ std::move(t) }, exp_{ std::move(e) } {}
        Value* Unary::compile() {

            if(!IArray::is_array(exp_.get())) {

                Value* E = exp_->compile();

                return Parser::Builder.CreateFNeg(E, "subneg");
            }

            return Parser::LogErrorV("invalid operand type");
        }

        Access::Access(Id* id, ValList vec) : Op{ nullptr }, 
            arr_{ id ? id->compile() : nullptr }, args_{ std::move(vec) } {}
        Value* Access::compile() {
            
            if(arr_) {

                ValList args{ Parser::Builder.getInt32(0) };
                
                std::transform(args_.begin(), args_.end(), std::back_inserter(args),
                [](auto el) {

                    return Parser::Builder.CreateFPToUI(el, Parser::Builder.getInt32Ty());
                });

                return Parser::Builder.CreateGEP(arr_, args);
            }

            return Parser::LogErrorV("trying to access non-array id");
        }

        Load::Load(std::shared_ptr<Expr> e) noexcept 
            : Op{ nullptr }, acc_{ e } {}
        Value* Load::compile() {

            auto val = acc_ ? acc_->compile() : nullptr;

            return Parser::Builder.CreateLoad(val);
        }

        ArrayLoad::ArrayLoad(std::shared_ptr<Id> e) noexcept
            : Op{ nullptr }, acc_{ std::static_pointer_cast<Array>(e) } {}
        Value* ArrayLoad::compile() {

            return acc_->compile();
        }
        Type* ArrayLoad::get_type() const {

            return acc_->get_type();
        }
        Align ArrayLoad::get_align() const {

            return acc_->get_align();
        }

        Store::Store(std::shared_ptr<Expr> e, std::unique_ptr<Expr> s) noexcept
            : Op{ nullptr }, acc_{ e }, val_{ std::move(s) } {}
        Value* Store::compile() {

            Value* Acc = acc_->compile();
            Value* Val = val_->compile();

            if(!IArray::is_array(acc_.get()) && !IArray::is_array(val_.get())){
        
                Parser::Builder.CreateStore(Val, Acc);
            }
            else {

                auto a_Acc = dynamic_cast<IArray const*>(acc_.get());
                auto a_Val = dynamic_cast<IArray const*>(val_.get());
                
                if(a_Acc && a_Val) {

                    if(auto T = a_Val->get_type(); T == a_Acc->get_type()) {

                        Parser::Builder.CreateMemCpy(
                            Acc, a_Acc->get_align(),
                            Val, a_Val->get_align(),
                            Parser::layout.getTypeSizeInBits(T) / IArray::kByteSize);
                    }
                    else    
                        return Parser::LogErrorV("incompatible array types");

                }
                else
                    return Parser::LogErrorV("incompatible types");
            }

            return Acc;
        }

        Call::Call(std::unique_ptr<Token> t, ArrList lst) 
            : Op{ std::move(t) }, name_{ static_cast<Word const*>(op_.get())->lexeme_ },
            args_{ std::move(lst) } {}
        Value* Call::compile() {

            auto Calee = Parser::Module->getFunction(name_);
            if(!Calee) 
                return Parser::LogErrorV("unknown function referenced");
            
            size_t par_sz = Calee->arg_size();
            size_t arg_sz = args_.size();
            if(par_sz != arg_sz)
                return Parser::LogErrorV("wrong arguments number: expected "
                + std::to_string(par_sz) + ", but " 
                + std::to_string(arg_sz) + " provided");

            ValList ArgsV;
            std::transform(args_.begin(), args_.end(),
                std::back_inserter(ArgsV), [](auto const& el) {

                return el->compile();
            });


            return Parser::Builder.CreateCall(Calee, ArgsV);
        }

        FConstant::FConstant(std::unique_ptr<Token> t) noexcept 
            : Expr{ std::move(t) } {}
        Value* FConstant::compile() {

            return ConstantFP::get(Parser::Context, 
                APFloat(static_cast<double>(*dynamic_cast<Num const*>(op_.get()))));
        }

        ArrayConstant::ArrayConstant(ArrList lst) : Expr{ nullptr } {

            SmallVector<Constant*, 16> carr;
            bool c_err{ false };
            auto array_cast = [](auto const& el) {
                        return dynamic_cast<ArrayConstant const*>(el.get())->carr_;
                    };
            auto constant_cast = [&c_err](auto const& el) {
                        
                        auto c_ = dyn_cast<llvm::Constant>(el.get()->compile());

                        if(c_) return c_;

                        c_err = true;
                        return c_;
                    };
            Type* T;

            if(auto A = dynamic_cast<ArrayConstant const*>(lst.begin()->get())) {

                std::transform(lst.begin(), lst.end(),
                    std::back_inserter(carr), array_cast);

                T = A->carr_->getType();
            }
            else {

                std::transform(lst.begin(), lst.end(),
                    std::back_inserter(carr), constant_cast);
                
                T = Parser::Builder.getDoubleTy();
            }

            if(c_err)
                Parser::LogErrorV("constant array has non-constant initializer");

            carr_ = ConstantArray::get(ArrayType::get(T, lst.size()), carr);
            align_ = Parser::layout.getPrefTypeAlign(carr_->getType());
        }
        Value* ArrayConstant::compile() {
            
            std::string name_ = "array" + std::to_string(cnt_++);

            Parser::Module->getOrInsertGlobal(name_, get_type());
            auto garr = Parser::Module->getNamedGlobal(name_);

            garr->setLinkage(GlobalValue::LinkageTypes::PrivateLinkage);
            garr->setConstant(true);
            garr->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
            garr->setInitializer(carr_);
            garr->setAlignment(align_);

            return garr;
        }
        Type* ArrayConstant::get_type() const {

            return carr_->getType();
        }
        Align ArrayConstant::get_align() const {

            return align_;
        }

        Logical::Logical(std::unique_ptr<Token> t) noexcept 
            : Expr{ std::move(t) } {}

        Bool::Bool(std::unique_ptr<Token> t, std::unique_ptr<Expr> e1, 
            std::unique_ptr<Expr> e2) noexcept : Logical{ std::move(t) },
            lhs_{ std::move(e1) }, rhs_{ std::move(e2) } {}
        Value* Bool::compile() {

            if(!IArray::is_array(lhs_.get()) && !IArray::is_array(rhs_.get())) {

                Value* L = lhs_->compile();
                Value* R = rhs_->compile();

                if(auto W = dynamic_cast<Word const*>(op_.get()); W) {

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

                        case Tag{'<'}:
                            L = Parser::Builder.CreateFCmpULT(L, R, "subcmp");
                            break;
                        case Tag{'>'}:
                            L = Parser::Builder.CreateFCmpUGT(L, R, "subcmp");
                            break;
                    }

                    return Parser::Builder.CreateUIToFP(L,
                        Parser::Builder.getDoubleTy(), "booltmp");
                }
            }

            return Parser::LogErrorV("invalid operand type");
        }

        Not::Not(std::unique_ptr<Token> t, std::unique_ptr<Expr> e) noexcept 
            : Logical{ std::move(t) }, exp_{ std::move(e) } {}
        Value* Not::compile() {

            if(!IArray::is_array(exp_.get())) {    

                Value* E = exp_->compile();

                switch(*op_) {
                    
                    case Tag{'!'}:
                        E = Parser::Builder.CreateXor(
                            Parser::Builder.CreateFCmpUNE(E,
                            ConstantFP::get(Parser::Context, APFloat(0.0))),
                            Parser::Builder.getTrue());
                        break;
                    case Tag{'-'}:
                        E = Parser::Builder.CreateNot(E, "subnot");
                        break;
                }

                return Parser::Builder.CreateUIToFP(E,
                        Parser::Builder.getDoubleTy(), "booltmp");
            }

            return Parser::LogErrorV("invalid operand type");
        }

        BasicBlock* Stmt::create_bb() const {

            return BasicBlock::Create(Parser::Context);
        }
        BasicBlock* Stmt::emit_bb(BasicBlock* BB) const {
            
            Function* par = Parser::Builder.GetInsertBlock()->getParent();

            if(!BB) BB = create_bb();
            BB->insertInto(par);

            return BB;
        }
        Stmt::EnclosingGuard::EnclosingGuard(Stmt* s) : saved_{ enclosing_ } {

            enclosing_ = s;
        }
        Stmt::EnclosingGuard::~EnclosingGuard() {

            enclosing_ = saved_;
        }

        StmtSeq::StmtSeq(std::unique_ptr<Stmt> s1, std::unique_ptr<Stmt> s2) :
            stmt1_{ std::move(s1) }, stmt2_{ std::move(s2) } {}
        Value* StmtSeq::compile() {

            if(stmt1_) stmt1_->compile();
            if(stmt2_) stmt2_->compile();

            return nullptr;
        }

        ExprStmt::ExprStmt(std::shared_ptr<Expr> e) : expr_{ e } {}
        Value* ExprStmt::compile() {

            if(expr_) 
                return expr_->compile();

            return nullptr;
        }

        FunStmt::FunStmt(std::unique_ptr<lexer::Token> t, ArgList lst) 
            : stmt_{ nullptr } {
            
            //function arguments
            SmallVector<Type*, 8> doubles(lst.size(),
                Parser::Builder.getDoubleTy());
            std::string name_ = static_cast<Word*>(t.get())->lexeme_;

            //create function
            auto FType = FunctionType::get(
                Parser::Builder.getDoubleTy(), doubles, false);
            auto Func = Function::Create(FType,
                Function::ExternalLinkage, name_, *Parser::Module);
            auto BB = BasicBlock::Create(Parser::Context, "", Func);
            Parser::Builder.SetInsertPoint(BB);
            
            //emitting function args as variables
            for(size_t i = 0, sz = lst.size(); i < sz; i++) {

                auto IdPtr = Id::get_id(std::move(lst[i]));
                Parser::Builder.CreateStore(Func->getArg(i), IdPtr->compile());
            }
        }
        void FunStmt::init(std::unique_ptr<Stmt> s) {

            stmt_ = std::move(s);
        }
        Value* FunStmt::compile() {
            
            if(stmt_) stmt_->compile();

            return nullptr;
        }

        MainStmt::MainStmt(std::unique_ptr<Stmt> s) : stmt_{ std::move(s) } {}
        Value* MainStmt::compile() {

            auto currBB = Parser::Builder.GetInsertBlock();
            auto main = Parser::Module->getFunction("main");
            auto& mainBB = main->getEntryBlock();

            Parser::Builder.SetInsertPoint(&mainBB);
            stmt_->compile();

            Parser::Builder.SetInsertPoint(currBB);

            return nullptr;
        }

        IfElseBase::IfElseBase(std::unique_ptr<Expr> e, std::unique_ptr<Stmt> s) 
            : expr_{ std::move(e) }, stmt_{ std::move(s) } {}
        User* IfElseBase::emit_if() const {
            
            Value* E = Parser::Builder.CreateFPToUI(
                expr_->compile(), Parser::Builder.getInt1Ty());

            BBList List{ emit_bb(), create_bb() };

            User* br = Parser::Builder.CreateCondBr(E, List[0], List[1]);

            Parser::Builder.SetInsertPoint(List[0]);
            stmt_->compile();

            emit_bb(List[1]);

            auto ret = Parser::Builder.CreateBr(List[1]);
            Parser::Builder.SetInsertPoint(List[1]);

            return ret;
        }
        llvm::Value* IfElseBase::compile() { 

            auto end = emit_if();
            emit_else(end);

            return nullptr;
        }

        If::If(std::unique_ptr<Expr> e, std::unique_ptr<Stmt> s)
            : IfElseBase{ std::move(e), std::move(s) } {}
        void If::emit_else(User* u) const {}

        IfElse::IfElse(std::unique_ptr<Expr> e, 
            std::unique_ptr<Stmt> s1, std::unique_ptr<Stmt> s2) 
            : IfElseBase{ std::move(e), std::move(s1)},
            stmt_{ std::move(s2) } {}
        void IfElse::emit_else(User* u) const {

            stmt_->compile();

            auto BB = emit_bb();
            u->setOperand(0, BB);

            Parser::Builder.CreateBr(BB);
            Parser::Builder.SetInsertPoint(BB);
        }

        LoopBase::LoopBase() : expr_{ nullptr }, stmt_{ nullptr } {}
        void LoopBase::init(std::unique_ptr<Expr> e, std::unique_ptr<Stmt> s) {

            expr_ = std::move(e);
            stmt_ = std::move(s);
        }
        Value* LoopBase::emit_preloop() const {

            return nullptr;
        }
        void LoopBase::emit_cond(BasicBlock* B1, BasicBlock* B2) const {

            Value* E = Parser::Builder.CreateFPToUI(
                expr_->compile(), Parser::Builder.getInt1Ty());

            Parser::Builder.CreateCondBr(E, B1, B2);
        }
        void LoopBase::emit_body(BasicBlock* BB) const {

            stmt_->compile();

            emit_bb(BB);
        }
        void LoopBase::fix_br(BasicBlock* B1, BasicBlock* B2) const {

            for(auto currBB = B1; currBB != B2; currBB = currBB->getNextNode()) {

                bool found{ false };
                
                for(auto IB = currBB->begin(); IB != currBB->end();) {
                    
                    if(found) {
                        IB = IB->eraseFromParent();
                        continue;
                    }

                    User* Br = dyn_cast<BranchInst>(IB);
                    if(Br && (Br->getNumOperands() == 1)){ 

                        auto BB = cast<BasicBlock>(Br->getOperand(0));

                        if(!BB->getParent()) { 

                            Br->setOperand(0, B2);
                            found = true;
                        }
                    }
                    IB++;
                } 
            }
        }
        Value* LoopBase::compile() {
            
            auto BB = emit_bb();

            Parser::Builder.CreateBr(BB);
            Parser::Builder.SetInsertPoint(BB);

            return BB;
        }

        void While::init(std::unique_ptr<Expr> e, std::unique_ptr<Stmt> s) {

            LoopBase::init(std::move(e), std::move(s));
        }
        void While::emit_head(Value*) const {}
        Value* While::compile() {

            auto BB = LoopBase::compile();
            BBList List{ cast<BasicBlock>(BB), emit_bb(), create_bb() };

            emit_cond(List[1], List[2]);

            Parser::Builder.SetInsertPoint(List[1]);
            emit_body(List[2]);
            fix_br(List[1], List[2]);
            Parser::Builder.CreateBr(List[0]);

            Parser::Builder.SetInsertPoint(List[2]);

            return nullptr;
        }

        void RepeatUntil::init(std::unique_ptr<Expr> e, std::unique_ptr<Stmt> s) {

            LoopBase::init(std::move(e), std::move(s));
        }
        void RepeatUntil::emit_head(Value*) const {}
        Value* RepeatUntil::compile() {

            auto BB = LoopBase::compile();
            BBList List{ cast<BasicBlock>(BB), create_bb() };

            emit_body(List[1]);
            emit_cond(List[0], List[1]);
            fix_br(List[0], List[1]);

            Parser::Builder.SetInsertPoint(List[1]);

            return nullptr;
        }

        For::For() : stmt_{ nullptr } {}
        void For::init(std::unique_ptr<Expr> e,
            std::unique_ptr<Stmt> s1, std::unique_ptr<Stmt> s2) {
            
            LoopBase::init(std::move(e), std::move(s1));
            stmt_ = std::move(s2);
        }
        void For::set_to() {
            
            to_downto_ = true;
        }
        void For::set_downto() {

            to_downto_ = false;
        }
        Value* For::emit_preloop() const {

            if(stmt_) return stmt_->compile();

            return nullptr;
        }
        void For::emit_head(Value* V) const {

            Value* L = Parser::Builder.CreateLoad(V);
            Value* Step;

            if(auto change = std::make_unique<Num>(1.0); to_downto_) 
                Step = Parser::Builder.CreateFAdd(L, 
                    FConstant{ std::move(change) }.compile());
            else
                Step = Parser::Builder.CreateFSub(L, 
                    FConstant{ std::move(change) }.compile());
            
            Parser::Builder.CreateStore(Step, V);
        }
        Value* For::compile() {

            //init of loop counter
            Value* V = emit_preloop();

            auto BB = LoopBase::compile();
            BBList List{ cast<BasicBlock>(BB), emit_bb(),
                create_bb(), create_bb() };

            emit_cond(List[1], List[2]);

            Parser::Builder.SetInsertPoint(List[1]);
            emit_body(List[2]);
            //emitting counter increment/decrement
            emit_head(V);
            Parser::Builder.CreateBr(List[0]);
            fix_br(List[1], List[2]);

            Parser::Builder.SetInsertPoint(List[2]);

            return nullptr;
        }

        Break::Break() : stmt_{ enclosing_ } {}
        Value* Break::compile() {
            
            if(!stmt_) 
                return Parser::LogErrorV("unenclosed break");

            auto BB = create_bb();
            Parser::Builder.CreateBr(BB);

            return nullptr;
        }

        Return::Return(std::unique_ptr<Expr> e) 
            : expr_{ std::move(e) } {}
        Value* Return::compile() {
            
            if(expr_)
                Parser::Builder.CreateRet(expr_->compile());

            return nullptr;
        }
    }
}