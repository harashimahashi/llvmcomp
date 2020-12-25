#include "ilex.h"
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
    Token::operator bool() noexcept {
        return static_cast<int>(tag_);
    }
    Token::operator int() noexcept {
        return static_cast<int>(tag_);
    }

    Num::Num(double v) noexcept : Token{ tag_cast(Tag::NUM) }, val_{ v } {}
    
    Word::Word(std::string s, int tag) : Token{ tag }, lexeme_{ s } {} 
    const std::unique_ptr<Word> 
        Word::And{ new Word("&&", tag_cast(Tag::AND)) },
        Word::Or{ new Word("||", tag_cast(Tag::OR)) },
        Word::eq{ new Word("==", tag_cast(Tag::EQ)) }, 
        Word::ne{ new Word("!=", tag_cast(Tag::NE)) }, 
        Word::le{ new Word("<=", tag_cast(Tag::LE)) }, 
        Word::ge{ new Word(">=", tag_cast(Tag::GE)) }, 
        Word::minus{ new Word("minus", tag_cast(Tag::MINUS)) }, 
        Word::True{ new Word("true", tag_cast(Tag::TRUE)) }, 
        Word::False{ new Word("false", tag_cast(Tag::FALSE)) }, 
        Word::temp{ new Word("t", tag_cast(Tag::TEMP)) };

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

    Lexer::Lexer(std::string s) : source_{ s } {

        words_.reserve(13);

        reserve(Word{ "if", tag_cast(Tag::IF) });
        reserve(Word{ "elseif", tag_cast(Tag::ELSEIF) });
        reserve(Word{ "else", tag_cast(Tag::ELSE) });
        reserve(Word{ "while", tag_cast(Tag::WHILE) });
        reserve(Word{ "repeat", tag_cast(Tag::REPEAT) });
        reserve(Word{ "until", tag_cast(Tag::UNTIL) });
        reserve(Word{ "for", tag_cast(Tag::FOR) });
        reserve(Word{ "to", tag_cast(Tag::TO) });
        reserve(Word{ "downto", tag_cast(Tag::DOWNTO) });
        reserve(Word{ "break", tag_cast(Tag::BREAK) });
        reserve(Word{ "fun", tag_cast(Tag::DECL) });
        reserve(*Word::True); reserve(*Word::False);
    }

    Token Lexer::scan(){

        for(;; readch()){

            if(peek_ == std::char_traits<char>::eof()) return Token{ 0 };
            if(peek_ == ' ' || peek_ == '\t') continue;
            else break;
        }

        switch(peek_) {
            
            case '\n':
                {   
                    unsigned prev{ ident_ };
                    unsigned cnt{ 0 };
                    readch();
                    for(; peek_ == '\t'; readch()) ++cnt;

                    if(std::abs((int)ident_ - (int)cnt) > 1) 
                        throw std::runtime_error{ "line " + std::to_string(line_) + ": ident_ation error\n" };
                    else if(cnt == ident_) return scan();
                    else {
                        if(cnt < ident_) {
                            --ident_;
                            return Token{ tag_cast(Tag::DEIDENT) };
                        }
                        else {
                            ++ident_;
                            return Token{ tag_cast(Tag::IDENT) };
                        }
                    }
                }
            case '&':
                if(readch('&')) return *Word::And;
                else return Token{ '&' };
            case '|':
                if(readch('|')) return *Word::Or;
                else return Token{ '|' };
            case '=':
                if(readch('=')) return *Word::eq;
                else return Token{ '=' };
            case '!':
                if(readch('=')) return *Word::ne;
                else return Token{ '!' };
            case '<':
                if(readch('=')) return *Word::le;
                else return Token{ '<' };
            case '>':
                if(readch('=')) return *Word::ge;
                else return Token{ '>' };
        }
        if(isdigit_s(peek_)) {
            
            int v = 0;

            do {
                v = 10 * v + (peek_ - '0');
                readch();
            } while(isdigit_s(peek_));
            
            if(peek_ != '.') return Num{ static_cast<double>(v) };
            double x = v; 
            double d = 10;

            for(;; readch()) {
                
                if(!isdigit_s(peek_)) break;
                x = x + (peek_ - '0') / d;
                d = d * 10;
            }

            return Num{ x };
        }
        if(isalpha_s(peek_)) {
            
            std::stringstream ss{};

            do {
                ss << peek_;
                readch();
            } while(isalnum_s(peek_));

            std::string s = ss.str();
            if(auto w = words_.find(s); w != words_.end()) return w->second;

            Word w{s, tag_cast(Tag::ID)};
            words_.emplace(s, w);

            return w;
        }

        Token tok{ peek_ };
        peek_ = ' ';

        return tok;
    }
}