#include <llvmc/ilex.h>
#include <cctype>

namespace {

    int tag_cast(lexer::Tag e) {
        return static_cast<int>(e);
    }

    bool isdigit_s(char ch) {
        return std::isdigit(static_cast<unsigned char>(ch));
    }

    bool isalpha_s(char ch) {
        return std::isalpha(static_cast<unsigned char>(ch));
    }

    bool isalnum_s(char ch) {
        return std::isalnum(static_cast<unsigned char>(ch));
    }
}

namespace lexer {

    Token::Token(int t) noexcept : tag_{ t } {}
    Token::~Token() = default;
    Token::operator int() const noexcept {
        return tag_cast(tag_);
    }
    bool Token::operator==(Token const& other) const noexcept {
        return (int)(*this) == (int)other;
    }

    Num::Num(double v) noexcept : Token{ tag_cast(Tag::NUM) }, val_{ v } {}
    
    Word::Word(std::string s, int tag) : Token{ tag }, lexeme_{ s } {}

    const std::shared_ptr<Word> 
        Word::And{ new Word{ "&&", tag_cast(Tag::AND) } },
        Word::Or{ new Word{ "||", tag_cast(Tag::OR) } },
        Word::eq{ new Word{ "==", tag_cast(Tag::EQ) } }, 
        Word::ne{ new Word{ "!=", tag_cast(Tag::NE) } }, 
        Word::le{ new Word{ "<=", tag_cast(Tag::LE) } }, 
        Word::ge{ new Word{ ">=", tag_cast(Tag::GE) } }, 
        Word::delim { new Word{ "->", tag_cast(Tag::DELIM) } },
        Word::minus{ new Word{ "minus", tag_cast(Tag::MINUS) } }, 
        Word::True{ new Word{ "true", tag_cast(Tag::TRUE) } }, 
        Word::False{ new Word{ "false", tag_cast(Tag::FALSE) } };

    void Lexer::reserve(Word* w) {

        words_.emplace(w->lexeme_, std::move(w));
    }

    void Lexer::readch() {

        peek_ = source_.get();
    }

    bool Lexer::readch(char c) {

        readch();

        if(peek_ != c) return false;
        peek_ = ' ';

        return true;
    }

    Lexer::Lexer(std::string s) : source_{ std::move(s) } {

        words_.reserve(15);

        reserve(new Word{ "if", tag_cast(Tag::IF) });
        reserve(new Word{ "else", tag_cast(Tag::ELSE) });
        reserve(new Word{ "while", tag_cast(Tag::WHILE) });
        reserve(new Word{ "repeat", tag_cast(Tag::REPEAT) });
        reserve(new Word{ "until", tag_cast(Tag::UNTIL) });
        reserve(new Word{ "for", tag_cast(Tag::FOR) });
        reserve(new Word{ "to", tag_cast(Tag::TO) });
        reserve(new Word{ "downto", tag_cast(Tag::DOWNTO) });
        reserve(new Word{ "break", tag_cast(Tag::BREAK) });
        reserve(new Word{ "fun", tag_cast(Tag::DECL) });
        reserve(new Word{ "return", tag_cast(Tag::RETURN) });
        words_.emplace((Word::True)->lexeme_, Word::True);
        words_.emplace((Word::True)->lexeme_, Word::False);
        words_.emplace("print", new Word{ "printf", tag_cast(Tag::ID) });
        words_.emplace("read", new Word{ "scanf", tag_cast(Tag::ID) });

        readch();
    }

    std::shared_ptr<Token> Lexer::scan() {

        if(new_ident_ < ident_) {
            --ident_;
            return std::make_shared<Token>(tag_cast(Tag::DEIDENT));
        }

        for(;; readch()) {

            if(peek_ == std::char_traits<char>::eof()) return std::make_shared<Token>(0);
            if(peek_ == ' ' || peek_ == '\t') continue;
            else break;
        }

        switch(peek_) {
            
            case '\n':
                {   
                    ++line_;
                    new_ident_ = 0;
                    for(; readch(), peek_ == '\t';) ++new_ident_;

                    if(new_ident_ == ident_) return scan();
                    else {
                        if(new_ident_ < ident_) {
                            --ident_;
                            return std::make_shared<Token>(tag_cast(Tag::DEIDENT));
                        }
                        else {
                            ident_ = new_ident_;
                            return std::make_shared<Token>(tag_cast(Tag::IDENT));
                        }
                    }
                }
            case '&':
                if(readch('&')) return Word::And;
                else return std::make_shared<Token>('&');
            case '|':
                if(readch('|')) return Word::Or;
                else return std::make_shared<Token>('|');
            case '=':
                if(readch('=')) return Word::eq;
                else return std::make_shared<Token>('=');
            case '!':
                if(readch('=')) return Word::ne;
                else return std::make_shared<Token>('!');
            case '<':
                if(readch('=')) return Word::le;
                else return std::make_shared<Token>('<');
            case '>':
                if(readch('=')) return Word::ge;
                else return std::make_shared<Token>('>');
            case '-':
                if(readch('>')) return Word::delim;
                else return std::make_shared<Token>('-');
        }
        if(isdigit_s(peek_)) {
            
            int v = 0;

            do {
                v = 10 * v + (peek_ - '0');
                readch();
            } while(isdigit_s(peek_));
            
            if(peek_ != '.') return std::make_shared<Num>(static_cast<double>(v));
            double x = v; 
            double d = 10;

            for(; readch(), true;) {
                
                if(!isdigit_s(peek_)) break;
                x = x + (peek_ - '0') / d;
                d = d * 10;
            }

            return std::make_shared<Num>(x);
        }
        if(isalpha_s(peek_)) {
            
            std::stringstream ss{};

            do {
                ss << peek_;
                readch();
            } while(isalnum_s(peek_));

            std::string s = ss.str();
            if(auto w = words_.find(s); w != words_.end()) return w->second;

            auto w = std::make_shared<Word>(s, tag_cast(Tag::ID));
            words_.emplace(s, w);

            return w;
        }

        auto tok = std::make_shared<Token>(peek_);
        peek_ = ' ';

        return tok;
    }
}