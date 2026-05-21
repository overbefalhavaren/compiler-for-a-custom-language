#pragma once

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"

#include "include/AST/Decl.hpp"
#include "include/AST/Expr.hpp"
#include "include/AST/Stmt.hpp"
#include "include/CodeGen/CodeGenModule.hpp"
#include "include/CodeGen/Value.hpp"

namespace c {
namespace codegen {

class CodeGenFunction {
private:
    CodeGenModule& CGM;
    llvm::IRBuilder<> Builder = llvm::IRBuilder<>(CGM.getContext());

    llvm::Function* Fn = nullptr;
    llvm::DenseMap<const ast::InitDecl*, Place> Locals;
public:
    CodeGenFunction(CodeGenModule& cgm, llvm::Function* fn) 
        : CGM(cgm), Builder(cgm.getContext()), Fn(fn) {}
    
    llvm::IRBuilder<>& getBuilder() {
        return Builder;
    }

    std::optional<Place> lookup(const ast::InitDecl* DC);

    void emit(const ast::FunctionDecl& DC);

    void emitVarDecl(const ast::VarDecl& DC);
    void emitParamDecl(const ast::ParamDecl& DC, llvm::Argument* Arg);

    void emitStmt(const ast::Stmt* ST);
    void emitIfStmt(const ast::IfStmt& ST);
    void emitWhileStmt(const ast::WhileStmt& ST);
    void emitReturnStmt(const ast::ReturnStmt& ST);

    void emitBlockStmt(const ast::BlockStmt& ST);

    void emitStore(Place ptr, Value val);

    Value emitLoad(Place ptr);

    Value emitExpr(const ast::Expr* EX);
    Place emitPlace(const ast::Expr* EX);

    llvm::Value* emitCondition(const ast::Expr* EX);

    Value emitBinaryOperator(const ast::BinaryOperator& EX);
    Value emitBinaryAssign(const ast::BinaryOperator& EX);
    llvm::Value* emitStructComparison(Value& RHS, Value& LHS);

    Value emitUnaryOperator(const ast::UnaryOperator& EX);
    Value emitUnaryAssign(const ast::UnaryOperator& EX);
    Place emitDereference(const ast::UnaryOperator& EX);

    Value emitConstantArray(const ast::ArrayLiteral& EX);
    Value emitArrayLiteral(const ast::ArrayLiteral& EX);

    Value emitCallExpr(const ast::CallExpr& EX);
    Place emitConstructCall(const ast::CallExpr& EX);

    Place emitDeclRef(const ast::DeclRefExpr& EX);

    Place emitAccessExpr(const ast::AccessExpr& EX);
    Place emitSliceExpr(const ast::SliceExpr& EX, bool doBoundsCheck);
};

} // codegen
} // c
