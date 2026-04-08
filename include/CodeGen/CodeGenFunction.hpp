#pragma once

#include <type_traits>

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include "include/AST/Decl.hpp"
#include "include/AST/Stmt.hpp"
#include "include/CodeGen/CodeGenModule.hpp"

namespace c {
namespace codegen {

class LValue {
private:
    llvm::Value* Addr;
    const Type* Ty;
public:
    LValue(llvm::Value* addr, const Type* T)
        : Addr(addr), Ty(T) {}

    llvm::Value* getAddr() const {
        return Addr;
    }

    const Type* getType() const {
        return Ty;
    }

    llvm::Type* getLLVMType() const {
        return Addr->getType();
    }
};

class CodeGenFunction {
private:
    CodeGenModule& CGM;
    
    llvm::Function* Fn;
    llvm::IRBuilder<> Builder;

    llvm::DenseMap<ast::VarDecl*, llvm::Value*> Locals;
public:
    CodeGenFunction(CodeGenModule& cgm, llvm::Function* fn) 
        : CGM(cgm), Fn(fn), Builder(cgm.getLLVMContext()) {}

    void emitFnBody(ast::FunctionDecl* decl) {
        auto build_block = llvm::BasicBlock::Create(
            CGM.getLLVMContext(), "entry", Fn
        );

        Builder.SetInsertPoint(build_block);

        for (ast::Stmt* stmt : decl->getBody()->stmts())


        if (!build_block->getTerminator())
            (void)Builder.CreateRetVoid();
    }

    LValue lookup(ast::VarDecl* decl) {
        auto local = Locals.find(decl);
        if (local != Locals.end())
            return LValue(local->second, decl->getType());

        if (llvm::Value* global = CGM.getGlobalVariable(decl))
            return LValue(global, decl->getType());

        llvm_unreachable("If this is reached it indicates a problem with how Sema resolves variables.");
    }

    void emitStmt(ast::Stmt* stmt) {
        if (llvm::isa<ast::IfStmt>(stmt)) {
            emitIfStmt(llvm::cast<ast::IfStmt>(stmt));
        } else if (llvm::isa<ast::WhileStmt>(stmt)) {
            emitWhileStmt(llvm::cast<ast::WhileStmt>(stmt));
        } else if (llvm::isa<ast::ReturnStmt>(stmt)) {
            emitReturnStmt(llvm::cast<ast::ReturnStmt>(stmt));
        } else if (auto dc = llvm::dyn_cast<ast::DeclStmt>(stmt)) {
            if (!llvm::isa<ast::VarDecl>(dc->getDecl()))
                return;

            ast::VarDecl* var = llvm::cast<ast::VarDecl>(dc->getDecl());
            llvm::Type* type = CGM.getTypes().convertType(var->getType());
            llvm::Value* ptr = Builder.CreateAlloca(type, nullptr, var->getName());

            (void)Locals.insert({var, std::move(ptr)});
        }
    }

    llvm::Value* emitExpr(ast::Expr* expr) {
        if (expr->isLValue()) {
            return emitLValue(expr).getAddr();
        } else
            return emitRValue(expr);
    }

    llvm::Value* emitRValue(ast::Expr* expr) {
        if (llvm::isa<ast::BinaryOperator>(expr)) {
            return emitBinaryOperator(llvm::cast<ast::BinaryOperator>(expr));
        } else if (llvm::isa<ast::UnaryOperator>(expr)) {
            return emitUnaryOperator(llvm::cast<ast::UnaryOperator>(expr));
        } else if (llvm::isa<ast::CallExpr>(expr)) {
            return emitCall(llvm::cast<ast::CallExpr>(expr));
        } else if (llvm::isa<ast::ArrayLiteral>(expr)) {
            ast::ArrayLiteral* lit = llvm::cast<ast::ArrayLiteral>(expr);

            llvm::SmallVector<llvm::Constant*> items(lit->getAmntItems());
            for (ast::Expr* item : lit->items())
                items.push_back(llvm::cast<llvm::Constant>(emitRValue(item)));

            auto type = static_cast<llvm::ArrayType*>(CGM.getTypes().convertType(lit->getType()));
            return llvm::ConstantArray::get(type, items);
        } else if (llvm::isa<ast::IntegerLiteral>(expr)) {
            ast::IntegerLiteral* lit = llvm::cast<ast::IntegerLiteral>(expr);
            return llvm::ConstantInt::get(
                CGM.getTypes().convertType(lit->getType()),
                lit->getValue()
            );
        } else if (llvm::isa<ast::FloatingLiteral>(expr)) {
            ast::FloatingLiteral* lit = llvm::cast<ast::FloatingLiteral>(expr);
            return llvm::ConstantFP::get(
                CGM.getTypes().convertType(lit->getType()),
                lit->getValue()
            );
        } else if (llvm::isa<ast::BooleanLiteral>(expr)) {
            return llvm::ConstantInt::get(
                Builder.getInt1Ty(),
                (uint8_t)llvm::cast<ast::BooleanLiteral>(expr)->getValue()
            );
        }

        // Will also trigger if a new RValue Expr subclass was
        // created and not implemented in this function
        llvm_unreachable("Non-RValue expression was passed");
    }

