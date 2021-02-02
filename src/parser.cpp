#include <llvmc/iparser.h>

namespace llvmc {

    namespace parser {

        using namespace llvm;

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

        }
    }
}
