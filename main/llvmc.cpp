#include "llvm/IR/Verifier.h"
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
static llvm::IRBuilder Builder(Context);

int main(int argc, char* argv[]){

    namespace fs = std::filesystem;

    if(argc != 2) {
        llvm::errs() << "Error: wrong argument numbers\n";
        return 1;
    }

    std::string program_path{ argv[1] };

    if(!fs::exists(program_path)) {
        llvm::errs() << "Error: no such file " << program_path << '\n';
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

    using namespace llvm;

    auto Module = std::make_unique<llvm::Module>("test algolang", Context);

    std::vector<Type*> scanf_args{ Builder.getInt8PtrTy() };

    FunctionType *printfType = FunctionType::get(Builder.getInt32Ty(), scanf_args, true);
    Function::Create(printfType, Function::ExternalLinkage, "printf", Module.get());

    FunctionType *mainType = FunctionType::get(Builder.getInt32Ty(), false);
    Function *main = Function::Create(mainType, Function::ExternalLinkage, "main", Module.get());
    BasicBlock *entry = BasicBlock::Create(Context, "entry", main);
    
    Builder.SetInsertPoint(entry);
    
    DataLayout layout{ Module.get() };

    std::vector<Value*> printArgs;
    Value *formatStr = Builder.CreateGlobalStringPtr("%lf\n");
    Module->getOrInsertGlobal("array", ArrayType::get(Builder.getDoubleTy(), 2));
    auto garr = Module->getNamedGlobal("array");
    garr->setLinkage(GlobalValue::LinkageTypes::PrivateLinkage);
    garr->setConstant(true);
    garr->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
    std::vector<Constant*> cvec{ ConstantFP::get(Builder.getDoubleTy(), APFloat(1.0)), ConstantFP::get(Builder.getDoubleTy(), APFloat(2.0)) };
    auto carr = ConstantArray::get(ArrayType::get(Builder.getDoubleTy(), 2), cvec);
    garr->setInitializer(carr);
    garr->setAlignment(layout.getPrefTypeAlign(carr->getType()));
    
    printArgs.push_back(formatStr);
    //Builder.CreateCall(Module->getFunction("printf"), printArgs);
    auto arr = Builder.CreateAlloca(ArrayType::get(Builder.getDoubleTy(), 2), nullptr, "someArr");
    auto arr1 = Builder.CreateAlloca(ArrayType::get(Builder.getDoubleTy(), 2), nullptr, "someArr");
    std::vector<Value*> gep_arg{ Builder.getInt32(0), Builder.getInt32(0) };
    Builder.CreateStore(ConstantFP::get(Context, APFloat(1.0)), Builder.CreateGEP(arr, gep_arg));
    gep_arg[1] = Builder.getInt32(1);
    Builder.CreateStore(ConstantFP::get(Context, APFloat(2.0)), Builder.CreateGEP(arr, gep_arg));
    gep_arg[1] = Builder.getInt32(0); 

    auto arrref = cast<ArrayType>(arr->getAllocatedType());
    raw_os_ostream tout{ std::cout };
    auto tgep = Builder.CreateGEP(arr, gep_arg);
    auto afgep = cast<ArrayType>(cast<PointerType>(cast<GetElementPtrInst>(tgep)->getPointerOperandType())->getElementType());

    Builder.CreateMemCpy(
        arr1, arr1->getAlign(),
        garr, garr->getAlign(), 
        layout.getTypeStoreSizeInBits(arrref) / 8);

    Builder.CreateRet(ConstantInt::get(Context, APInt(32, 0)));

    std::ofstream out{ "test.ll" };
    raw_os_ostream OutputFile(out);

    Module->print(OutputFile, nullptr);
}