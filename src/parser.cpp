#include <llvmc/iparser.h>

namespace llvmc {

    namespace parser {

        using namespace llvm;
        using namespace lexer;
        using namespace inter;

        class Parser::EnvGuard {

            std::shared_ptr<symbols::Env> pPrev;

        public:

            EnvGuard() : pPrev{ top } {

                top = std::make_shared<symbols::Env>(top);
            }
            
            ~EnvGuard() {

                top = pPrev;
            }
        };

        Parser::Parser(Lexer lex) : lex_{ std::move(lex) } {

            move();
        }

        std::nullptr_t Parser::LogErrorV(std::string s) {
            
            ++err_num;
            errs() << "error:" << Lexer::line_ << ": " << s << '\n';
            return nullptr;
        }

        void Parser::move() {

            tok_ = lex_.scan();
        }

        std::unique_ptr<Token> Parser::match(Tag t) {

            if(*tok_ == t) {
                
                auto ret = std::move(tok_);
                move();
                return ret;
            }
            return LogErrorV("syntax error");
        }

        void Parser::program_preinit() {

            std::vector<Type*> args_type{ Builder.getInt8PtrTy() };

            auto funType = FunctionType::get(
                Builder.getInt32Ty(), args_type, true);
            auto printf = Function::Create(
                funType, Function::ExternalLinkage, "printf", Module.get());
            auto scanf = Function::Create(
                funType, Function::ExternalLinkage, "scanf", Module.get());

            args_type[0] = Builder.getDoubleTy();
            auto printFunType = FunctionType::get(
                Builder.getDoubleTy(), args_type, false);
            auto print = Function::Create(
                printFunType, Function::ExternalLinkage, "print", Module.get());

            auto printBB = BasicBlock::Create(Context, "", print);
            Builder.SetInsertPoint(printBB);

            auto printStr = Builder.CreateGlobalStringPtr("%lf\n");
            std::vector<Value*> printf_args{ printStr, print->getArg(0)};

            auto pCall = Builder.CreateCall(funType, printf, printf_args);
            auto pRet = Builder.CreateSIToFP(pCall, Builder.getDoubleTy());
            Builder.CreateRet(pRet);

            args_type[0] = PointerType::getUnqual(Builder.getDoubleTy());
            auto readFunType = FunctionType::get(
                Builder.getDoubleTy(), args_type, false);
            auto read = Function::Create(
                readFunType, Function::ExternalLinkage, "read", Module.get());

            auto readBB = BasicBlock::Create(Context, "", read);
            Builder.SetInsertPoint(readBB);
            
            auto readStr = Builder.CreateGlobalStringPtr("%lf\n");
            std::vector<Value*> scanf_args{ readStr, read->getArg(0)};

            auto rCall = Builder.CreateCall(funType, scanf, scanf_args);
            auto rRet = Builder.CreateSIToFP(rCall, Builder.getDoubleTy());
            Builder.CreateRet(rRet);

            auto mainType = FunctionType::get(Builder.getInt32Ty(), false);
            auto main = Function::Create(
                mainType, Function::ExternalLinkage, "main", Module.get());
            auto mainBB = BasicBlock::Create(Context, "", main);
            
            Builder.SetInsertPoint(mainBB);
        }

        void Parser::program() {
            
            program_preinit();
            fun_stmts();
        }

        void Parser::fun_stmts() {

            std::vector<std::unique_ptr<Stmt>> program;

            while(tok_) {

                switch(*tok_) {

                    case Tag::FUN:
                        {
                            auto fun = fun_def();
                            program.emplace_back(std::move(fun));
                            break;
                        }
                    case Tag::ID:
                        {
                            auto call = fun_call();
                            program.emplace_back(std::move(call));
                            break;
                        }
                }
            }

            if(err_num) {
                
                std::string err = err_num > 1 ? "errors" : "error";

                errs() << std::to_string(err_num) + ' ' + err + " generated\n";
                return;
            }

            std::for_each(program.begin(), program.end(), [](auto&& stmt) {

                stmt->compile();
            });
        }

        std::unique_ptr<Stmt> Parser::fun_def() {

            match(Tag::FUN);
            auto name = match(Tag::ID);
            match(Tag{'('});

            ArgList lst;
            while(*tok_ != Tag{')'}) {

                lst.emplace_back(match(Tag::ID));
                if(*tok_ == Tag{','}) match(Tag{','});
            }
            move();

            EnvGuard g{};
            
            auto fun = std::make_unique<FunStmt>(std::move(name), std::move(lst));

            match(Tag::IDENT);
            fun->init(stmts());
            match(Tag::DEIDENT);

            return fun;
        }

        std::unique_ptr<Stmt> Parser::fun_call() {

            auto name = match(Tag::ID);
            return std::unique_ptr<Stmt>{ nullptr };
        }

        std::unique_ptr<Stmt> Parser::stmts() {

            if(*tok_ == Tag::DEIDENT) return nullptr;

            return std::make_unique<StmtSeq>(stmt(), stmts());
        }

        std::unique_ptr<Stmt> Parser::stmt() {

            std::unique_ptr<Expr> exp;
            std::unique_ptr<Stmt> stmt1;
            std::unique_ptr<Stmt> stmt2;
            Stmt* saved;

            switch(*tok_) {

                case Tag{';'}:
                    move();
                    return nullptr;
                    break;
                case Tag::LET:
                    return decls();
                    break;
            }

            return std::unique_ptr<Stmt>{ nullptr };
        }

