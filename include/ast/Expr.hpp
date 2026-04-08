#pragma once

#include <cassert>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Casting.h"

#include "include/AST/Decl.hpp"
#include "include/AST/Stmt.hpp"
#include "include/AST/Type.hpp"
#include "include/IO/SrcSpan.hpp"

namespace c {
namespace ast {

class Expr : public Stmt {
private:
    const Type* Ty;
protected:
    Expr(Kind SK, Type* T, SrcSpan span)
        : Stmt(SK, span), Ty(T) {}
public:
    static bool classof(const Stmt* s) {
        return s->getKind() <= firstExpr && 
               s->getKind() <= lastExpr;
    }

    const Type* getType() const {
        return Ty;
    }

    void setType(const Type* T) {
        Ty = T;
    }

    bool isTypeDependant() const {
        // FIXME:
    }

    bool isLValue() const {
        if (getKind() == UnaryOperatorKind)
            return llvm::cast<UnaryOperator>(this)->isLValue();
        if (getKind() == DeclRefExprKind)
            return llvm::cast<VarDeclRef>(this)->isLValue();
        return getKind() == AccessExprKind ||
               getKind() == SliceExprKind ||
               getKind() == BinaryOperatorKind;
    }

    bool isRValue() const {
        return !isLValue();
    }

    bool isLiteralExpr() const {
        return getKind() == ArrayLiteralKind ||
               getKind() == StringLiteralKind ||
               getKind() == CharLiteralKind ||
               getKind() == IntegerLiteralKind ||
               getKind() == FloatingLiteralKind ||
               getKind() == BooleanLiteralKind;
    }
};

class QualExpr : public Expr {
private:
    Expr* Base;
protected:
    QualExpr(Kind SK, Type* T, SrcSpan span, Expr* base)
        : Expr(SK, T, span), Base(base) {}
public:
    static bool classof(const Stmt* s) {
        return s->getKind() <= firstQualExpr && 
               s->getKind() <= lastQualExpr;
    }

    Expr* getBase() {
        return Base;
    }

    const Expr* getBase() const {
        return Base;
    }
};

class AccessExpr : public QualExpr {
public:
    static constexpr Kind ClassKind = AccessExprKind;
private:
    llvm::StringRef Name;
    FieldDecl* Field;
public:
    AccessExpr(SrcSpan span, Type* T, Expr* base, llvm::StringRef name)
        : QualExpr(ClassKind, T, span, base), Name(name) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }

    llvm::StringRef getFieldName() const {
        return Name;
    }
    
    FieldDecl* getFieldDecl() {
        return Field;
    }

    void setFieldDecl(FieldDecl* decl) {
        Field = decl;
    }
};

class SliceExpr : public QualExpr {
public:
    static constexpr Kind ClassKind = SliceExprKind;
private:
    Expr* Start;

    // TODO: Should eventually support slicing
    // size_t End;

    // TODO: Consider supporting python-like slicing for 
    // matrixes, like for example: array[start:end, start:end]
public:
    SliceExpr(SrcSpan span, Type* T, Expr* base, Expr* start)
        : QualExpr(ClassKind, T, span, base), Start(start) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }
    
    Expr* getStart() const {
        return Start;
    }
};

class BinaryOperator : public Expr {
public:
    static constexpr Kind ClassKind = BinaryOperatorKind;

    enum OpKind : uint8_t {
        Add,
        Sub,
        Mul,
        Div,

        Assign,
        AddAssign,
        SubAssign,
        MulAssign,
        DivAssign,

        Equal,
        NotEqual,
        MoreThan,
        LessThan,
        MTEqual,
        LTEqual,

        And,
        Or
    };
private:
    OpKind Op;
    Expr* LHS;
    Expr* RHS;
public:
    BinaryOperator(SrcSpan span, Type* T, OpKind op, Expr* lhs, Expr* rhs)
        : Expr(ClassKind, T, span), Op(op), LHS(lhs), RHS(rhs) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }

    OpKind getOpKind() const {
        return Op;
    }

    Expr* getLHS() {
        return LHS;
    }

    const Expr* getLHS() const {
        return LHS;
    }

    Expr* getRHS() {
        return RHS;
    }

    const Expr* getRHS() const {
        return RHS;
    }

    bool isLogicalOp() const {
        return Op == And ||
               Op == Or;
    }

    bool isAssignOp() const {
        return Op == Assign || 
               Op == AddAssign || 
               Op == SubAssign || 
               Op == MulAssign || 
               Op == DivAssign;
    }

    bool isArithmeticOp() const {
        return Op == Add ||
               Op == Sub ||
               Op == Mul ||
               Op == Div;
    }

    bool isComparisonOp() const {
        return Op == Equal ||
               Op == NotEqual ||
               Op == LessThan ||
               Op == MoreThan ||
               Op == LTEqual ||
               Op == MTEqual;
    }
};

class UnaryOperator : public Expr {
public:
    static constexpr Kind ClassKind = UnaryOperatorKind;

