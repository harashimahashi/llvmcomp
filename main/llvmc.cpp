#include <iostream>
#include <fstream>
#include <filesystem>
#include <llvmc/ilex.h>
#include <llvmc/iparser.h>

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

    llvmc::parser::Parser par{ std::move(lex), program_path };
    par.program();
}