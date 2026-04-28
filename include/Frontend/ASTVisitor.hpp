#pragma once

#if true

#include <optional>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include "include/AST/Decl.hpp"
#include "include/AST/Expr.hpp"
#include "include/AST/Stmt.hpp"
#include "include/AST/Type.hpp"
#include "include/AST/TypeInfo.hpp"

namespace c {

struct FormatFlags {
    uint8_t IndentationSpaces = 4;
};

class ASTVisitor {
protected:
    size_t IndentLevel = 0;
    FormatFlags Flags;
    llvm::raw_ostream& Stream;

    ASTVisitor(FormatFlags flags, llvm::raw_ostream& stream)
        : Flags(flags), Stream(stream) {}
public:
    virtual ~ASTVisitor() = default;

    static FormatFlags getDefaultFlags();

    FormatFlags& getFlags() {
        return Flags;
    }

    const FormatFlags& getFlags() const {
        return Flags;
    }

    llvm::raw_ostream& getStream() {
        return Stream;
    }

    void indent() {
        IndentLevel++;
    }

    void dedent() {
        IndentLevel--;
    }

    void print(llvm::StringRef data, bool endLine = false, std::optional<size_t> indentLevel = std::nullopt, char indentChar = ' ');

    virtual void visitDecl(const ast::Decl& DC) = 0;

    virtual void visitStmt(const ast::Stmt& ST) = 0;

    virtual void visitExpr(const ast::Expr& EX) = 0;

    virtual void visitType(const ast::TypeInfo& Ty) = 0;
};

class ASTDumper : public ASTVisitor {
public:
    ASTDumper(FormatFlags flags, llvm::raw_ostream& stream)
        : ASTVisitor(flags, stream) {}

    void newline(bool isLast);

    void dump(const ast::Decl* DC) {
        visitDecl(*DC);
        Stream << '\n';
    }

    void dump(const ast::Stmt* ST) {
        visitStmt(*ST);
        Stream << '\n';
    }

    void dump(const ast::Expr* EX) {
        visitExpr(*EX);
        Stream << '\n';
    }

    void dump(const ast::TypeInfo* Ty) {
        visitType(*Ty);
        Stream << '\n';
    }

    void visitDecl(const ast::Decl& DC);

    void visitStmt(const ast::Stmt& ST);

    void visitExpr(const ast::Expr& EX);

    void visitType(const ast::TypeInfo& Ty);
};

// class ASTPrinter : public ASTFormatter;
// class ASTJSONDumper : public ASTFormatter;

} // namespace c

#endif
