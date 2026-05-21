#include "include/CodeGen/CodeGenModule.hpp"

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constant.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include "include/AST/Expr.hpp"
#include "include/CodeGen/CodeGenFunction.hpp"

#include "src/Debug.hpp"

using namespace c::ast;
using namespace c::codegen;

// FIXME: Probably find a better way of doing this.
llvm::Constant* codegenGlobalVariableLiteralValue(CodeGenModule& cgm, Expr* expr) {
    DEBUG("Called: codegenGlobalVariableLiteralValue");
    if (llvm::isa<ArrayLiteral>(expr)) {
        auto lit = llvm::cast<ArrayLiteral>(expr);

        llvm::SmallVector<llvm::Constant*> items(lit->getAmntItems());
        for (Expr* item : lit->items())
            items.push_back(llvm::cast<llvm::Constant>(codegenGlobalVariableLiteralValue(cgm, item)));

        auto type = llvm::cast<llvm::ArrayType>(cgm.getTypes().convertType(lit->getType()));
        return llvm::ConstantArray::get(type, items);
    } else if (llvm::isa<IntegerLiteral>(expr)) {
        auto lit = llvm::cast<IntegerLiteral>(expr);
        return llvm::ConstantInt::get(
            cgm.getTypes().convertType(lit->getType()),
            lit->getValue()
        );
    } else if (llvm::isa<FloatingLiteral>(expr)) {
        auto lit = llvm::cast<FloatingLiteral>(expr);
        return llvm::ConstantFP::get(
            cgm.getTypes().convertType(lit->getType()),
            lit->getValue()
        );
    } else if (llvm::isa<BooleanLiteral>(expr))
        return llvm::ConstantInt::get(
            llvm::Type::getInt1Ty(cgm.getContext()),
            (uint8_t)llvm::cast<BooleanLiteral>(expr)->getValue()
        );
    
    llvm_unreachable("Currently only literals are allowed as values of global variables.");
}

namespace c {
namespace codegen {

void CodeGenModule::emitModule(ModuleDecl& module) {
    DEBUG("Called: emitModule");
    for (Decl* DC : module.decls()) 
        if (llvm::isa<ModuleDecl>(DC)) {
            llvm_unreachable("Module declarations other than the global module are currently not supported.");
            // emitModule(llvm::cast<ModuleDecl>(DC));
        } else if (llvm::isa<StructDecl>(DC)) {
            Types.convertType(llvm::cast<StructDecl>(DC)->getDeclType());
        } else if (llvm::isa<VarDecl>(DC)) {
            emitGlobalVariable(llvm::cast<VarDecl>(DC));
        } else if (llvm::isa<FunctionDecl>(DC)) {
            emitFunction(llvm::cast<FunctionDecl>(DC));
        } else if (!llvm::isa<TypeAliasDecl>(DC))
            // Just ignore TypeAliasDecl since they are aliases and 
            // because they are resolved during semantic analasys.
            llvm_unreachable("Decl not supported inside of a ModuleDecl.");
}

void CodeGenModule::emitGlobalVariable(VarDecl* decl) {
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

void CodeGenModule::emitFunction(FunctionDecl* decl) {
    DEBUG("Called: emitFunction");
    llvm::SmallVector<llvm::Type*> params;
    params.reserve(decl->getAmntParams());
    for (ParamDecl* a : decl->parameters())
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

} // namespace codegen
} // namespace c 

