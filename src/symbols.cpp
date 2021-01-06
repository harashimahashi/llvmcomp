#include <llvmc/isymbols.h>

namespace symbols {

    using namespace llvm;

    Env::Env(Env* n) : prev_{ n } {}
    void Env::insert(std::unique_ptr<lexer::Word> t, Value* i) {
        
        table_.emplace(std::move(t), std::move(i));
    }
    Value* Env::get(std::unique_ptr<lexer::Word> t) {

        for(Env* e = this; e != nullptr; e = &*(e->prev_)) {
            auto found = table_.find(t);
            if(found != table_.end()) return found->second; 
        }
        return nullptr;
    }

}