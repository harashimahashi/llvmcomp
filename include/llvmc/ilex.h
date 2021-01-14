#ifndef LLVMC_ILEX_H_
#define LLVMC_ILEX_H_
#include <string>
#include <memory>
#include <unordered_map>
#include <sstream>

namespace llvmc {

    namespace lexer {

        enum class Tag {
            AND = 256, BREAK, REPEAT, ELSE, EQ,
            FALSE, GE, ID, IF, INDEX, LE, MINUS, NE,
            NUM, OR, TRUE, WHILE, UNTIL, TO, DOWNTO,
            FOR, IDENT, DEIDENT, FUN, LET, RETURN
        };

        class Token {
        
            const Tag tag_;
        
        public:

            Token(int) noexcept;
            virtual ~Token();
            operator int() const noexcept;
        };

        class Num : public Token {

            const double val_;

        public:

            Num(double) noexcept;
            operator double() const noexcept;
        };

        class Word : public Token {

        public:

            Word(std::string, int);
            
            static const Word And, Or, eq, ne, le, ge, minus, True, False;
            std::string lexeme_;
        };

        class Lexer {

            char peek_;
            std::unordered_map<std::string, Word> words_;
            unsigned ident_ = 0;
            unsigned new_ident_ = ident_;
            std::stringstream source_;

            void reserve(Word);
            void readch();
            bool readch(char);

        public:

            Lexer(std::string);
            std::unique_ptr<Token> scan();

            static inline unsigned line_ = 1;
        };
    }
}
#endif