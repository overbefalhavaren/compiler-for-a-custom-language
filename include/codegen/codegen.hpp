#pragma once

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"

namespace c {
namespace codegen {

class CodeGenerator {
protected:
    static llvm::LLVMContext& ctx() {
        static llvm::LLVMContext ctx = llvm::LLVMContext();
        return ctx;
    }
};

}
}