    LValue emitLValue(ast::Expr* expr) {
        if (llvm::isa<ast::VarDeclRef>(expr)) {
            return lookup(llvm::cast<ast::VarDeclRef>(expr)->getDecl());
        } else if (llvm::isa<ast::UnaryOperator>(expr)) {
            ast::UnaryOperator* op = llvm::cast<ast::UnaryOperator>(expr);
            assert(op->isDerefOp() && "Only pointer dereference produces an lvalue.");

            return LValue(emitRValue(op->getSubExpr()), expr->getType());
        } else if (llvm::isa<ast::AccessExpr>(expr)) {
            ast::AccessExpr* e = llvm::cast<ast::AccessExpr>(expr);
            LValue base = emitLValue(e->getBase());

            llvm::Value* ptr = Builder.CreateStructGEP(
                base.getLLVMType(),
                base.getAddr(),
                e->getFieldDecl()->getFieldIndex()
            );

            return LValue(ptr, expr->getType());
        } else if (llvm::isa<ast::SliceExpr>(expr)) {
            ast::SliceExpr* e = llvm::cast<ast::SliceExpr>(expr);
            LValue base = emitLValue(e->getBase());
            llvm::Value* idx = emitRValue(e->getStart());

            llvm::Value* ptr = Builder.CreateGEP(
                base.getLLVMType(),
                base.getAddr(),
                idx
            );

            return LValue(ptr, expr->getType());
        }

        // Will also trigger if a new LValue expression has been 
        // added but not implemented in this function.
        llvm_unreachable("Non-LValue expression was passed.");
    }
private:
    llvm::Value* emitComparison(ast::BinaryOperator::OpKind op, const Type* type, llvm::Value* lhs, llvm::Value* rhs) {
        switch (op) {
            case ast::BinaryOperator::Equal:
                if (type->isIntegerTy() || type->isPointerTy()) {
                    return Builder.CreateICmpEQ(lhs, rhs);
                } else if (type->isFloatingTy()) {
                    return Builder.CreateFCmpOEQ(lhs, rhs);
                }else if (type->isStructTy()) 
                    return emitStructComparison(llvm::cast<StructType>(type), lhs, rhs);
            case ast::BinaryOperator::NotEqual:
                if (type->isIntegerTy() || type->isPointerTy()) {
                    return Builder.CreateICmpNE(lhs, rhs);
                } else if (type->isFloatingTy()) {
                    return Builder.CreateFCmpONE(lhs, rhs);
                }else if (type->isStructTy()) 
                    return Builder.CreateNot(
                        emitStructComparison(llvm::cast<StructType>(type), lhs, rhs)
                    );
            case ast::BinaryOperator::MoreThan:
                if (type->isIntegerTy()) {
                    return llvm::cast<BuiltinType>(type)->isUnsigned()
                        ? Builder.CreateICmpSGT(lhs, rhs)
                        : Builder.CreateICmpUGT(lhs, rhs);
                } else if (type->isFloatingTy()) {
                    return Builder.CreateFCmpOGT(lhs, rhs);
                }
            case ast::BinaryOperator::MTEqual:
                if (type->isIntegerTy()) {
                    return llvm::cast<BuiltinType>(type)->isUnsigned()
                        ? Builder.CreateICmpSGE(lhs, rhs)
                        : Builder.CreateICmpUGE(lhs, rhs);
                } else if (type->isFloatingTy()) {
                    return Builder.CreateFCmpOGE(lhs, rhs);
                }
            case ast::BinaryOperator::LessThan:
                if (type->isIntegerTy()) {
                    return llvm::cast<BuiltinType>(type)->isUnsigned()
                        ? Builder.CreateICmpSLT(lhs, rhs)
                        : Builder.CreateICmpULT(lhs, rhs);
                } else if (type->isFloatingTy()) {
                    return Builder.CreateFCmpOLT(lhs, rhs);
                }
            case ast::BinaryOperator::LTEqual:
                if (type->isIntegerTy()) {
                    return llvm::cast<BuiltinType>(type)->isUnsigned()
                        ? Builder.CreateICmpSLE(lhs, rhs)
                        : Builder.CreateICmpULE(lhs, rhs);
                } else if (type->isFloatingTy()) {
                    return Builder.CreateFCmpOLE(lhs, rhs);
                }
        }
    }

