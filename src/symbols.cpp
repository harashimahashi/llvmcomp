#include <llvmc/isymbols.h>

namespace llvmc {

    namespace symbols {

        Env::Env(std::unique_ptr<Env> n) : prev_{ std::move(n) } {}
        void Env::insert(std::string l, llvm::Value* i) {
            
            table_.emplace(std::move(l), std::move(i));
        }
        llvm::Value* Env::get(std::string t) {

            for(Env* e = this; e != nullptr; e = &*(e->prev_)) {
                auto found = table_.find(t);
                if(found != table_.end()) return found->second; 
            }
            return nullptr;
        }

    }
}