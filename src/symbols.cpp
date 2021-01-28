#include <llvmc/isymbols.h>

namespace llvmc {

    namespace symbols {

        Env::Env(std::unique_ptr<Env> n) : prev_{ std::move(n) } {}
        void Env::insert(std::string l, std::unique_ptr<inter::Id> i) {
            
            table_.emplace(std::move(l), std::move(i));
        }
        std::shared_ptr<inter::Id> Env::get(std::string t) {

            for(Env* e = this; e != nullptr; e = e->prev_.get()) {
                auto found = table_.find(t);
                if(found != table_.end()) return found->second; 
            }
            return nullptr;
        }

    }
}