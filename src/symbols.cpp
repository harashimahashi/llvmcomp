#include <llvmc/isymbols.h>

namespace llvmc {

    namespace symbols {

        Env::Env(std::shared_ptr<Env> n) : prev_{ std::move(n) } {}
        void Env::insert(std::string_view l, std::shared_ptr<inter::Id> i) {
            
            table_.emplace(std::move(l), std::move(i));
        }
        std::shared_ptr<inter::Id> Env::get_current(std::string_view t) {

            auto found = table_.find(t);
            if(found != table_.end()) return found->second;

            return nullptr;
        }
        std::shared_ptr<inter::Id> Env::get(std::string_view t) {

            for(Env* e = this; e != nullptr; e = e->prev_.get()) {
                
                auto ret = e->get_current(t);
                if(ret) return ret;
            }

            return nullptr;
        }
    }
}