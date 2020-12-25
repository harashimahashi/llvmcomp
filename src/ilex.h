#include <string>
#include <memory>
#include <unordered_map>
#include <sstream>

namespace lexer {

    enum class Tag {
        AND = 256, BASIC, BREAK, REPEAT, ELSE, EQ,
        FALSE, GE, ID, IF, INDEX, LE, MINUS, NE,
        NUM, OR, TEMP, TRUE, WHILE, UNTIL, TO, 
        DOWNTO, ELSEIF, FOR, IDENT, DEIDENT, DELIM,
        DECL
    };

    class Token {
    
        const Tag tag_;
    
    public:

        Token(int) noexcept;
        virtual ~Token();

        operator bool() noexcept;
        operator int() noexcept;
    };

    class Num : public Token {

        const double val_;

    public:

        Num(double) noexcept;
    };

    class Word : public Token {

    public:

        Word(std::string, int);

        std::string lexeme_;
        static const std::unique_ptr<Word> And, Or, eq, ne, le, ge, minus, True, False, temp;

    };

    class Lexer {

        char peek_;
        std::unordered_map<std::string, Word> words_;
        static inline unsigned line_ = 1;
        unsigned ident_ = 0;
        std::stringstream source_;

        void reserve(Word);
        void readch();
        bool readch(char);

    public:

        Lexer(std::string);
        Token scan();
    };
}