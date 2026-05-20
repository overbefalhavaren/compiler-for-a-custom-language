#pragma once

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
    ASTAllocator& Alloc;
    Token CurrentToken = Token();
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

    const Token& nextToken();

    bool parse(ast::ModuleDecl& result);
};

} // namespace c
