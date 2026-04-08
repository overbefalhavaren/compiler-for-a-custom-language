#pragma once

#include <type_traits>

#include "llvm/Support/Allocator.h"

namespace c {

class ASTAllocator {
private:
    llvm::BumpPtrAllocator Allocator;
public:
    ASTAllocator() = default;
    ~ASTAllocator() = default;

    void* Allocate(size_t amnt, size_t size) {

    }

    template <typename T>
    T* Allocate(size_t amnt) {
        return static_cast<T*>(Allocate(amnt, sizeof(T)));
    }

    template <typename T, typename... Args>
    T* Create(Args&&... args) {
        T init = T(std::forward<Args>(args)...);
        return Create<T>(std::move(init));
    }

    template <typename T>
    T* Create(T&& init) {

    }
};

} // namespace c