    enum OpKind {
        Deref,      // Pointer dereference. Syntax: *
        AdressOf,   // Get pointer to. Syntax: &
        AddOne,     // "++" syntax, alias for "x += 1"
        SubOne,     // "--" syntax, aluas for "x -= 1"
        Plus,       // Positive number literal or cast. Ex: +10, +getNumber()
        Minus,      // Negative number literal or cast. Ex: -10, -getNumber()
        Not         // Negate operator; turns a true value false and vice versa. Syntax: !
    };
private:
    OpKind Op;
    Expr* Sub;
public:
    UnaryOperator(SrcSpan span, Type* T, OpKind op, Expr* sub)
        : Expr(ClassKind, T, span), Op(op), Sub(sub) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }

    OpKind getOpKind() const {
        return Op;
    }

    Expr* getSubExpr() {
        return Sub;
    }

    bool isLValue() const {
        return isDerefOp();
    }

    bool isRValue() const {
        return !isLValue();
    }

    const Expr* getSubExpr() const {
        return Sub;
    }

    bool isIncrementOp() const {
        return Op == AddOne;
    }

    bool isDecrementOp() const {
        return Op == SubOne;
    }

    bool isAssignOp() const {
        return isIncrementOp() || isDecrementOp();
    }

    bool isArithmeticOp() const {
        return Op == Plus ||
               Op == Minus ||
               Op == Not;
    }

    bool isNegateOp() const {
        return Op == Not;
    }

    bool isDerefOp() const {
        return Op == Deref;
    }

    bool isAdressOp() const {
        return Op == AdressOf;
    }

    bool isPointerOp() const {
        return isDerefOp() || isAdressOp(); 
    }
};

class CallExpr : public Expr {
public:
    static constexpr Kind ClassKind = CallExprKind;
private:
    FunctionDecl* DC;
    llvm::StringRef Callee;
    llvm::SmallVector<Expr*> Arguments;
public:
    CallExpr(SrcSpan span, llvm::StringRef callee)
        : Expr(ClassKind, nullptr, span), Callee(callee) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }

    llvm::StringRef getCallee() const {
        return Callee;
    }

    FunctionDecl* getCalleeDecl() {
        return DC;
    }

    const FunctionDecl* getCalleeDecl() const {
        return DC;
    }

    void setCalleeDecl(FunctionDecl* decl) {
        assert(!DC && "This CallExpr already has a callee declaration defined");
        DC = decl;
    }

    void setArguments(llvm::SmallVector<Expr*>&& args) {
        Arguments = args;
    }

    size_t getAmntArgs() const {
        return Arguments.size();
    }

    Expr* getArg(size_t idx) {
        return Arguments[idx];
    }

    const Expr* getArg(size_t idx) const {
        return Arguments[idx];
    }

    llvm::ArrayRef<Expr*> arguments() const {
        return Arguments;
    }
    
    llvm::MutableArrayRef<Expr*> arguments() {
        return llvm::MutableArrayRef<Expr*>(Arguments.begin(), Arguments.end());
    }
};

// TODO: Eventually should support expressions like namespace::attribute
// TODO: DeclRefExpr usually supports nested expressions like namespace::attribute.
// This includes functions, variables and enum constants.
// class DeclRefExpr : public Expr {
// private:
//     llvm::StringRef Name;
// public:
//     DeclRefExpr(SrcSpan span, Type* T, llvm::StringRef name)
//         : Expr(DeclRefExprKind, T, span), Name(name) {}
    
//     llvm::StringRef getName() const {
//         return Name;
//     }

//     Decl* getDecl() {

//     }
// };

// NOTE: Temporary class until DeclRefExpr is implemented
class VarDeclRef : public Expr {
public:
    static constexpr Kind ClassKind = DeclRefExprKind;
private:
    llvm::StringRef Name;
    VarDecl* DC;
public:
    VarDeclRef(SrcSpan span, llvm::StringRef name)
        : Expr(ClassKind, nullptr, span), Name(name) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }

    llvm::StringRef getName() const {
        return Name;
    }

    void setDecl(VarDecl* decl) {
        assert(DC == nullptr && "This VarDeclRef has already been resolved.");
        setType(decl->getType());
        DC = decl;
    }

    VarDecl* getDecl() {
        return DC;
    }

    const VarDecl* getDecl() const {
        return DC;
    }

    bool isLValue() const {
        return DC->isMutable();
    }

    bool isRValue() const {
        return !isLValue();
    }
};

class ArrayLiteral : public Expr {
public:
    static constexpr Kind ClassKind = ArrayLiteralKind;
private:
    llvm::SmallVector<Expr*> Items;
public: 
    ArrayLiteral(SrcSpan span, Type* T, llvm::ArrayRef<Expr*> items)
        : Expr(ClassKind, T, span), Items(items) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }

    size_t getAmntItems() const {
        return Items.size();
    }

    Expr* getItem(size_t idx) {
        return Items[idx];
    }

    const Expr* getInit(size_t idx) const {
        return Items[idx];
    }

    llvm::ArrayRef<Expr*> itmes() const {
        return Items;
    }

    llvm::MutableArrayRef<Expr*> items() {
        return llvm::MutableArrayRef<Expr*>(Items.begin(), Items.end());
    }
};

class IntegerLiteral : public Expr {
public:
    static constexpr Kind ClassKind = IntegerLiteralKind;
private:
    llvm::APInt Value;
public:
    IntegerLiteral(SrcSpan span, llvm::APInt value)
        : Expr(ClassKind, nullptr, span), Value(value) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }
    
    size_t getBitWidth() {
        return Value.getBitWidth();
    }

    llvm::APInt getValue() const {
        return Value;
    }
};

class FloatingLiteral : public Expr {
public:
    static constexpr Kind ClassKind = FloatingLiteralKind;
private:
    llvm::APFloat Value;
public:
    FloatingLiteral(SrcSpan span, llvm::APFloat value)
        : Expr(ClassKind, nullptr, span), Value(value) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }

    llvm::APFloat getValue() const {
        return Value;
    }

    const llvm::fltSemantics& getSemantics() const {
        return getValue().getSemantics();
    }
};

class BooleanLiteral : public Expr {
public:
    static constexpr Kind ClassKind = BooleanLiteralKind;
private:
    bool Value;
public:
    BooleanLiteral(SrcSpan span, Type* T, bool value)
        : Expr(ClassKind, T, span), Value(value) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }

    bool getValue() const {
        return Value;
    }
};

} // namespace ast
} // namespace c
