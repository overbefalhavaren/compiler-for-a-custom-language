#pragma once

#include "llvm/ADT/StringMap.h"

#include "stmt.hpp"

namespace c {
namespace ast {

class Expr : public Stmt {
protected:
    Expr(StmtClass cls, SrcSpan span) 
        : Stmt(cls, std::move(span)) {}
public:
    bool isLValue() {

    }
};

class BaseTrait {
private:
    std::unique_ptr<Expr> Base;
public:
    BaseTrait(std::unique_ptr<Expr>&& base)
        : Base(std::move(base)) {
        assert(Base && "'base' can't be nullptr");
    }

    inline const std::unique_ptr<Expr>& getBase() const {
        return Base;
    }

    inline std::unique_ptr<Expr>& getBase() {
        return Base;
    }
};

class QualNameExpr : public Expr, public NameTrait, public BaseTrait{
public:
    static constexpr StmtClass ClassID = StmtClass::SC_QualNameExpr;

    QualNameExpr(SrcSpan span, llvm::StringRef name, std::unique_ptr<Expr>&& base)
        : Expr(ClassID, span), NameTrait(name), BaseTrait(std::move(base)) {}
};

class AccessExpr : public Expr, public NameTrait, public BaseTrait{
public:
    static constexpr StmtClass ClassID = StmtClass::SC_AccessExpr;

    AccessExpr(SrcSpan span, llvm::StringRef name, std::unique_ptr<Expr>&& base)
        : Expr(ClassID, span), NameTrait(name), BaseTrait(std::move(base)) {}
};

class CastExpr : public Expr {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_CastExpr;
private:
    std::unique_ptr<Expr> Value;
    std::unique_ptr<TypeExpr> Ty;
public:
    CastExpr(SrcSpan span, std::unique_ptr<TypeExpr>&& type, std::unique_ptr<Expr>&& value)
        : Expr(ClassID, span), Ty(std::move(type)), Value(std::move(value)) {}
    
    inline bool isVoidCast() const {
        return Ty != nullptr;
    }

    inline const std::unique_ptr<Expr>& getValue() const {
        return Value;
    }

    inline std::unique_ptr<Expr>& getValue() {
        return Value;
    }

    inline const std::unique_ptr<TypeExpr>& getType() const {
        return getType();
    }

    inline std::unique_ptr<TypeExpr>& getType() {
        assert(!isVoidCast() && "Can't get the type for a void cast.");
        return Ty;
    }
};

class MoveExpr : public Expr {
    static constexpr StmtClass ClassID = StmtClass::SC_MoveExpr;
private:
    std::unique_ptr<Expr> Value;
public:
    MoveExpr(SrcSpan span, std::unique_ptr<Expr>&& value)
        : Expr(ClassID, span), Value(std::move(value)) {
        assert(Value && "'value' can't be nullptr.");
    }

    inline const Expr& getValue() const {
        return *Value;
    }

    inline Expr& getValue() {
        return *Value;
    }
};

class CallExpr : public Expr {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_CallExpr;
private:
    llvm::StringRef Callee;
    llvm::SmallVector<std::unique_ptr<Expr>> Args;
public:
    CallExpr(SrcSpan span, llvm::StringRef callee)
        : Expr(ClassID, span), Callee(callee) {}
    CallExpr(SrcSpan span, llvm::StringRef callee, llvm::SmallVector<std::unique_ptr<Expr>>&& args)
        : Expr(ClassID, span), Callee(callee), Args(std::move(args)) {}
    
    inline llvm::StringRef getCallee() const {
        return Callee;
    }

    inline bool hasParams() const {
        return Args.size() != 0;
    }

    inline const llvm::SmallVector<std::unique_ptr<Expr>>& getArgs() const {
        return Args; 
    }

    inline llvm::SmallVector<std::unique_ptr<Expr>>& getArgs() {
        return Args;
    }
};

class NameExpr : public Expr, public NameTrait {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_NameExpr;

    NameExpr(SrcSpan span, llvm::StringRef name) 
        : Expr(ClassID, span), NameTrait(name) {}
};

class BinaryOperator : public Expr {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_BinaryOperator;
private:
    TokenType Op;
    std::unique_ptr<Expr> LHS;
    std::unique_ptr<Expr> RHS;
public:
    BinaryOperator(SrcSpan span, TokenType op, std::unique_ptr<Expr>&& lhs, std::unique_ptr<Expr>&& rhs)
        : Expr(ClassID, span), Op(op), LHS(std::move(lhs)), RHS(std::move(rhs)) {
        assert(LHS && "'lhs' can't be nullptr.");
        assert(RHS && "'rhs' can't be nullptr.");
    }

    inline TokenType getOperator() const { 
        return Op; 
    }

    inline const std::unique_ptr<Expr>& getLHS() const { 
        return LHS; 
    }

    inline std::unique_ptr<Expr>& getLHS() {
        return LHS;
    }

    inline const std::unique_ptr<Expr>& getRHS() const { 
        return RHS; 
    }

    inline std::unique_ptr<Expr>& getRHS() {
        return RHS;
    }
};

class UnaryOperator : public Expr {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_UnaryOperator;
private:
    TokenType Op;
    std::unique_ptr<Expr> LHS;
public:
    UnaryOperator(SrcSpan span, TokenType op, std::unique_ptr<Expr>&& lhs)
        : Expr(ClassID, span), Op(op), LHS(std::move(lhs)) {
        assert(LHS && "'lhs' can't be nullptr.");
    }

    inline TokenType getOperator() const { 
        return Op; 
    }

    inline const std::unique_ptr<Expr>& getLHS() const { 
        return LHS; 
    }

    inline std::unique_ptr<Expr>& getLHS() { 
        return LHS;
    }
};

template <typename T, Stmt::StmtClass ClassT>
class Literal : public Expr {
public:
    static constexpr StmtClass ClassID = ClassT;
private:
    T Value;
public:
    Literal(SrcSpan span, T value)
        : Expr(ClassID, span), Value(std::move(value)) {}

    inline T getValue() const {
        return Value;
    }
};

using StringLiteral = Literal<llvm::StringRef, Stmt::SC_StringLiteral>;
using NumberLiteral = Literal<size_t, Stmt::SC_NumberLiteral>;
using FloatLiteral = Literal<double, Stmt::SC_FloatLiteral>;
using CharLiteral = Literal<char, Stmt::SC_CharLiteral>;
using BoolLiteral = Literal<bool, Stmt::SC_BoolLiteral>;

} // namespace ast
} // namespace c
