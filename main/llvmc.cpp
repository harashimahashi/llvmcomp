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
    lexer::Lexer lex{ std::string{ (std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>() } };
    
    // while(true) {

    //     if(lexer::Token t = lex.scan(); t) {
    //         std::cout << static_cast<int>(t) << ' ';
    //     }
    //     else break;
        
    // }
    // std::cout << '\n';

    auto Module = std::make_unique<llvm::Module>("test algolang", Context);

    std::vector<llvm::Type*> scanf_args{ llvm::Type::getInt8PtrTy(Context) };

    llvm::FunctionType *printfType = llvm::FunctionType::get(Builder.getInt32Ty(), scanf_args, true);
    llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", Module.get());

    llvm::FunctionType *mainType = llvm::FunctionType::get(Builder.getInt32Ty(), false);
    llvm::Function *main = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", Module.get());
    llvm::BasicBlock *entry = llvm::BasicBlock::Create(Context, "entry", main);
  
    Builder.SetInsertPoint(entry);
  
    std::vector<llvm::Value*> printArgs;
    llvm::Value *formatStr = Builder.CreateGlobalStringPtr("%lf\n");
    
    printArgs.push_back(formatStr);
    printArgs.push_back(llvm::ConstantInt::get(Context, llvm::APInt(32, 20)));
    Builder.CreateCall(Module->getFunction("printf"), printArgs);
    Builder.CreateAlloca(llvm::ArrayType::get(Builder.getDoubleTy(), 4), nullptr, "someArr");
  
    Builder.CreateRet(llvm::ConstantInt::get(Context, llvm::APInt(32, 0)));

    std::ofstream out{ "test.ll" };
    llvm::raw_os_ostream OutputFile(out);

    Module->print(OutputFile, nullptr);
}