    llvm::Value* emitStructComparison(const StructType* type, llvm::Value* lhs, llvm::Value* rhs) {
        llvm::Value* result;

        size_t i = 0;
        for (const ast::FieldDecl* field : type->getDecl()->fields()) {
            llvm::Value* lhs_field = Builder.CreateExtractValue(lhs, i);
            llvm::Value* rhs_field = Builder.CreateExtractValue(rhs, i);

            llvm::Value* cmp = emitComparison(
                ast::BinaryOperator::Equal, field->getType(), lhs_field, rhs_field
            );

            result = result != nullptr ? Builder.CreateAnd(result, cmp) : cmp;
        }

        return result;
    }

    llvm::Value* emitArithmetic(ast::BinaryOperator::OpKind op, const Type* type, llvm::Value* lhs, llvm::Value* rhs) {
        switch (op) {
            case ast::BinaryOperator::Add:
            case ast::BinaryOperator::AddAssign:
                if (type->isIntegerTy()) {
                    return Builder.CreateAdd(lhs, rhs);
                } else if (type->isFloatingTy()) {
                    return Builder.CreateFAdd(lhs, rhs);
                }
            case ast::BinaryOperator::Sub:
            case ast::BinaryOperator::SubAssign:
                if (type->isIntegerTy()) {
                    return Builder.CreateSub(lhs, rhs);
                } else if (type->isFloatingTy()) {
                    return Builder.CreateFSub(lhs, rhs);
                }
            case ast::BinaryOperator::Mul:
            case ast::BinaryOperator::MulAssign:
                if (type->isIntegerTy()) {
                    return Builder.CreateMul(lhs, rhs);
                } else if (type->isFloatingTy()) {
                    return Builder.CreateFMul(lhs, rhs);
                }
            case ast::BinaryOperator::Div:
            case ast::BinaryOperator::DivAssign:
                if (type->isIntegerTy()) {
                    return llvm::cast<BuiltinType>(type)->isUnsigned()
                        ? Builder.CreateUDiv(lhs, rhs)
                        : Builder.CreateSDiv(lhs, rhs); 
                } else if (type->isFloatingTy()) {
                    return Builder.CreateFDiv(lhs, rhs);
                }
        }
    }

    llvm::Value* emitBinaryOperator(ast::BinaryOperator* expr) {
        llvm::Value* rhs = emitExpr(expr->getRHS());
        if (expr->isAssignOp()) {
            llvm::Value* ptr;
            llvm::Type* ty = CGM.getTypes().convertType(expr->getType());
            
            llvm::Value* result = rhs;
            if (expr->getOpKind() != ast::BinaryOperator::Assign)
                result = emitArithmetic(
                    expr->getOpKind(), expr->getType(), Builder.CreateLoad(ty, ptr), rhs
                );
            
            (void)Builder.CreateStore(result, ptr);
            return result;
        }

        llvm::Value* lhs = emitExpr(expr->getLHS());

        if (expr->isArithmeticOp()) {
            return emitArithmetic(expr->getOpKind(), expr->getType(), lhs, rhs);
        } else if (expr->isComparisonOp()) {
            return emitComparison(expr->getOpKind(), expr->getType(), lhs, rhs);
        }
    }

