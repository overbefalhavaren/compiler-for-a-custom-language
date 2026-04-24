#pragma once

#include <type_traits>

#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/Allocator.h"

#include "include/AST/Type.hpp"

namespace c {

class ASTAllocator {
private:
    llvm::BumpPtrAllocator Allocator;

    llvm::DenseMap<BuiltinType::BuiltinKind, BuiltinType*> Builtins;
public:
    ASTAllocator() = default;
    ~ASTAllocator() = default;

    void createBuiltinTypes() {
        uint8_t amnt_builtins = BuiltinType::getAmntBuiltinTypes();
        Builtins.grow(amnt_builtins);
        for (uint8_t k = 0; k < amnt_builtins; k++) {
            auto kind = static_cast<BuiltinType::BuiltinKind>(k);
            Builtins.insert({kind, new BuiltinType(kind)}); // FIXME: I don't like "new"
        }
    }

    const BuiltinType* getBuiltinType(BuiltinType::BuiltinKind kind) {
        assert(!Builtins.empty() && "Builtins were never created. Call ASTAllocator::createBuiltinTypes()");
        return Builtins.lookup(kind);
    }

    void* Allocate(size_t amnt, size_t align) {
        return Allocator.Allocate(amnt, align);
    }

    template <typename T>
    T* Allocate(size_t amnt) {
        return Allocator.Allocate(amnt, sizeof(T));
    }

    template <typename T, typename... Args>
    T* Create(Args&&... args) {
        T* mem = (T*)Allocator.Allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }
};

} // namespace c
