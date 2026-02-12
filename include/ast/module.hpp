#pragma once

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"

#include "include/ast/stmt.hpp"

namespace c {

class Module {
private:
    FileID ID;
    llvm::SmallVector<std::string> Export;
    llvm::SmallVector<std::unique_ptr<ast::Stmt>> Stmts;
public:
    Module() = default;
    Module(FileID id, llvm::SmallVector<std::unique_ptr<ast::Stmt>>&& stmts)
        : ID(id), Stmts(std::move(stmts)) {}
    ~Module() = default;

    Module(Module&&) = default;
    Module& operator=(Module&&) = default;

    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;

    inline bool hasPublicAttr(llvm::StringRef name) const {
        for (const std::string& attr : Export)
            if (attr == name) 
                return true;
        return false;
    }

    inline const llvm::SmallVector<std::unique_ptr<ast::Stmt>>& getStmts() const {
        return Stmts;
    }
};

} // namespace c
