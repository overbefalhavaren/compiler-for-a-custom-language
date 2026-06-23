#pragma once

#include "llvm/ADT/StringRef.h"

#include "include/IO/Source.hpp"
#include "include/IO/SrcSpan.hpp"
#include "include/Lex/Token.hpp"

namespace c {

class Lexer {
private:
    Source& Src;
    llvm::StringRef Data;
    size_t Offset = 0;

    uint8_t IndentationSpaces = 4;
    bool LexIndentation = false;
    bool LexComments = false;
public:
    Lexer(Source& src, bool lexIndent = false, bool lexComments = false)
        : Src(src), Data(src.getBufferData()), LexIndentation(lexIndent), LexComments(lexComments) {}

    void setIndentationSpaces(uint8_t spaces) {
        IndentationSpaces = spaces;
    }

    void setLexIndentation(bool lexIndent) {
        LexIndentation = lexIndent;
    }

    void setLexComments(bool lexComments) {
        LexComments = lexComments;
    }

    Source& getSource() {
        return Src;
    }

    const Source& getSource() const {
        return Src;
    }

    llvm::StringRef getData() const {
        return Data;
    }

    bool isEOF() const {
        return Offset >= Data.size();
    }

    Token lex();

    Token lexSingleComment();

    Token lexIdentifier();

    Token lexNumber();

    Token lexLiteral(TokenKind kind, size_t start, llvm::StringRef closing);

    TokenKind lexOperator();
};

} // namespace c
