#pragma once

#include <cassert>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

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
               s->getKind() >= lastExpr;
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

    bool isModifiableValue() const {
        // FIXME:
    }

    bool isStorageLocation() const {
        // NOTE: This switch statement only includes expression kinds that 
        // are implemented and have classes implemented for them.
        switch (getKind()) {
            default:
                llvm_unreachable("Expr subclass has no case implemented.");

            case UnaryOperatorKind:
                if (llvm::cast<UnaryOperator>(this)->isDerefOp())
                    return true;
                return false;
            case BinaryOperatorKind:
                // Expressions like "a = b = c" are allowed
                if (llvm::cast<BinaryOperator>(this)->isAssignOp()) // FIXME: Might be wrong
                    return true;
                return false;

            // TODO: When we implement things like function pointers, namespaces, 
            // templates and etc, DeclRefExpr will be able to point to almost any
            // kind of NamedDecl. Logic for this will need to be implemented.
            case DeclRefExprKind:
                // Conditional assertion to try save future headache.
                auto this_ = llvm::cast<DeclRefExpr>(this);
                if (this_->getDecl())
                    // Only VarDecl and ParamDecl have logic implemented for them.
                    assert(llvm::isa<InitDecl>(this_->getDecl()));
                return true;

            // TODO: When we add functions to structs in the future AccessExpr 
            // might also point to a function. This will need its own logic 
            // since a function can't be assigned to.
            case AccessExprKind:
                // No assertion here since the class methods will probably have
                // to change when logic for method access is implemented.
                return true;

            case SliceExprKind:
                // Both normal array access and the slice syntax return an
                // assignable memory location.
                return true;
            
            case CallExprKind:
            case ArrayLiteralKind:
            case FloatingLiteralKind:
            case IntegerLiteralKind:
            case BooleanLiteralKind:
                return false;
        }
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
    bool IsRawPtr;

    OpKind Op;
    Expr* Sub;
public:
    UnaryOperator(SrcSpan span, OpKind op, Expr* sub, bool isRawPtr = false)
        : Expr(ClassKind, nullptr, span), Op(op), Sub(sub), IsRawPtr(isRawPtr) {}

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
    Decl* DC;

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

    Decl* getCalleeDecl() {
        return DC;
    }

    const Decl* getCalleeDecl() const {
        return DC;
    }

    void setCalleeDecl(Decl* decl) {
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

class DeclRefExpr : public Expr {
public:
    static constexpr Kind ClassKind = DeclRefExprKind;
private:
    llvm::StringRef Name;
    Decl* DC;
public:
    DeclRefExpr(SrcSpan span, Type* T, llvm::StringRef name)
        : Expr(DeclRefExprKind, T, span), Name(name), DC(nullptr) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }
   
    llvm::StringRef getName() const {
        return Name;
    }

    void setDecl(ast::Decl* decl) {
        DC = decl;
    }

    Decl* getDecl() {
        return DC;
    }

    const Decl* getDecl() const {
        return DC;
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
