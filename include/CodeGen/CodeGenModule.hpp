#pragma once

#include <type_traits>

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include "include/AST/Decl.hpp"
#include "include/AST/Type.hpp"
#include "include/CodeGen/CodeGenTypes.hpp"

namespace c {
namespace codegen {

class CodeGenModule {
private:
    llvm::LLVMContext Context;
    std::unique_ptr<llvm::Module> Module;

    CodeGenTypes Types;

    llvm::DenseMap<ast::VarDecl*, llvm::GlobalVariable*> Globals;
    llvm::DenseMap<ast::FunctionDecl*, llvm::Function*> Functions;
public:
    CodeGenModule() = default;
    ~CodeGenModule() = default;

    inline llvm::LLVMContext& getLLVMContext() {
        return Context;
    }

    inline llvm::Module& getModule() {
        return *Module;
    }

    inline CodeGenTypes& getTypes() {
        return Types;
    }

    llvm::Function* getFunction(const ast::FunctionDecl* decl) {
        return Functions.lookup(decl);
    }

    llvm::GlobalVariable* getGlobalVariable(const ast::VarDecl* decl) {
        return Globals.lookup(decl);
    }

    void emitGlobalVariable(ast::VarDecl* decl) {
        llvm::GlobalVariable* glob = &llvm::GlobalVariable(
            *Module,
            Types.convertType(decl->getType()),
            decl->isConstant(),
            llvm::GlobalValue::ExternalLinkage,
            condegenGlobalVariableLiteralValue(decl->getInit()),
            decl->getName()
        );

        (void)Globals.insert({decl, std::move(glob)});
    }

    void emitFunction(ast::FunctionDecl* decl) {
        llvm::SmallVector<llvm::Type*> params(decl->getAmntParams());
        for (ast::ParamDecl* a : decl->parameters())
            params.push_back(Types.convertType(a->getType()));

        llvm::Type* rt = Types.convertType(decl->getType());
        llvm::FunctionType* ft = llvm::FunctionType::get(rt, params, false);

        llvm::Function* fn = llvm::Function::Create(
            ft,
            llvm::Function::ExternalLinkage,
            decl->getName(),
            getModule()
        );

        (void)Functions.insert({decl, std::move(fn)});
     }
private:
    llvm::Constant* condegenGlobalVariableLiteralValue(ast::Expr* expr) {
        if (llvm::isa<ast::ArrayLiteral>(expr)) {
            ast::ArrayLiteral* lit = llvm::cast<ast::ArrayLiteral>(expr);

            llvm::SmallVector<llvm::Constant*> items(lit->getAmntItems());
            for (ast::Expr* item : lit->items())
                items.push_back(llvm::cast<llvm::Constant>(condegenGlobalVariableLiteralValue(item)));

            auto type = llvm::cast<llvm::ArrayType>(Types.convertType(lit->getType()));
            return llvm::ConstantArray::get(type, items);
        } else if (llvm::isa<ast::IntegerLiteral>(expr)) {
            ast::IntegerLiteral* lit = llvm::cast<ast::IntegerLiteral>(expr);
            return llvm::ConstantInt::get(
                Types.convertType(lit->getType()),
                lit->getValue()
            );
        } else if (llvm::isa<ast::FloatingLiteral>(expr)) {
            ast::FloatingLiteral* lit = llvm::cast<ast::FloatingLiteral>(expr);
            return llvm::ConstantFP::get(
                Types.convertType(lit->getType()),
                lit->getValue()
            );
        } else if (llvm::isa<ast::BooleanLiteral>(expr)) {
            return llvm::ConstantInt::get(
                llvm::Type::getInt1Ty(Context),
                (uint8_t)llvm::cast<ast::BooleanLiteral>(expr)->getValue()
            );
        }
        
        llvm_unreachable("Currently only literals are allowed as values of global variables.");
    }
};

} // namepspace codegen
} // namespace c
