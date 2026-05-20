#include "include/CodeGen/CodeGenFunction.hpp"

#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/ErrorHandling.h"

#include "include/Frontend/ASTVisitor.hpp"

#include "src/Debug.hpp"

using namespace c;
using namespace c::ast;

namespace c {
namespace codegen {

std::optional<Place> CodeGenFunction::lookup(const InitDecl* DC) {
    auto result = Locals.find(DC);
    if (result == Locals.end())
        return std::nullopt;
    return result->second;
}

void CodeGenFunction::emit(const FunctionDecl& DC) {
    DEBUG("Called: emit");
    auto build_block = llvm::BasicBlock::Create(CGM.getContext(), "entry", Fn);
    Builder.SetInsertPoint(build_block);

    for (size_t i = 0; i < DC.getAmntParams(); i++)
        emitParamDecl(*DC.getParamDecl(i), Fn->getArg(i));
    
    emitBlockStmt(*DC.getBody());

    if (!Builder.GetInsertBlock()->getTerminator())
        (void)Builder.CreateRetVoid();
}

void CodeGenFunction::emitVarDecl(const VarDecl& DC) {
    DEBUG("Called: emitVarDecl");
    assert(DC.hasInit());
    if (DC.getInit()->isPlace()) {
        (void)Locals.insert({&DC, emitPlace(DC.getInit())});
        return;
    }

    llvm::Type* ty = CGM.getTypes().convertType(DC.getType());
    auto align = CGM.getModule().getDataLayout().getABITypeAlign(ty);

    llvm::AllocaInst* alloca = Builder.CreateAlloca(ty, nullptr, DC.getName());
    alloca->setAlignment(align);

    Value value = emitExpr(DC.getInit());
    Place ptr = Place(alloca, DC.getType());
    emitStore(ptr, value);

    (void)Locals.insert({&DC, ptr});
}

void CodeGenFunction::emitParamDecl(const ParamDecl& DC, llvm::Argument* Arg) {
    DEBUG("Called: emitParamDecl");
    llvm::IRBuilder<> fn_start(&Fn->getEntryBlock(), Fn->getEntryBlock().begin());

    llvm::Type* ty = CGM.getTypes().convertType(DC.getType());
    llvm::Value* alloca = fn_start.CreateAlloca(ty, nullptr, DC.getName());
    (void)fn_start.CreateStore(Arg, alloca);

    (void)Locals.insert({&DC, Place(alloca, DC.getType())});
}

void CodeGenFunction::emitStmt(const Stmt* ST) {
    DEBUG("Called: emitStmt");
    if (auto EX = llvm::dyn_cast<Expr>(ST)) {
        (void)emitExpr(EX);
        return;
    }

    switch (ST->getKind()) {
        default:
            llvm_unreachable("Stmt subclass missing a case");
        case Stmt::BlockStmtKind:
            emitBlockStmt(*llvm::cast<BlockStmt>(ST));
            break;
        case Stmt::IfStmtKind:
            emitIfStmt(*llvm::cast<IfStmt>(ST));
            break;
        case Stmt::WhileStmtKind:
            emitWhileStmt(*llvm::cast<WhileStmt>(ST));
            break;
        case Stmt::ReturnStmtKind:
            emitReturnStmt(*llvm::cast<ReturnStmt>(ST));
            break;
        case Stmt::DeclStmtKind: {
            const Decl* DC = llvm::cast<DeclStmt>(ST)->getDecl();
            
            assert(llvm::isa<VarDecl>(DC));
            emitVarDecl(*llvm::cast<VarDecl>(DC));
            break;
        }
    }
}

void CodeGenFunction::emitBlockStmt(const BlockStmt& ST) {
    DEBUG("Called: emitBlockStmt");
    for (const Stmt* s : ST.stmts())
        if (auto EX = llvm::dyn_cast<Expr>(s)) {
            if (EX->isPlace()) {
                (void)emitPlace(EX);
            } else
                (void)emitExpr(EX);
        } else
            emitStmt(s);
}

void CodeGenFunction::emitIfStmt(const IfStmt& ST) {
    DEBUG("Called: emitIfStmt");
    auto end_block = llvm::BasicBlock::Create(CGM.getContext(), "if.end", Fn);

    const IfStmt* cur = &ST;
    while (true) {
        llvm::Value* condition = emitCondition(cur->getCondition());
        auto body_block = llvm::BasicBlock::Create(CGM.getContext(), "if.then", Fn);
        auto next_block = cur->hasElse()
            ? llvm::BasicBlock::Create(CGM.getContext(), "if.else", Fn)
            : end_block;

        (void)Builder.CreateCondBr(condition, body_block, next_block);
        
        Builder.SetInsertPoint(body_block);
        emitStmt(cur->getBody());
        if (!Builder.GetInsertBlock()->getTerminator())
            (void)Builder.CreateBr(end_block);

        Builder.SetInsertPoint(next_block);
        if (!cur->hasElse()) {
            break;
        } else if (auto elif = llvm::dyn_cast<IfStmt>(cur->getElse())) {
            cur = elif;
            continue;
        }

        emitStmt(cur->getElse());
        if (!Builder.GetInsertBlock()->getTerminator())
            (void)Builder.CreateBr(end_block);
        break;
    }
}

void CodeGenFunction::emitWhileStmt(const WhileStmt& ST) {
    DEBUG("Called: emitWhileStmt");
    auto cond_block = llvm::BasicBlock::Create(CGM.getContext(), "while.cond", Fn);
    auto body_block = llvm::BasicBlock::Create(CGM.getContext(), "while.body");
    auto end_block = llvm::BasicBlock::Create(CGM.getContext(), "while.end", Fn);

    // Condition
    (void)Builder.CreateBr(cond_block);
    (void)Builder.SetInsertPoint(cond_block);

    llvm::Value* cond = emitCondition(ST.getCondition());
    (void)Builder.CreateCondBr(cond, body_block, end_block);

    // Body
    (void)Fn->insert(Fn->end(), body_block);
    (void)Builder.SetInsertPoint(body_block);
    
    emitStmt(ST.getBody());
    if (!Builder.GetInsertBlock()->getTerminator())
        (void)Builder.CreateBr(cond_block);
    
    // End
    (void)Builder.SetInsertPoint(end_block);
}

void CodeGenFunction::emitReturnStmt(const ReturnStmt& ST) {
    DEBUG("Called: emitReturnStmt");
    if (ST.isVoid()) {
        (void)Builder.CreateRetVoid();
    } else
        (void)Builder.CreateRet(emitExpr(ST.getValue()).getValue());
}

void CodeGenFunction::emitStore(Place ptr, Value val) {
    DEBUG("Called: emitStore");
    llvm::Type* ty = CGM.getTypes().convertType(ptr.getType());
    llvm::Align align = CGM.getModule().getDataLayout().getABITypeAlign(ty);

    llvm::StoreInst* store = Builder.CreateStore(val.getValue(), ptr.getPointer());
    store->setAlignment(align);
}

Value CodeGenFunction::emitLoad(Place ptr) {
    DEBUG("Called: emitLoad");
    llvm::Type* ty = CGM.getTypes().convertType(ptr.getType());
    return Builder.CreateLoad(ty, ptr.getPointer());
}

Value CodeGenFunction::emitExpr(const Expr* EX) {
    DEBUG("Called: emitExpr");
    if (EX->isPlace())
        return emitLoad(emitPlace(EX));

    switch (EX->getKind()) {
        default:
            llvm_unreachable("Expr subclass is missing a case.");

        case Expr::CallExprKind:
            return emitCallExpr(*llvm::cast<CallExpr>(EX));
        case Expr::BinaryOperatorKind: {
            auto& op = *llvm::cast<BinaryOperator>(EX);
            if (op.isAssignOp())
                return emitBinaryAssign(op);
            return emitBinaryOperator(op);
        }
        case Expr::UnaryOperatorKind: {
            auto& op = *llvm::cast<UnaryOperator>(EX);
            if (op.isAssignOp())
                return emitUnaryAssign(op);
            return emitUnaryOperator(op);
        }
        case Expr::ArrayLiteralKind: {
            auto& lit = *llvm::cast<ArrayLiteral>(EX);
            if (lit.isConstant())
                return emitConstantArray(lit);
            return emitArrayLiteral(lit);
        }
        case Expr::IntegerLiteralKind: {
            // FIXME: FIXME: FIXME: FIXME: FIXME:
            auto ty = CGM.getTypes().convertType(EX->getType());

            llvm::APInt val(
                ty->getIntegerBitWidth(),
                *llvm::cast<IntegerLiteral>(EX)->getValue().getRawData()
            );


            auto ret = llvm::ConstantInt::get(
                ty,
                val
            );

            return ret;
        }

            // return llvm::ConstantInt::get(
            //     CGM.getTypes().convertType(EX->getType()),
            //     llvm::cast<IntegerLiteral>(EX)->getValue()
            // );
        case Expr::FloatingLiteralKind:
            return llvm::ConstantFP::get(
                CGM.getTypes().convertType(EX->getType()),
                llvm::cast<FloatingLiteral>(EX)->getValue()
            );
        case Expr::BooleanLiteralKind:
            return llvm::ConstantInt::getBool(
                Builder.getInt1Ty(),
                llvm::cast<BooleanLiteral>(EX)->getValue()
            );
    }
}

Place CodeGenFunction::emitPlace(const Expr* EX) {
    DEBUG("Called: emitPlace");
    assert(EX->isPlace());
    switch (EX->getKind()) {
        default:
            llvm_unreachable("Place Expr missing case.");
        case Expr::AccessExprKind:
            return emitAccessExpr(*llvm::cast<AccessExpr>(EX));
        case Expr::SliceExprKind:
            return emitSliceExpr(*llvm::cast<SliceExpr>(EX), true);
        case Expr::DeclRefExprKind:
            return emitDeclRef(*llvm::cast<DeclRefExpr>(EX));
        case Expr::UnaryOperatorKind:
            return emitDereference(*llvm::cast<UnaryOperator>(EX));
        case Expr::CallExprKind:
            assert(llvm::isa<StructDecl>(llvm::cast<CallExpr>(EX)->getCalleeDecl()));
            return emitConstructCall(*llvm::cast<CallExpr>(EX));
    }
}

llvm::Value* CodeGenFunction::emitCondition(const Expr* EX) {
    DEBUG("Called: emitCondition");
    Value result = emitExpr(EX);
    auto ty = CGM.getTypes().convertType(EX->getType());
    if (EX->getType()->isBooleanTy()) {
        return result.getValue();
    } else if (ty->isIntegerTy()) {
        return Builder.CreateICmpNE(
            result.getValue(),
            llvm::ConstantInt::get(ty, 0),
            "Integer.to_bool"
        );
    } else if (ty->isFloatingPointTy()) {
        return Builder.CreateFCmpONE(
            result.getValue(),
            llvm::ConstantFP::get(ty, 0.0),
            "FloatingPoint.to_bool"
        );
    }
    llvm_unreachable("Type not supported for conversion to bool.");
}

llvm::Value* EmitEqualComparison(CodeGenFunction& CGF, Value& lhs, Value& rhs) {
    DEBUG("Called: EmitEqualComparison");

    auto Ty = rhs.getValue()->getType();
    if (Ty->isIntegerTy() || Ty->isPointerTy()) {
        return CGF.getBuilder().CreateICmpEQ(lhs.getValue(), rhs.getValue());
    } else if (Ty->isFloatingPointTy()) {
        return CGF.getBuilder().CreateFCmpOEQ(lhs.getValue(), rhs.getValue());
    } else if (Ty->isStructTy()) {
        return CGF.emitStructComparison(lhs, rhs);
    }
    llvm_unreachable("Invalid Type for Equal comparison.");
}

Value CodeGenFunction::emitBinaryOperator(const BinaryOperator& EX) {
    DEBUG("Called: emitBinaryOperator");
    assert(EX.getLHS()->getType() == EX.getRHS()->getType());
    Value lhs = emitExpr(EX.getLHS());
    Value rhs = emitExpr(EX.getRHS());

    const Type* Ty = EX.getType();
    switch (EX.getOpKind()) {
        default:
            llvm_unreachable("BinaryOperator::OpKind missing condition.");

        case BinaryOperator::Add:
        case BinaryOperator::AddAssign:
            if (Ty->isIntegerTy()) {
                return Builder.CreateAdd(lhs.getValue(), rhs.getValue());
            } else if (Ty->isFloatingTy()) 
                return Builder.CreateFAdd(lhs.getValue(), rhs.getValue());
            llvm_unreachable("Invalid Type for Add.");
        case BinaryOperator::Sub:
        case BinaryOperator::SubAssign:
            if (Ty->isIntegerTy()) {
                return Builder.CreateSub(lhs.getValue(), rhs.getValue());
            } else if (Ty->isFloatingTy()) 
                return Builder.CreateFSub(lhs.getValue(), rhs.getValue());
            llvm_unreachable("Invalid Type for Sub.");
        case BinaryOperator::Mul:
        case BinaryOperator::MulAssign:
            if (Ty->isIntegerTy()) {
                return Builder.CreateMul(lhs.getValue(), rhs.getValue());
            } else if (Ty->isFloatingTy()) 
                return Builder.CreateFMul(lhs.getValue(), rhs.getValue());
            llvm_unreachable("Invalid Type for Mul.");
        case BinaryOperator::Div:
        case BinaryOperator::DivAssign:
            if (Ty->isIntegerTy()) {
                return llvm::cast<BuiltinType>(Ty)->isUnsigned()
                    ? Builder.CreateUDiv(lhs.getValue(), rhs.getValue())
                    : Builder.CreateSDiv(lhs.getValue(), rhs.getValue()); 
            } else if (Ty->isFloatingTy())
                return Builder.CreateFDiv(lhs.getValue(), rhs.getValue());
            llvm_unreachable("Invalid Type for Div.");
        case BinaryOperator::Equal:
            return EmitEqualComparison(*this, lhs, rhs);
        case BinaryOperator::NotEqual:
            return Builder.CreateNot(EmitEqualComparison(*this, lhs, rhs));
        case BinaryOperator::MoreThan:
            if (Ty->isIntegerTy()) {
                return llvm::cast<BuiltinType>(Ty)->isUnsigned()
                    ? Builder.CreateICmpUGT(lhs.getValue(), rhs.getValue())
                    : Builder.CreateICmpSGT(lhs.getValue(), rhs.getValue());
            } else if (Ty->isFloatingTy())
                return Builder.CreateFCmpOGT(lhs.getValue(), rhs.getValue());
            llvm_unreachable("Invalid Type for MoreThan.");
        case BinaryOperator::MTEqual:
            if (Ty->isIntegerTy()) {
                return llvm::cast<BuiltinType>(Ty)->isUnsigned()
                    ? Builder.CreateICmpUGE(lhs.getValue(), rhs.getValue())
                    : Builder.CreateICmpSGE(lhs.getValue(), rhs.getValue());
            } else if (Ty->isFloatingTy())
                return Builder.CreateFCmpOGE(lhs.getValue(), rhs.getValue());
            llvm_unreachable("Invalid Type for MTEqual.");
        case BinaryOperator::LessThan:
            if (Ty->isIntegerTy()) {
                return llvm::cast<BuiltinType>(Ty)->isUnsigned()
                    ? Builder.CreateICmpULT(lhs.getValue(), rhs.getValue())
                    : Builder.CreateICmpSLT(lhs.getValue(), rhs.getValue());
            } else if (Ty->isFloatingTy())
                return Builder.CreateFCmpOLT(lhs.getValue(), rhs.getValue());
            llvm_unreachable("Invalid Type for LessThan.");
        case BinaryOperator::LTEqual:
            if (Ty->isIntegerTy()) {
                return llvm::cast<BuiltinType>(Ty)->isUnsigned()
                    ? Builder.CreateICmpULE(lhs.getValue(), rhs.getValue())
                    : Builder.CreateICmpSLE(lhs.getValue(), rhs.getValue());
            } else if (Ty->isFloatingTy())
                return Builder.CreateFCmpOLE(lhs.getValue(), rhs.getValue());
            llvm_unreachable("Invalid Type for LTEqual.");
    }
}

Value CodeGenFunction::emitBinaryAssign(const BinaryOperator& EX) {
    DEBUG("Called: emitBinaryAssing");
    assert(EX.isAssignOp());
    Place ptr = emitPlace(EX.getLHS());
    Value value = EX.getOpKind() != BinaryOperator::Assign 
        ? emitBinaryOperator(EX)
        : emitExpr(EX.getRHS());
    
    emitStore(ptr, value);
    return emitLoad(ptr);
}

llvm::Value* CodeGenFunction::emitStructComparison(Value& LHS, Value& RHS) {
    DEBUG("Called: emitStructComparison");
    auto Ty = llvm::cast<llvm::StructType>(RHS.getValue()->getType());
    llvm::Value* result = nullptr;
    for (size_t i = 0; i < Ty->getNumElements(); i++) {
        Value left = Builder.CreateExtractValue(LHS.getValue(), i);
        Value right = Builder.CreateExtractValue(RHS.getValue(), i);

        llvm::Value* cmp = EmitEqualComparison(*this, left, right);
        if (result != nullptr) {
            result = Builder.CreateAnd(result, cmp);
        } else 
            result = cmp;
    }

    return result;
}

Value CodeGenFunction::emitUnaryOperator(const UnaryOperator& EX) {
    DEBUG("Called: emitUnaryOperator");
    assert(!EX.isAssignOp());
    assert(!EX.isDerefOp());
    if (EX.isNegateOp()) {
        return Builder.CreateNot(emitCondition(EX.getSubExpr()));
    } else if (EX.isAdressOp()) {
        assert(EX.getSubExpr()->isPlace());
        return emitPlace(EX.getSubExpr()).getPointer();
    } else if (EX.isArithmeticOp()) {
        Value result = emitExpr(EX.getSubExpr());
        if (EX.getOpKind() == UnaryOperator::Minus) {
            if (EX.getType()->isIntegerTy() || EX.getType()->isPointerTy()) {
                return Builder.CreateNeg(result.getValue());
            } else if (EX.getType()->isFloatingTy()) {
                return Builder.CreateFNeg(result.getValue());
            } else 
                llvm_unreachable("Unsupported Type for Plus and Minus unary operators.");
        }

        assert(EX.getOpKind() == UnaryOperator::Plus);
        return result;
    }
    llvm_unreachable("");
}

Value CodeGenFunction::emitUnaryAssign(const UnaryOperator& EX) {
    DEBUG("Called: emitUnaryAssign");
    assert(EX.isAssignOp());
    Place ptr = emitPlace(EX.getSubExpr());
    
    llvm::Value* value;
    llvm::Type* ty = CGM.getTypes().convertType(EX.getType());
    llvm::Constant* amnt = llvm::ConstantInt::get(ty, 1);
    if (EX.isIncrementOp()) {
        if (ty->isIntegerTy()) {
            value = Builder.CreateAdd(emitLoad(ptr).getValue(), amnt);
        } else if (ty->isFloatingPointTy()) {
            value = Builder.CreateFAdd(emitLoad(ptr).getValue(), amnt);
        } else 
            llvm_unreachable("Invalid type for AddOne.");
    } else if (EX.isDecrementOp()) {
        if (ty->isIntegerTy()) {
            value = Builder.CreateSub(emitLoad(ptr).getValue(), amnt);
        } else if (ty->isFloatingPointTy()) {
            value = Builder.CreateFSub(emitLoad(ptr).getValue(), amnt);
        } else 
            llvm_unreachable("Invalid type for SubOne");
    } else
        llvm_unreachable("Unary assign operator missing condition");
    
    emitStore(ptr, Value(value));
    return emitLoad(ptr);
}

Place CodeGenFunction::emitDereference(const UnaryOperator& EX) {
    DEBUG("Called: emitDereference");
    assert(EX.isDerefOp());
    Place value = emitPlace(EX.getSubExpr());
    return Place(
        emitLoad(value).getValue(), 
        EX.getType()
    );
}

Value CodeGenFunction::emitConstantArray(const ArrayLiteral& EX) {
    DEBUG("Called: emitConstantArray");
    assert(EX.isConstant());
    llvm::SmallVector<llvm::Constant*> items;
    items.reserve(EX.getAmntItems());
    for (Expr* it : EX.items()) {
        auto value = emitExpr(it).getValue();
        assert(llvm::isa<llvm::Constant>(value));
        items.push_back(llvm::cast<llvm::Constant>(value));
    }

    llvm::Type* ty = CGM.getTypes().convertType(EX.getType());
    return llvm::ConstantArray::get(
        llvm::cast<llvm::ArrayType>(ty), items
    );
}

Value CodeGenFunction::emitArrayLiteral(const ArrayLiteral& EX) {
    DEBUG("Called: emitArrayLiteral");
    assert(!EX.isConstant());
    llvm::Value* size = Builder.getInt64(EX.getAmntItems());
    llvm::Type* ty = CGM.getTypes().convertType(EX.getType());
    llvm::Value* alloca = Builder.CreateAlloca(ty, size);
    for (size_t i = 0; i < EX.getAmntItems(); i++) {
        llvm::Value* idx = Builder.getInt64(i);
        llvm::Value* item_ptr = Builder.CreateGEP(
            ty, alloca, {Builder.getInt32(0), idx}
        );

        llvm::Value* value = emitExpr(EX.getItem(i)).getValue();
        (void)Builder.CreateStore(value, item_ptr);
    }

    return alloca;
}

Value CodeGenFunction::emitCallExpr(const CallExpr& EX) {
    DEBUG("Called: emitCallExpr");
    // Calls fo function pointers are currently not supported
    assert(llvm::isa<FunctionDecl>(EX.getCalleeDecl()));
    auto DC = llvm::cast<FunctionDecl>(EX.getCalleeDecl());
    llvm::Function* callee = CGM.getFunction(DC);

    llvm::SmallVector<llvm::Value*> arguments;
    arguments.reserve(EX.getAmntArgs());
    for (auto arg : EX.arguments())
        arguments.push_back(emitExpr(arg).getValue());

    auto ty = callee->getFunctionType();
    llvm::CallInst* call = Builder.CreateCall(ty, callee, arguments);
    if (DC->isVoid())
        return Value(nullptr); // FIXME: Might be dangerous
    return Value(call);
}

Place CodeGenFunction::emitConstructCall(const CallExpr& EX) {
    DEBUG("Called: emitConstructCall");
    assert(llvm::isa<StructType>(EX.getType()));
    llvm::Type* ty = CGM.getTypes().convertType(EX.getType());
    llvm::AllocaInst* alloca = Builder.CreateAlloca(ty);
    for (size_t i = 0; i < EX.getAmntArgs(); i++)
        (void)Builder.CreateStore(
            emitExpr(EX.getArg(i)).getValue(),
            Builder.CreateStructGEP(ty, alloca, i)
        );
    return Place(alloca, EX.getType());
}

Place CodeGenFunction::emitDeclRef(const DeclRefExpr& EX) {
    DEBUG("Called: emitDeclRef");
    if (auto DC = llvm::dyn_cast<InitDecl>(EX.getDecl())) {
        auto result = lookup(DC);
        if (result.has_value())
            return result.value();
        
        assert(llvm::isa<VarDecl>(DC));
        auto glob = CGM.getGlobalVariable(llvm::cast<VarDecl>(DC));
        assert(glob != nullptr);
        return Place(glob, DC->getType());
    }
    
    llvm_unreachable("Only VarDecl is supported for DeclRefExpr in CodeGen.");
}

Place CodeGenFunction::emitAccessExpr(const AccessExpr& EX) {
    DEBUG("Called: emitAccessExpr");
    llvm::Value* field = Builder.CreateStructGEP(
        CGM.getTypes().convertType(EX.getBase()->getType()),
        emitPlace(EX.getBase()).getPointer(),
        EX.getFieldDecl()->getFieldIndex()
    );

    return Place(field, EX.getType());
}

Place CodeGenFunction::emitSliceExpr(const SliceExpr& EX, bool doBoundsCheck) {
    DEBUG("Called: emitSliceExpr");
    llvm::Value* index = emitExpr(EX.getStart()).getValue();
    if (doBoundsCheck) {
        auto ok_block = llvm::BasicBlock::Create(CGM.getContext(), "in_bounds", Fn);
        auto trap_block = llvm::BasicBlock::Create(CGM.getContext(), "out_of_bounds", Fn);

        size_t array_size = llvm::cast<ArrayType>(EX.getBase()->getType())->getInitSize();
        llvm::Value* condition = Builder.CreateICmpUGE(index, Builder.getInt64(array_size));
        Builder.CreateCondBr(condition, ok_block, trap_block);

        Builder.SetInsertPoint(trap_block);
        Builder.CreateCall(llvm::Intrinsic::getOrInsertDeclaration(
            &CGM.getModule(), llvm::Intrinsic::trap
        ));
        Builder.CreateUnreachable();

        Builder.SetInsertPoint(ok_block);
    }

    llvm::Value* result = Builder.CreateGEP(
        CGM.getTypes().convertType(EX.getBase()->getType()),
        emitPlace(EX.getBase()).getPointer(),
        {Builder.getInt32(0), index}
    );

    return Place(result, EX.getType());
}

} // namespace codegen
} // namespace c
