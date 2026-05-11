#pragma once

#include <type_traits>

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "include/AST/Decl.hpp"
#include "include/CodeGen/CodeGenTypes.hpp"

namespace c {
namespace codegen {

class CodeGenModule {
private:
    llvm::LLVMContext& Context;
    llvm::Module& Module;

    CodeGenTypes Types;

    llvm::DenseMap<ast::VarDecl*, llvm::GlobalVariable*> Globals;
    llvm::DenseMap<ast::FunctionDecl*, llvm::Function*> Functions;
public:
    CodeGenModule(llvm::Module& module, llvm::LLVMContext& ctx) 
        : Module(module), Context(ctx), Types(*this) {}
    ~CodeGenModule() = default;

    llvm::LLVMContext& getContext() {
        return Context;
    }

    llvm::Module& getModule() {
        return Module;
    }

    CodeGenTypes& getTypes() {
        return Types;
    }

    llvm::Function* getFunction(const ast::FunctionDecl* decl) {
        return Functions.lookup(decl);
    }

    llvm::GlobalVariable* getGlobalVariable(const ast::VarDecl* decl) {
        return Globals.lookup(decl);
    }

    void emitModule(ast::ModuleDecl& module);

    void emitGlobalVariable(ast::VarDecl* decl);

    void emitFunction(ast::FunctionDecl* decl);
};

} // namepspace codegen
} // namespace c
