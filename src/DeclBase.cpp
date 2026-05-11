#include "include/AST/DeclBase.hpp"

#include "llvm/Support/ErrorHandling.h"

namespace c {
namespace ast {

llvm::StringRef Decl::getDeclKindName() const {
    switch (DeclKind) {
        default:
            llvm_unreachable("Decl::Kind missing case.");
        
        case StructDeclKind:
            return "StructDecl";
        case TypeAliasDeclKind:
            return "TypeAliasDecl";
        
        case VarDeclKind:
            return "VarDecl";
        case ParamDeclKind:
            return "ParamDecl";
        case FieldDeclKind:
            return "FieldDecl";

        case FunctionDeclKind:
            return "FunctionDecl";

        case ModuleDeclKind:
            return "ModuleDecl";
    }
}

} // namespace ast
} // namespace c
