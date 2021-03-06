#include <llvmc/iinter.h>
#include <llvmc/iparser.h>

namespace llvmc::inter {

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
        : Id{ std::move(t), V }, align_{ a } {}
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
            
            if(!lhs_ || !rhs_) return nullptr;

            Value* L = lhs_->compile();
            Value* R = rhs_->compile();

            if(!L || !R) return nullptr;

            switch(*op_) {
                
                case Tag{'+'}:
                    return Parser::Builder.CreateFAdd(L, R);
                case Tag{'-'}:
                    return Parser::Builder.CreateFSub(L, R);
                case Tag{'*'}:
                    return Parser::Builder.CreateFMul(L, R);
                case Tag{'/'}:
                    return Parser::Builder.CreateFDiv(L, R);
            }
        }
        
        return Parser::LogErrorV("invalid operand type");
    }

    Unary::Unary(std::unique_ptr<Token> t, std::unique_ptr<Expr> e) noexcept
        : Op{ std::move(t) }, exp_{ std::move(e) } {}
    Value* Unary::compile() {

        if(!IArray::is_array(exp_.get())) {
            
            if(!exp_) return nullptr;

            Value* E = exp_->compile();

            if(!E) return nullptr;

            return Parser::Builder.CreateFNeg(E);
        }

        return Parser::LogErrorV("invalid operand type");
    }

    Access::Access(std::shared_ptr<Id> id, ArrList vec) : Op{ nullptr }, 
        arr_{ std::move(id) }, args_{ std::move(vec) } {}
    Value* Access::compile() {
        
        if(arr_) {

            ValList args{ Parser::Builder.getInt32(0) };
            Value* arr;
            
            try {

                std::transform(args_.begin(), args_.end(), std::back_inserter(args),
                [](auto const& el) {

                    if(!el) throw std::runtime_error{ "invalid index" };
                    
                    auto V = el->compile();
                    if(!V)
                        throw std::runtime_error{ "invalid index" };

                    return Parser::Builder.CreateFPToUI(V, Parser::Builder.getInt32Ty());
                });

                arr = arr_->compile();
                auto checkType = cast<AllocaInst>(arr)->getAllocatedType();
                if(!GetElementPtrInst::getIndexedType(checkType, args))
                    throw std::runtime_error{ "invalid index" };
            }
            catch(std::exception& e) {
                
                return Parser::LogErrorV(e.what());
            }

            return Parser::Builder.CreateGEP(arr, args);
        }

        return Parser::LogErrorV("trying to access non-array id");
    }

    Load::Load(std::shared_ptr<Expr> e) noexcept 
        : Op{ nullptr }, acc_{ std::move(e) } {}
    Value* Load::compile() {

        if(!acc_) return nullptr;

        auto V = acc_->compile();
        if(!V) return nullptr;

        return Parser::Builder.CreateLoad(V);
    }

    ArrayLoad::ArrayLoad(std::shared_ptr<Id> e) noexcept
        : Op{ nullptr }, acc_{ std::static_pointer_cast<Array>(std::move(e)) } {}
    Value* ArrayLoad::compile() {

        if(!acc_) return nullptr;

        return acc_->compile();
    }
    Type* ArrayLoad::get_type() const {

        return acc_->get_type();
    }
    Align ArrayLoad::get_align() const {

        return acc_->get_align();
    }

    Store::Store(std::shared_ptr<Expr> e, std::unique_ptr<Expr> s) noexcept
        : Op{ nullptr }, acc_{ std::move(e) }, val_{ std::move(s) } {}
    Value* Store::compile() {

        if(!acc_ || !val_) return nullptr;

        Value* Acc = acc_->compile();
        Value* Val = val_->compile();

        if(!Acc || !Val) return nullptr;

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
        args_{ std::move(lst) }, saved_{ Lexer::line_ } {}
    class Call::LineGuard {

        unsigned saved_;

    public:

        LineGuard() {

            saved_ = Lexer::line_;
        }
        ~LineGuard() {

            Lexer::line_ = saved_;
        }
    };
    Value* Call::compile() {

        LineGuard g{};
        Lexer::line_ = saved_;

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

        try {

            std::transform(args_.begin(), args_.end(),
                std::back_inserter(ArgsV), [](auto const& el) {
                
                if(!el) throw std::runtime_error{ "" };

                return el->compile();
            });
        }
        catch(std::exception&) {

            return nullptr;
        }

        return Parser::Builder.CreateCall(Calee, ArgsV);
    }

    FConstant::FConstant(std::unique_ptr<Token> t) noexcept 
        : Expr{ std::move(t) } {}
    Value* FConstant::compile() {

        if(!op_) return nullptr;

        return ConstantFP::get(Parser::Context, 
            APFloat(static_cast<double>(*dynamic_cast<Num const*>(op_.get()))));
    }

    ArrayConstant::ArrayConstant(ArrList lst) : Expr{ nullptr } {

        SmallVector<Constant*, 16> carr{};

        carr_ = ConstantArray::get(
            ArrayType::get(Parser::Builder.getDoubleTy(), 0), carr);
        align_ = Parser::layout.getPrefTypeAlign(carr_->getType());
        auto array_cast = [](auto const& el) {

                    auto cnst = dynamic_cast<ArrayConstant const*>(el.get());
                    if(!cnst)
                        throw std::runtime_error{ "invalid constant initializer" };

                    return cnst->carr_;
                };
        auto constant_cast = [](auto const& el) {
                    
                    if(!el) throw std::runtime_error{ "invalid constant initializer" };
                    auto el_ = el->compile();
                    if(!el_) throw std::runtime_error{ "invalid constant initializer" };

                    if(auto c_ = dyn_cast<llvm::Constant>(el_)) return c_;

                    throw std::runtime_error{ "constant array has non-constant initializer" };
                };
        Type* T;

        try {
            
            if(lst.size()) {

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

                carr_ = ConstantArray::get(ArrayType::get(T, lst.size()), carr);
                align_ = Parser::layout.getPrefTypeAlign(carr_->getType());
            }
        }
        catch(std::exception& e) {

            Parser::LogErrorV(e.what());
        }
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

            if(!lhs_ || !rhs_) return nullptr;

            Value* L = lhs_->compile();
            Value* R = rhs_->compile();

            if(!L || !R) return nullptr;

            if(auto W = dynamic_cast<Word const*>(op_.get()); W) {

                if(*W == Word::Or) 
                    L = Parser::Builder.CreateOr(L, R);
                else if(*W == Word::And) 
                    L = Parser::Builder.CreateAnd(L, R);
                else if(*W == Word::le) 
                    L = Parser::Builder.CreateFCmpULE(L, R);
                else if(*W == Word::ge) 
                    L = Parser::Builder.CreateFCmpUGE(L, R);
                else if(*W == Word::eq) 
                    L = Parser::Builder.CreateFCmpUEQ(L, R);
                else if(*W == Word::ne) 
                    L = Parser::Builder.CreateFCmpUNE(L, R);

                return Parser::Builder.CreateUIToFP(L,
                    Parser::Builder.getDoubleTy());
            }
            else {

                switch(*op_) {

                    case Tag{'<'}:
                        L = Parser::Builder.CreateFCmpULT(L, R);
                        break;
                    case Tag{'>'}:
                        L = Parser::Builder.CreateFCmpUGT(L, R);
                        break;
                }

                return Parser::Builder.CreateUIToFP(L,
                    Parser::Builder.getDoubleTy());
            }
        }

        return Parser::LogErrorV("invalid operand type");
    }

    Not::Not(std::unique_ptr<Token> t, std::unique_ptr<Expr> e) noexcept 
        : Logical{ std::move(t) }, exp_{ std::move(e) } {}
    Value* Not::compile() {

        if(!IArray::is_array(exp_.get())) {    

            if(!exp_) return nullptr;

            Value* E = exp_->compile();

            if(!E) return nullptr;

            E = Parser::Builder.CreateXor(
                Parser::Builder.CreateFCmpUNE(E,
                ConstantFP::get(Parser::Context, APFloat(0.0))),
                Parser::Builder.getTrue());

            return Parser::Builder.CreateUIToFP(E,
                    Parser::Builder.getDoubleTy());
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

    ExprStmt::ExprStmt(std::shared_ptr<Expr> e) : expr_{ std::move(e) } {}
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

        if(!t)
            throw std::runtime_error{ "expected function name" };

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
            if(!IdPtr) continue;
            Parser::Builder.CreateStore(Func->getArg(i), IdPtr->compile());
        }

        ret_ = Parser::Builder.CreateAlloca(Parser::Builder.getDoubleTy());
    }
    void FunStmt::init(std::unique_ptr<Stmt> s) {

        stmt_ = std::move(s);
    }
    Value* FunStmt::compile() {
        
        if(stmt_) stmt_->compile();

        if(ret_) {
        
            auto V = Parser::Builder.CreateLoad(ret_);
            Parser::Builder.CreateRet(V);
        }

        return nullptr;
    }

    IfElseBase::IfElseBase(std::unique_ptr<Expr> e, std::unique_ptr<Stmt> s) 
        : expr_{ std::move(e) }, stmt_{ std::move(s) } {}
    User* IfElseBase::emit_if() const {

        if(!expr_) return nullptr;

        auto V = expr_->compile();

        if(!V) return nullptr;
        
        Value* E = Parser::Builder.CreateFPToUI(
            V, Parser::Builder.getInt1Ty());

        BBList List{ emit_bb(), create_bb() };

        Parser::Builder.CreateCondBr(E, List[0], List[1]);

        Parser::Builder.SetInsertPoint(List[0]);
        if(stmt_)
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
    void If::emit_else(User*) const {}

    IfElse::IfElse(std::unique_ptr<Expr> e, 
        std::unique_ptr<Stmt> s1, std::unique_ptr<Stmt> s2) 
        : IfElseBase{ std::move(e), std::move(s1)},
        stmt_{ std::move(s2) } {}
    void IfElse::emit_else(User* U) const {

        if(!U || !stmt_) return;

        stmt_->compile();

        auto BB = emit_bb();
        U->setOperand(0, BB);

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
        
        if(!expr_) return;

        auto V = expr_->compile();

        if(!V) return;
        
        Value* E = Parser::Builder.CreateFPToUI(
            V, Parser::Builder.getInt1Ty());

        Parser::Builder.CreateCondBr(E, B1, B2);
        
    }
    void LoopBase::emit_body(BasicBlock* BB) const {

        if(stmt_)
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

        if(stmt_) 
            return stmt_->compile();

        return nullptr;
    }
    void For::emit_head(Value* V) const {

        if(!V) return;

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
        
        if(expr_ && ret_)
            if(auto V = expr_->compile())
                Parser::Builder.CreateStore(
                    expr_->compile(), ret_);

        return nullptr;
    }
}