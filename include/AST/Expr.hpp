#pragma once

#include <cassert>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include "include/AST/Stmt.hpp"
#include "include/AST/Type.hpp"

namespace c {
namespace ast {

class FieldDecl; // include/AST/Decl.hpp

class Expr : public Stmt {
private:
    const Type* Ty = nullptr;
protected:
    Expr(Kind SK, SrcSpan span)
        : Stmt(SK, span) {}
public:
    static bool classof(const Stmt* s) {
        return s->getKind() >= firstExpr && 
               s->getKind() <= lastExpr;
    }

    const Type* getType() const {
        return Ty;
    }

    void setType(const Type* T) {
        Ty = T;
    }

    bool isResolved() const {
        return Ty != nullptr;
    }

    bool isTypeDependant() const;

    bool isPlace() const;

    bool isModifiableValue() const;

    bool isLiteralExpr() const;
};

class QualExpr : public Expr {
private:
    Expr* Base;
protected:
    QualExpr(Kind SK, SrcSpan span, Expr* base)
        : Expr(SK, span), Base(base) {}
public:
    static bool classof(const Stmt* s) {
        return s->getKind() >= firstQualExpr && 
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
    FieldDecl* Field = nullptr;
public:
    AccessExpr(SrcSpan span, Expr* base, llvm::StringRef name)
        : QualExpr(ClassKind, span, base), Name(name) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }

    llvm::StringRef getFieldName() const {
        return Name;
    }
    
    FieldDecl* getFieldDecl() {
        return Field;
    }

    const FieldDecl* getFieldDecl() const {
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
    SliceExpr(SrcSpan span, Expr* base, Expr* start)
        : QualExpr(ClassKind, span, base), Start(start) {}

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
        MoreThan, // >
        LessThan, // <
        MTEqual,  // >=
        LTEqual,  // <=

        And,
        Or
    };
private:
    OpKind Op;
    Expr* LHS;
    Expr* RHS;
public:
    BinaryOperator(SrcSpan span, OpKind op, Expr* lhs, Expr* rhs)
        : Expr(ClassKind, span), Op(op), LHS(lhs), RHS(rhs) {}

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
        Deref,      // Pointer dereference.
        Adress,     // Get pointer pointer of item.
        AddOne,     // "++" syntax, alias for "x += 1"
        SubOne,     // "--" syntax, aluas for "x -= 1"
        Plus,       // Positive number literal or cast. Ex: +10, +getNumber()
        Minus,      // Negative number literal or cast. Ex: -10, -getNumber()
        Not         // Negate operator; turns a true value false and vice versa. Syntax: !
    };
private:
    // Only used in combination with Deref or Adress. Indicates
    // wether the pointer is raw or a reference.
    bool IsRawPtr = false;

    OpKind Op;
    Expr* Sub;
public:
    UnaryOperator(SrcSpan span, OpKind op, Expr* sub, bool isRawPtr = false)
        : Expr(ClassKind, span), Op(op), Sub(sub), IsRawPtr(isRawPtr) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }

    bool isRawPtrOp() const {
        assert(isPointerOp() && "This unary operator is not a pointer operator.\n");
        return IsRawPtr;
    }

    OpKind getOpKind() const {
        return Op;
    }

    Expr* getSubExpr() {
        return Sub;
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
        return Op == Adress;
    }

    bool isPointerOp() const {
        return isDerefOp() || isAdressOp(); 
    }
};

class CallExpr : public Expr {
public:
    static constexpr Kind ClassKind = CallExprKind;
private:
    // Pointer to the declaration of the callee. Can be either FunctionDecl* or StructDecl*
    NamedDecl* DC = nullptr;

    llvm::StringRef Callee;
    llvm::SmallVector<Expr*> Arguments = {};
public:
    CallExpr(SrcSpan span, llvm::StringRef callee)
        : Expr(ClassKind, span), Callee(callee) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }

    llvm::StringRef getCallee() const {
        return Callee;
    }

    NamedDecl* getCalleeDecl() {
        return DC;
    }

    const NamedDecl* getCalleeDecl() const {
        return DC;
    }

    void setCalleeDecl(NamedDecl* decl) {
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
};

class DeclRefExpr : public Expr {
public:
    static constexpr Kind ClassKind = DeclRefExprKind;
private:
    llvm::StringRef Name;

    // Can be FunctionDecl or VarDecl
    NamedDecl* DC = nullptr;
public:
    DeclRefExpr(SrcSpan span, llvm::StringRef name)
        : Expr(ClassKind, span), Name(name) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }
   
    llvm::StringRef getName() const {
        return Name;
    }

    void setDecl(NamedDecl* decl) {
        DC = decl;
    }

    NamedDecl* getDecl() {
        return DC;
    }

    const NamedDecl* getDecl() const {
        return DC;
    }
};

class ArrayLiteral : public Expr {
public:
    static constexpr Kind ClassKind = ArrayLiteralKind;
private:
    bool IsConstant = false;
    llvm::SmallVector<Expr*> Items;
public: 
    ArrayLiteral(SrcSpan span, llvm::SmallVector<Expr*>&& items = {})
        : Expr(ClassKind, span), Items(std::move(items)) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }

    bool isConstant() const {
        return IsConstant;
    }

    void setConstant(bool v) {
        IsConstant = v;
    }

    void setItems(llvm::SmallVector<Expr*>&& items) {
        Items = std::move(items);
    }

    size_t getAmntItems() const {
        return Items.size();
    }

    Expr* getItem(size_t idx) {
        return Items[idx];
    }

    const Expr* getItem(size_t idx) const {
        return Items[idx];
    }

    llvm::ArrayRef<Expr*> items() const {
        return Items;
    }
};

class IntegerLiteral : public Expr {
public:
    static constexpr Kind ClassKind = IntegerLiteralKind;
private:
    llvm::APInt Value;
public:
    IntegerLiteral(SrcSpan span, llvm::APInt value)
        : Expr(ClassKind, span), Value(value) {}

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
        : Expr(ClassKind, span), Value(value) {}

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
    BooleanLiteral(SrcSpan span, bool value)
        : Expr(ClassKind, span), Value(value) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }

    bool getValue() const {
        return Value;
    }
};

} // namespace ast
} // namespace c