        std::unique_ptr<Stmt> Parser::decls() {
            
            match(Tag::LET);
            auto name = match(Tag::ID);
            std::shared_ptr<Id> id;

            if(*tok_ != Tag{'['})
                id = Id::get_id(std::move(name));
            else {

                IndexList idxs;

                while(*tok_ == Tag{'['}) {

                    move();
                    if(*tok_ == Tag{'-'}) {

                        move();
                        LogErrorV("array size must be positive number");
                    }
                    double val{0.0};
                    if(auto num = match(Tag::NUM)) {
                        val = *static_cast<Num const*>(num.get());
                    }
                    else
                        while(*tok_ != Tag{']'}) move();
                    double intp; 

                    if(!(std::modf(val, &intp) == 0)) {

                        LogErrorV("array size must not be double");
                    }

                    idxs.emplace_back(static_cast<uint64_t>(intp));
                    match(Tag{']'});
                }

                id = Array::get_array(std::move(name), idxs);
            }

            if(*tok_ != Tag{'='}) return nullptr;
            
            move();
            auto store = std::make_unique<Store>(id, pbool());
            return std::make_unique<ExprStmt>(std::move(store));
        }

        std::unique_ptr<Expr> Parser::pbool() {

            auto exp = join();

            while(*tok_ == Tag::OR) {

                auto op = match(Tag::OR);
                exp = std::make_unique<Bool>(
                    std::move(op), std::move(exp), join());
            }

            return exp;
        }

        std::unique_ptr<Expr> Parser::join() {

            auto exp = equality();

            while(*tok_ == Tag::AND) {

                auto op = match(Tag::AND);
                exp = std::make_unique<Bool>(
                    std::move(op), std::move(exp), equality());
            }

            return exp;
        }

        std::unique_ptr<Expr> Parser::equality() {

            auto exp = rel();

            while((*tok_ == Tag::EQ) || (*tok_ == Tag::NE)) {

                auto op = std::move(tok_); move();
                exp = std::make_unique<Bool>(
                    std::move(op), std::move(exp), rel());
            }

            return exp;
        }

        std::unique_ptr<Expr> Parser::rel() {

            auto exp = expr();

            switch(*tok_) {

                [[fallthrough]];
                case Tag{'<'}:
                case Tag::LE:
                case Tag::GE:
                case Tag{'>'}:
                    {
                        auto op = std::move(tok_); move();
                        return std::make_unique<Bool>(
                            std::move(op), std::move(exp), expr());
                    }
                default:
                    return exp;
            }
        }

        std::unique_ptr<Expr> Parser::expr() {

            auto exp = term();

            while((*tok_ == Tag{'+'}) || (*tok_ == Tag{'-'})) {

                auto op = std::move(tok_); move();
                exp = std::make_unique<Arith>(
                    std::move(op), std::move(exp), term());
            }

            return exp;
        }

        std::unique_ptr<Expr> Parser::term() {

            auto exp = unary();

            while((*tok_ == Tag{'*'}) || (*tok_ == Tag{'/'})) {

                auto op = std::move(tok_); move();
                exp = std::make_unique<Arith>(
                    std::move(op), std::move(exp), unary());
            }

            return exp;
        }

        std::unique_ptr<Expr> Parser::unary() {

            if((*tok_ == Tag{'-'}) || (*tok_ == Tag{'!'})) {

                auto op = std::move(tok_); move();
                return std::make_unique<Not>(
                    std::move(op), unary());
            }

            return factor();
        }

        std::unique_ptr<Expr> Parser::factor() {

            std::unique_ptr<Expr> exp{};

            switch(*tok_) {

                case Tag{'('}:
                    move();
                    exp = pbool();
                    match(Tag{')'});
                    return exp;
                case Tag::NUM:
                    exp = std::make_unique<FConstant>(
                        std::move(tok_)); move();
                    return exp;
                case Tag::TRUE:
                    exp = std::make_unique<FConstant>(
                        std::make_unique<Num>(1.0));
                    move();
                    return exp;
                case Tag::FALSE:
                    exp = std::make_unique<FConstant>(
                        std::make_unique<Num>(0.0));
                    move();
                    return exp;
                case Tag::ID:
                    {
                        auto name = static_cast<Word const*>(
                            match(Tag::ID).get())->lexeme_;
                        auto id = top->get(name);
                        
                        if(auto pId = std::dynamic_pointer_cast<Array>(id); id && !pId) {

                            return std::make_unique<Load>(id);
                        }
                        else if(id && (*tok_ != Tag{'['})){

                            return std::make_unique<ArrayLoad>(id);
                        }
                        else if(*tok_ == Tag{'['}) {

                            return std::make_unique<Load>(access(id.get()));
                        }

                        return LogErrorV("using of undeclared \'" + name + '\'');
                    }
                case Tag{'['}:
                    move();
                    exp = std::make_unique<ArrayConstant>(expr_seq());
                    match(Tag{']'});
                    return exp;
            }
        }

        std::unique_ptr<Expr> Parser::access(Id* id) {

            ValList idxs;

            while(*tok_ == Tag{'['}) {
                
                move();
                idxs.emplace_back(pbool()->compile());
                match(Tag{']'});
            }

            return std::make_unique<Access>(id, idxs);
        }

        ArrList Parser::expr_seq() {

            ArrList lst{};

            if((*tok_ != Tag{']'}) && (*tok_ != Tag{')'}))
                lst.emplace_back(pbool());

            while(*tok_ == Tag{','}) {

                move();
                lst.emplace_back(pbool());
            }

            return lst;
        }
    }
}
