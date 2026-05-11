#include "include/CodeGen/CodeGenModule.hpp"

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constant.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include "include/AST/Expr.hpp"
#include "include/CodeGen/CodeGenFunction.hpp"
#include "include/CodeGen/CodeGenTypes.hpp"

#include "src/Debug.hpp"

namespace c {
namespace codegen {

llvm::Constant* codegenGlobalVariableLiteralValue(CodeGenModule& cgm, ast::Expr* expr);

void CodeGenModule::emitModule(ast::ModuleDecl& module) {
    DEBUG("Called: emitModule");
    for (ast::Decl* DC : module.decls()) 
        if (llvm::isa<ast::ModuleDecl>(DC)) {
            llvm_unreachable("Module declarations other than the global module are currently not supported.");
            // emitModule(llvm::cast<ast::ModuleDecl>(DC));
        } else if (llvm::isa<ast::StructDecl>(DC)) {
            Types.convertType(llvm::cast<ast::StructDecl>(DC)->getDeclType());
        } else if (llvm::isa<ast::VarDecl>(DC)) {
            emitGlobalVariable(llvm::cast<ast::VarDecl>(DC));
        } else if (llvm::isa<ast::FunctionDecl>(DC)) {
            emitFunction(llvm::cast<ast::FunctionDecl>(DC));
        } else if (!llvm::isa<ast::TypeAliasDecl>(DC))
            // Just ignore TypeAliasDecl since they are aliases and 
            // because they are resolved during semantic analasys.
            llvm_unreachable("Decl not supported inside of a ModuleDecl.");
}

void CodeGenModule::emitGlobalVariable(ast::VarDecl* decl) {
    DEBUG("Called: emitGlobalVariable");
    llvm::GlobalVariable* glob = new llvm::GlobalVariable( // FIXME: I don't like "new"
        Module,
        Types.convertType(decl->getType()),
        decl->isConstant(),
        llvm::GlobalValue::ExternalLinkage,
        codegenGlobalVariableLiteralValue(*this, decl->getInit()),
        decl->getName()
    );

    (void)Globals.insert({decl, std::move(glob)});
}

void CodeGenModule::emitFunction(ast::FunctionDecl* decl) {
    DEBUG("Called: emitFunction");
    llvm::SmallVector<llvm::Type*> params;
    params.reserve(decl->getAmntParams());
    for (ast::ParamDecl* a : decl->parameters())
        params.push_back(Types.convertType(a->getType()));

    llvm::Type* ret_ty = decl->isVoid()
        ? llvm::Type::getVoidTy(getContext())
        : Types.convertType(decl->getType());
    llvm::FunctionType* fm_ty = llvm::FunctionType::get(ret_ty, params, false);

    llvm::Function* fn = llvm::Function::Create(
        fm_ty,
        llvm::Function::ExternalLinkage,
        decl->getName(),
        getModule()
    );

    CodeGenFunction cgf(*this, fn);
    cgf.emit(*decl);

    (void)Functions.insert({decl, std::move(fn)});
}

llvm::Constant* codegenGlobalVariableLiteralValue(CodeGenModule& cgm, ast::Expr* expr) {
    DEBUG("Called: codegenGlobalVariableLiteralValue");
    if (llvm::isa<ast::ArrayLiteral>(expr)) {
        auto lit = llvm::cast<ast::ArrayLiteral>(expr);

        llvm::SmallVector<llvm::Constant*> items(lit->getAmntItems());
        for (ast::Expr* item : lit->items())
            items.push_back(llvm::cast<llvm::Constant>(codegenGlobalVariableLiteralValue(cgm, item)));

        auto type = llvm::cast<llvm::ArrayType>(cgm.getTypes().convertType(lit->getType()));
        return llvm::ConstantArray::get(type, items);
    } else if (llvm::isa<ast::IntegerLiteral>(expr)) {
        auto lit = llvm::cast<ast::IntegerLiteral>(expr);
        return llvm::ConstantInt::get(
            cgm.getTypes().convertType(lit->getType()),
            lit->getValue()
        );
    } else if (llvm::isa<ast::FloatingLiteral>(expr)) {
        auto lit = llvm::cast<ast::FloatingLiteral>(expr);
        return llvm::ConstantFP::get(
            cgm.getTypes().convertType(lit->getType()),
            lit->getValue()
        );
    } else if (llvm::isa<ast::BooleanLiteral>(expr))
        return llvm::ConstantInt::get(
            llvm::Type::getInt1Ty(cgm.getContext()),
            (uint8_t)llvm::cast<ast::BooleanLiteral>(expr)->getValue()
        );
    
    llvm_unreachable("Currently only literals are allowed as values of global variables.");
}

} // namespace codegen
} // namespace c 

