#include <iostream>
#include <fstream>
#include <filesystem>
#include "ilex.h"

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
    std::string source{ (std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>() };
    for(char c : source) {
        std::cout << (int)c << ' ';
    }
    std::cout << '\n';
    lexer::Lexer lex{ source };
    
    while(true) {

        if(lexer::Token t = lex.scan(); t) {
            std::cout << static_cast<int>(t) << ' ';
        }
        else break;
        
    }
    std::cout << '\n';

}