#ifndef LLVMC_ISYMBOLS_H_
#define LLVMC_ISYMBOLS_H_
#include "llvm/IR/Value.h"
#include <llvmc/iinter.h>

namespace llvmc {

    namespace symbols {

        class Env {

            std::unordered_map<std::string, std::shared_ptr<inter::Id>> table_;
            std::shared_ptr<Env> prev_;

        public:

            Env(std::shared_ptr<Env>);

            void insert(std::string, std::shared_ptr<inter::Id>);
            std::shared_ptr<inter::Id> get(std::string);
            std::shared_ptr<inter::Id> get_current(std::string);

        };
    }
}
#endif