#pragma once

#include <cassert>

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Allocator.h"

#include "include/AST/Type.hpp"

namespace c {

class ASTAllocator {
private:
    llvm::BumpPtrAllocator Allocator = {};
    llvm::SmallVector<BuiltinType> Builtins = {};
public:
    ASTAllocator() = default;
    ~ASTAllocator() = default;

    void createBuiltinTypes() {
        BuiltinType::BuiltinKind first = BuiltinType::firstBuiltinKind;
        BuiltinType::BuiltinKind last = BuiltinType::lastBuiltinKind;

        Builtins.reserve(last + 1);
        for (uint8_t k = first; k <= last; k++) {
            auto kind = static_cast<BuiltinType::BuiltinKind>(k);
            assert(kind <= last && kind >= first);
            Builtins.push_back(BuiltinType(kind));
        }
    }

    const BuiltinType* getBuiltinType(BuiltinType::BuiltinKind kind) const {
        assert(!Builtins.empty() && "Builtins were never created. Call ASTAllocator::createBuiltinTypes()");
        assert(kind <= Builtins.size());
        auto result = &Builtins[kind];
        assert(result->getBuiltinKind() == kind);
        return result;
    }

    const BuiltinType* getIntegerType() const {
        return getBuiltinType(BuiltinType::I32);
    }

    const BuiltinType* getFloatingType() const {
        return getBuiltinType(BuiltinType::I32);
    }

    const BuiltinType* getBooleanType() const {
        return getBuiltinType(BuiltinType::Bool);
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
