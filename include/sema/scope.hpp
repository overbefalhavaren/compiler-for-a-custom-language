#pragma once

#include "llvm/ADT/StringMap.h"

#include "include/ast/stmt.hpp"

namespace c {
namespace sema {

// Forward declarations since intellisense is retarded
class Scope;
class TopLevelScope;

class Scope {
public:
    enum ScopeFlag {
        NoScope,
        FunctionScope,
        FunctionDeclScope,
        ControllScope,
        StructScope,
        EnumScope,
    };
private:
    // The parent scope of this scope. Null if parent is the global scope.
    Scope* Parent;

    ScopeFlag Flag;

    // How nested the scope is. 0 if parent is the global scope.
    uint16_t Depth;

    llvm::StringMap<llvm::SmallVector<ast::DeclStmt*>> Symbols;
public:
    Scope() = default;
    Scope(Scope* parent, ScopeFlag flag = NoScope)
        : Parent(parent), Depth() {}
    ~Scope() = default;

    inline bool isGlobalScope() const {
        return Parent == nullptr;
    }

    inline bool hasParent() const {
        return Parent != nullptr;
    }

    inline const Scope* getParent() const {
        assert(hasParent() && "The global scope has no parent.");
        return Parent;
    }

    inline uint8_t getDepth() const {
        return Depth;
    }

    ScopeFlag getFlag() const {
        return Flag;
    }

    bool pushDecl(ast::DeclStmt* stmt) {
        
    }

    std::optional<ast::VarDecl*> lookupVariable(llvm::StringRef name) {
        return lookup<ast::VarDecl>(name);
    }

    std::optional<ast::TypeExpr*> lookupType(llvm::StringRef name) {
        std::optional<ast::TypeDecl*> result = lookup<ast::TypeDecl>(name);
        if (result.has_value())
            return result.value()->getTypeExpr().get();
        return std::nullopt;
    }

    std::optional<ast::FunctionDecl*> lookupFunction(llvm::StringRef name) {
        return lookup<ast::FunctionDecl>(name);
    }
protected:
    template <typename T>
    std::optional<T*> lookup(llvm::StringRef name) {
        static_assert(std::is_base_of<ast::DeclStmt, T>::value, "T must be a subclass of DeclStmt.");
        
        auto item = Symbols.find(name);
        if (item == Symbols.end())
            return std::nullopt;

        for (ast::DeclStmt* stmt : item->second)
            if (stmt.isa(T::ClassID))
                return stmt;

        return std::nullopt;
    }
};

class TopLevelScope : public Scope {
private:
    llvm::StringMap<const ast::FunctionDecl*> Functions;
    llvm::StringMap<const ast::StructDecl*> Structs;
public:
    TopLevelScope(Scope* parent)
        : Scope(parent) {}

    bool pushDecl(const ast::DeclStmt* stmt) {
        if (ast::isa<ast::TypeDecl>(stmt)) {
            Types.insert(cast<ast::TypeDecl>(stmt));
        } else if (ast::isa<ast::VarDecl>(stmt)) {
            Variables.insert(cast<ast::VarDecl>(stmt));
        } else if (ast::isa<ast::FunctionDecl>(stmt)) {
            Functions.insert(cast<ast::FunctionDecl>(stmt));
        } else if (ast::isa<ast::StructDecl>(stmt))
            Structs.insert(cast<ast::StructDecl>(stmt));
        return false;
    }

    std::optional<ast::StructDecl*> lookupStruct(llvm::StringRef name) {
        return lookup<ast::StructDecl>(name);
    }
};

} // namespace sema
} // namespace c
 