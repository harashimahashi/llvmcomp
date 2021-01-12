//#include "llvm/IR/Verifier.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <llvmc/ilex.h>
#include <llvmc/iparser.h>

static llvm::LLVMContext Context;
static llvm::IRBuilder<> Builder(Context);

int main(int argc, char* argv[]){

    namespace fs = std::filesystem;

    if(argc != 2) {
        std::cerr << "Error: wrong argument numbers\n";
        return 1;
    }

    std::string program_path{ argv[1] };

    if(!fs::exists(program_path)) {
        std::cerr << "Error: no such file " << program_path << '\n';
        return 1;
    }

    std::ifstream in{ program_path };
    llvmc::lexer::Lexer lex{ std::string{ (std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>() } };
    
    // while(true) {

    //     if(lexer::Token t = lex.scan(); t) {
    //         std::cout << static_cast<int>(t) << ' ';
    //     }
    //     else break;
        
    // }
    // std::cout << '\n';

    auto Module = std::make_unique<llvm::Module>("test algolang", Context);

    std::vector<llvm::Type*> scanf_args{ Builder.getInt8PtrTy() };

    llvm::FunctionType *printfType = llvm::FunctionType::get(Builder.getInt32Ty(), scanf_args, true);
    llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", Module.get());

    llvm::FunctionType *mainType = llvm::FunctionType::get(Builder.getInt32Ty(), false);
    llvm::Function *main = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", Module.get());
    llvm::BasicBlock *entry = llvm::BasicBlock::Create(Context, "entry", main);
    
    Builder.SetInsertPoint(entry);
    
    std::vector<llvm::Value*> printArgs;
    llvm::Value *formatStr = Builder.CreateGlobalStringPtr("%lf\n");
    Module->getOrInsertGlobal("array", llvm::ArrayType::get(Builder.getInt32Ty(), 4));
    auto garr = Module->getNamedGlobal("array");
    garr->setLinkage(llvm::GlobalValue::LinkageTypes::PrivateLinkage);
    garr->setConstant(true);
    garr->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
    
    printArgs.push_back(formatStr);
    printArgs.push_back(llvm::ConstantInt::get(Context, llvm::APInt(32, 20)));
    Builder.CreateCall(Module->getFunction("printf"), printArgs);
    auto arr = Builder.CreateAlloca(llvm::ArrayType::get(llvm::ArrayType::get(Builder.getDoubleTy(), 4), 2), nullptr, "someArr");
    auto arr1 = Builder.CreateAlloca(llvm::ArrayType::get(Builder.getDoubleTy(), 2), nullptr, "someArr");
    std::vector<llvm::Value*> gep_arg{ Builder.getInt32(0), Builder.getInt32(0), Builder.getInt32(0) };
    /* Builder.CreateStore(llvm::ConstantFP::get(Context, llvm::APFloat(1.0)), Builder.CreateGEP(arr, gep_arg));
    gep_arg[1] = Builder.getInt32(1);
    Builder.CreateStore(llvm::ConstantFP::get(Context, llvm::APFloat(2.0)), Builder.CreateGEP(arr, gep_arg));
    gep_arg[1] = Builder.getInt32(0); */

    auto arrref = llvm::cast<llvm::ArrayType>(arr->getAllocatedType());
    llvm::raw_os_ostream tout{ std::cout };
    auto tgep = Builder.CreateGEP(arr, gep_arg);
    auto afgep = llvm::cast<llvm::ArrayType>(llvm::cast<llvm::PointerType>(llvm::cast<llvm::GetElementPtrInst>(tgep)->getPointerOperandType())->getElementType());
    std::cout << std::boolalpha << afgep->isArrayTy() << ' ' << afgep->getElementType()->isArrayTy() << '\n';

    Builder.CreateMemCpy(
        Builder.CreateGEP(arr1, gep_arg), arr1->getAlign(),
        Builder.CreateGEP(arr, gep_arg), arr->getAlign(), 
        arrref->getElementType()->getPrimitiveSizeInBits()
        * arrref->getArrayNumElements() / 8);
    
    Builder.CreateRet(llvm::ConstantInt::get(Context, llvm::APInt(32, 0)));

    std::ofstream out{ "test.ll" };
    llvm::raw_os_ostream OutputFile(out);

    Module->print(OutputFile, nullptr);
}