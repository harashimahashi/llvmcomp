#ifndef LLVMC_ISYMBOLS_H_
#define LLVMC_ISYMBOLS_H_
#include "llvm/IR/Value.h"

namespace llvmc {

    namespace symbols {

        class Env {

            std::unordered_map<std::string, llvm::Value*> table_;
            std::shared_ptr<Env> prev_;

        public:

            Env(std::unique_ptr<Env>);

            void insert(std::string, llvm::Value*);
            llvm::Value* get(std::string);

        };
    }
}
#endif