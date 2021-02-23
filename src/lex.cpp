#include <llvmc/ilex.h>
#include <cctype>

namespace {

    int tag_cast(llvmc::lexer::Tag e) {
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

namespace llvmc::lexer {

    Token::Token(int t) noexcept : tag_{ t } {}
    Token::~Token() = default;
    Token::operator Tag() const noexcept {
        
        return tag_;
    }

    Num::Num(double v) noexcept : Token{ tag_cast(Tag::NUM) }, val_{ v } {}
    Num::operator double() const noexcept {
        
        return val_;
    }
    
    Word::Word(std::string s, int tag) : Token{ tag }, lexeme_{ s } {}
    bool Word::operator==(Word const& w) const noexcept {

        return lexeme_ == w.lexeme_;
    }

    const Word 
        Word::And{ "&&", tag_cast(Tag::AND) },
        Word::Or{ "||", tag_cast(Tag::OR) },
        Word::eq{ "==", tag_cast(Tag::EQ) }, 
        Word::ne{ "!=", tag_cast(Tag::NE) }, 
        Word::le{ "<=", tag_cast(Tag::LE) }, 
        Word::ge{ ">=", tag_cast(Tag::GE) }, 
        Word::True{ "true", tag_cast(Tag::TRUE) }, 
        Word::False{ "false", tag_cast(Tag::FALSE) };

    void Lexer::reserve(Word w) {

        words_.emplace(w.lexeme_, std::move(w));
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

        words_.reserve(16);

        reserve(Word{ "if", tag_cast(Tag::IF) });
        reserve(Word{ "else", tag_cast(Tag::ELSE) });
        reserve(Word{ "while", tag_cast(Tag::WHILE) });
        reserve(Word{ "repeat", tag_cast(Tag::REPEAT) });
        reserve(Word{ "until", tag_cast(Tag::UNTIL) });
        reserve(Word{ "for", tag_cast(Tag::FOR) });
        reserve(Word{ "to", tag_cast(Tag::TO) });
        reserve(Word{ "downto", tag_cast(Tag::DOWNTO) });
        reserve(Word{ "break", tag_cast(Tag::BREAK) });
        reserve(Word{ "fun", tag_cast(Tag::FUN) });
        reserve(Word{ "let", tag_cast(Tag::LET) });
        reserve(Word{ "return", tag_cast(Tag::RETURN) });
        reserve(Word::True); reserve(Word::False);
        words_.emplace("print", Word{ "print", tag_cast(Tag::ID) });
        words_.emplace("read", Word{ "read", tag_cast(Tag::ID) });

        readch();
    }

    std::unique_ptr<Token> Lexer::scan() {

        if(new_ident_ < ident_) {
            --ident_;
            return std::make_unique<Token>(tag_cast(Tag::DEIDENT));
        }

        for(;; readch()) {

            if(peek_ == std::char_traits<char>::eof()) return nullptr;
            if(peek_ == ' ' || peek_ == '\t') continue;
            else break;
        }

        switch(peek_) {
            
            case '\n':
                {   
                    ++line_;
                    new_ident_ = 0;
                    for(; readch(), peek_ == '\t';) ++new_ident_;

                    if(new_ident_ == ident_) {

                        while(peek_ == ' ') readch();
                        if(peek_ == '\n') {

                            new_ident_ = 0;
                            if(ident_) --ident_;
                            return std::make_unique<Token>(tag_cast(Tag::DEIDENT));
                        }
                        
                        return scan();
                    }
                    else {
                        if(new_ident_ < ident_) {
                            --ident_;
                            return std::make_unique<Token>(tag_cast(Tag::DEIDENT));
                        }
                        else {
                            ident_ = new_ident_;
                            return std::make_unique<Token>(tag_cast(Tag::IDENT));
                        }
                    }
                }
            case '&':
                if(readch('&')) return std::make_unique<Word>(Word::And);
                else return std::make_unique<Token>('&');
            case '|':
                if(readch('|')) return std::make_unique<Word>(Word::Or);
                else return std::make_unique<Token>('|');
            case '=':
                if(readch('=')) return std::make_unique<Word>(Word::eq);
                else return std::make_unique<Token>('=');
            case '!':
                if(readch('=')) return std::make_unique<Word>(Word::ne);
                else return std::make_unique<Token>('!');
            case '<':
                if(readch('=')) return std::make_unique<Word>(Word::le);
                else return std::make_unique<Token>('<');
            case '>':
                if(readch('=')) return std::make_unique<Word>(Word::ge);
                else return std::make_unique<Token>('>');
        }
        if(isdigit_s(peek_)) {
            
            int v = 0;

            do {
                v = 10 * v + (peek_ - '0');
                readch();
            } while(isdigit_s(peek_));
            
            if(peek_ != '.') 
                return std::make_unique<Num>(static_cast<double>(v));
            double x = v; 
            double d = 10;

            for(; readch(), true;) {
                
                if(!isdigit_s(peek_)) break;
                x = x + (peek_ - '0') / d;
                d = d * 10;
            }

            return std::make_unique<Num>(x);
        }
        if(isalpha_s(peek_)) {
            
            std::stringstream ss{};

            do {
                ss << peek_;
                readch();
            } while(isalnum_s(peek_));

            std::string s = ss.str();
            if(auto w = words_.find(s); w != words_.end()) 
                return std::make_unique<Word>(w->second);

            auto w = std::make_unique<Word>(s, tag_cast(Tag::ID));
            words_.emplace(s, *w);

            return w;
        }

        auto tok = std::make_unique<Token>(peek_);
        peek_ = ' ';

        return tok;
    }
}