    llvm::Value* emitUnaryOperator(ast::UnaryOperator* expr) {
        if (expr->isDerefOp()) {
            LValue ptr = emitLValue(expr->getSubExpr());
            return Builder.CreateLoad(ptr.getLLVMType(), ptr.getAddr());
        } else if (expr->isAdressOp())
            return emitLValue(expr->getSubExpr()).getAddr();

        llvm::Type* ty = CGM.getTypes().convertType(expr->getType());
        if (expr->isAssignOp()) {
            llvm::Value* ptr = emitExpr(expr->getSubExpr());

            ast::BinaryOperator::OpKind op;
            llvm::Constant* amnt = llvm::ConstantInt::get(ty, 1);
            if (expr->getOpKind() == ast::UnaryOperator::AddOne) {
                op = ast::BinaryOperator::AddAssign;
            } else if (expr->getOpKind() == ast::UnaryOperator::SubOne) {
                op = ast::BinaryOperator::SubAssign;
            }

            llvm::Value* result = emitArithmetic(
                op, expr->getType(), Builder.CreateLoad(ty, ptr), amnt
            );

            (void)Builder.CreateStore(result, ptr);
            return result;
        } else if (expr->isNegateOp()) {
            llvm::Value* result = emitExpr(expr->getSubExpr());
            if (!expr->getSubExpr()->getType()->isBooleanTy()) 
                result = Builder.CreateICmpEQ(
                    result, llvm::ConstantInt::get(result->getType(), 0)
                );

            return Builder.CreateNot(result);
        } else if (expr->isArithmeticOp()) {
            if (expr->getOpKind() == ast::UnaryOperator::Plus) {
                return emitExpr(expr->getSubExpr());
            } else if (expr->getOpKind() == ast::UnaryOperator::Minus) {
                llvm::Value* result = emitExpr(expr->getSubExpr());
                if (expr->getType()->isIntegerTy() || expr->getType()->isPointerTy()) {
                    return Builder.CreateNeg(result);
                } else if (expr->getType()->isFloatingTy()) {
                    return Builder.CreateFNeg(result);
                }
            }
        }
    }

    llvm::Value* emitCall(ast::CallExpr* expr) {
        llvm::Function* fn = CGM.getFunction(expr->getCalleeDecl());

        llvm::SmallVector<llvm::Value*> args(expr->getCalleeDecl()->getAmntParams());
        for (ast::Expr* e : expr->arguments())
            args.push_back(emitExpr(e));

        return Builder.CreateCall(fn, args);
    }

    void emitReturnStmt(ast::ReturnStmt* stmt) {
        if (stmt->isVoid()) {
            (void)Builder.CreateRetVoid();
            return;
        }

        (void)Builder.CreateRet(emitExpr(stmt->getValue()));
    }

    void emitIfStmt(ast::IfStmt* stmt) {
        llvm::LLVMContext& ctx = CGM.getLLVMContext();

        llvm::BasicBlock* merge = llvm::BasicBlock::Create(ctx, "if.end", Fn);

        while (true) {
            llvm::Value* condition = emitExpr(stmt->getCondition());
            
            llvm::BasicBlock* body = llvm::BasicBlock::Create(ctx, "if.body", Fn);
            llvm::BasicBlock* next = stmt->hasElse() 
                ? llvm::BasicBlock::Create(ctx, "if.else")
                : merge;
            
            (void)Builder.CreateCondBr(condition, body, next);

            Builder.SetInsertPoint(body);
            emitStmt(stmt->getBody());

            if (!Builder.GetInsertBlock()->getTerminator())
                (void)Builder.CreateBr(merge);

            (void)Fn->insert(Fn->end(), next);
            Builder.SetInsertPoint(next);
            if (!stmt->hasElse())
                break;

            stmt = llvm::dyn_cast<ast::IfStmt>(stmt->getElse());
            if (stmt == nullptr) { // Means it is an "else" instead of "elif"
                emitStmt(stmt->getBody());
                if (!Builder.GetInsertBlock()->getTerminator())
                    (void)Builder.CreateBr(merge);

                (void)Fn->insert(Fn->end(), merge);
                Builder.SetInsertPoint(merge);
                break;
            }
        }
    }

    void emitWhileStmt(ast::WhileStmt* stmt) {
        llvm::Value* condition = emitExpr(stmt->getCondition());

        llvm::LLVMContext& ctx = CGM.getLLVMContext();
        llvm::BasicBlock* body = llvm::BasicBlock::Create(ctx, "", Fn);
        llvm::BasicBlock* merge = llvm::BasicBlock::Create(ctx);

        (void)Builder.CreateCondBr(condition, body, merge);

        Builder.SetInsertPoint(body);
        emitStmt(stmt->getBody());

        if (!Builder.GetInsertBlock()->getTerminator())
            (void)Builder.CreateBr(merge);

        (void)Fn->insert(Fn->end(), merge);
        Builder.SetInsertPoint(merge);
    }
};

} // codegen
} // c
