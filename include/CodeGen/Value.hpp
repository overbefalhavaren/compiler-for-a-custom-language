#pragma once

#include "llvm/IR/Value.h"

#include "include/AST/Type.hpp"

namespace c {
namespace codegen {

class Place {
private:
    llvm::Value* Ptr;
    const Type* Ty;
public:
    Place(llvm::Value* Ptr, const Type* Ty)
        : Ptr(Ptr), Ty(Ty) {}
    ~Place() = default;

    llvm::Value* getPointer() {
        return Ptr;
    }

    const Type* getType() const {
        return Ty;
    }
};

class Value {
private:
    llvm::Value* Val;
public:
    Value(llvm::Value* Val)
        : Val(Val) {}

    llvm::Value* getValue() {
        return Val;
    }
};

} // namespace codegen
} // namespace c
