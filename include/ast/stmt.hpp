#pragma once

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

#include "include/AST/Decl.hpp"
#include "include/AST/Expr.hpp"
#include "include/IO/SrcSpan.hpp"

// // Forward declarations instead of imports because intellisense
// // is retarded and has severe schizofrenia freak-outs otherwise
// namespace c {
// namespace ast {
// class Decl; // include/AST/Decl.hpp
// class Expr; // include/AST/Expr.hpp
// } // namespace ast
// // class Type; // include/AST/Type.hpp
// } // namespace c

namespace c {
namespace ast {

class Stmt {
public:
    enum Kind {
        firstStmt,

        firstStmtWithBody = firstStmt,

        firstConditionalStmt = firstStmtWithBody,
        IfStmtKind = firstConditionalStmt,
        WhileStmtKind,
        lastConditionalStmt = WhileStmtKind,

        ForStmtKind,     // NOTE: Planned but currently not implemented
        CaseStmtKind,    // NOTE: Planned but currently not implemented
        lastStmtWithBody = CaseStmtKind,

        BlockStmtKind,

        // Uses its own iterator different from StmtWithBody 
        // since the body only consists of CaseStmt
        MatchStmtKind,   // NOTE: Planned but currently not implemented

        ReturnStmtKind,
        DeclStmtKind,

        firstExpr,

        firstQualExpr = firstExpr,
        MoveExprKind = firstQualExpr,   // NOTE: Planned but currently not implemented
        CastExprKind,                   // NOTE: Planned but currently not implemented
        AccessExprKind, 
        SliceExprKind,
        lastQualExpr = SliceExprKind,

        BinaryOperatorKind, 
        UnaryOperatorKind,

        CallExprKind,
        DeclRefExprKind,
        
        ArrayLiteralKind,
        StringLiteralKind,  // NOTE: Planned but currently not implemented
        CharLiteralKind,    // NOTE: Planned but currently not implemented
        IntegerLiteralKind,
        FloatingLiteralKind,
        BooleanLiteralKind,

        lastExpr = BooleanLiteralKind,

        lastStmt = lastExpr
    };
private:
    Kind StmtKind;
    SrcSpan Span;
protected:
    Stmt(Kind SK, SrcSpan span)
        : StmtKind(SK), Span(span) {}
public:
    static bool classof(const Stmt* s) {
        return true;
    }

    Kind getKind() const {
        return StmtKind;
    }

    SrcSpan getSpan() const {
        return Span;
    }

    SrcLoc getStartLoc() const {
        return Span.getStart();
    }

    SrcLoc getEndLoc() const {
        return Span.getEnd();
    }
};

class StmtWithBody : public Stmt {
private:
    Stmt* Body;
protected:
    StmtWithBody(Kind SK, SrcSpan span, Stmt* body)
        : Stmt(SK, span), Body(body) {}
public:
    static bool classof(const Stmt* s) {
        return s->getKind() <= firstStmtWithBody && 
               s->getKind() <= lastStmtWithBody;
    }

    Stmt* getBody() {
        return Body;
    }

    const Stmt* getBody() const {
        return Body;
    }
};

class ConditionalStmt : public StmtWithBody {
private:
    Expr* Condition;
protected:
    ConditionalStmt(Kind SK, SrcSpan span, Expr* cond, Stmt* body)
        : StmtWithBody(SK, span, body), Condition(cond) {}
public:
    static bool classof(const Stmt* s) {
        return s->getKind() <= firstConditionalStmt && 
               s->getKind() <= lastConditionalStmt;
    }

    Expr* getCondition() {
        return Condition;
    }

    const Expr* getCondition() const {
        return Condition;
    }
};

class IfStmt : public ConditionalStmt {
public:
    static constexpr Kind ClassKind = IfStmtKind;
private:
    Stmt* Else;
public:
    IfStmt(SrcSpan span, Expr* cond, Stmt* body, Stmt* else_)
        : ConditionalStmt(ClassKind, span, cond, body), Else(else_) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }
    
    bool hasElse() const {
        return Else != nullptr;
    }

    Stmt* getElse() {
        return Else;
    }

    const Stmt* getElse() const {
        return Else;
    }
};

class WhileStmt : public ConditionalStmt {
public:
    static constexpr Kind ClassKind = WhileStmtKind;

    WhileStmt(SrcSpan span, Expr* cond, Stmt* body)
        : ConditionalStmt(ClassKind, span, cond, body) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }
};

class BlockStmt : public Stmt {
public:
    static constexpr Kind ClassKind = BlockStmtKind;
private:
    llvm::SmallVector<Stmt*> Stmts;
public:
    BlockStmt(SrcSpan span, llvm::SmallVector<Stmt*> stmts = {})
        : Stmt(ClassKind, span), Stmts(stmts) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }

    void pushStmt(Stmt* stmt) {
        Stmts.push_back(stmt);
    }

    size_t getAmntStmts() const {
        return Stmts.size();
    }

    llvm::ArrayRef<Stmt*> stmts() const {
        return Stmts;
    }

    llvm::MutableArrayRef<Stmt*> stmts() {
        return llvm::MutableArrayRef<Stmt*>(Stmts.begin(), Stmts.end());
    }
};

class ReturnStmt : public Stmt {
public:
    static constexpr Kind ClassKind = ReturnStmtKind;
private:
    Expr* Value;
public:
    ReturnStmt(SrcSpan span, Expr* value)
        : Stmt(ClassKind, span), Value(value) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }

    bool isVoid() const {
        return Value == nullptr;
    }

    const Expr* getValue() const {
        return Value;
    }

    Expr* getValue() {
        return Value;
    }
};

class DeclStmt : public Stmt {
public:
    static constexpr Kind ClassKind = DeclStmtKind;
private:
    NamedDecl* DC;
public:
    DeclStmt(SrcSpan span, NamedDecl* decl)
        : Stmt(ClassKind, span), DC(decl) {}

    static bool classof(const Stmt* s) {
        return s->getKind() == ClassKind;
    }
    
    const NamedDecl* getDecl() const {
        return DC;
    }

    NamedDecl* getDecl() {
        return DC;
    }
};

} // namespace ast
} // namespace c
