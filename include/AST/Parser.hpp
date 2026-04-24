#pragma once

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/StringRef.h"

#include "include/Frontend/ASTAllocator.hpp"
#include "include/Lexer/Lexer.hpp"
#include "include/Lexer/Token.hpp"

namespace c {

namespace ast {
class ModuleDecl; // include/AST/Decl.hpp
}

class Parser {
private:
    Lexer& LexerSource;
    Token CurrentToken;
    ASTAllocator& Alloc;
public:
    Parser(ASTAllocator& alloc, Lexer& lexer)
        : LexerSource(lexer), Alloc(alloc) {
        (void)nextToken();
    }
    ~Parser() = default;

    ASTAllocator& getAllocator() {
        return Alloc;
    }

    Lexer& getLexer() {
        return LexerSource;
    }

    const Token& peekToken() const {
        return CurrentToken;
    }

    const Token& nextToken() {
        CurrentToken = LexerSource.next();
        llvm::outs() << "Next Token: " << strTokenType(CurrentToken.getType());
        if (CurrentToken.is(TokenType::Identifier) || CurrentToken.isLiteral())
            llvm::outs() << ": '" << CurrentToken.getData() << "'";
        llvm::outs() << "\n";
        return CurrentToken;
    }

    bool parse(ast::ModuleDecl& result);
};

} // namespace c
