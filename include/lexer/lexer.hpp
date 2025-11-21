#pragma once

#include <optional>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBufferRef.h"
#include "llvm/Support/Error.h"

#include "include/lexer/token.hpp"
#include "include/lexer/utils.hpp"
// #include "include/io/source_location.hpp"

namespace c {

class Lexer {
private:
    llvm::StringRef Src;
    size_t Offset;
    // Location loc; // TODO: Implement when file stream manager class is implemented
public:
    Lexer(const llvm::MemoryBufferRef& source) : Src(source.getBufferStart(), source.getBufferSize()), Offset(0) {}
    // Lexer(llvm::StringRef source, Location start) : src(std::move(source)), loc(std::move(start)) {}
    ~Lexer() = default;

    Token next() {
        if (eof()) 
            return Token{TokenType::Eof, ""};
        skip_trivial();

        if (lexer::is_ident_start(peek())) {
            return lexIdentifier();
        } else if (lexer::is_digit(peek())) {
            return lexNumber();
        } else if (peek() == '"') {
            return lexStringLiteral();
        } else if (peek() == "'"[0]) { // Looks nicer than this abomination: '\''
            return lexCharLiteral();
        } else return lexOperatorOrPunct(); // Works as default, will return Unknown if invalid
    }
private:
    inline size_t offset() const { return Offset/*loc.offset()*/; }
    inline llvm::StringRef src() const { return Src; }

    inline bool eof() const { return offset() >= Src.size(); }

    inline char peek() const { return Src[offset()]; }
    inline void consume(size_t count) { Offset++/*(void)loc.add(count)*/; }

    void skip_trivial() {
        for (; !eof(); consume(1))
            if (!lexer::is_space(peek()))
                break;
    }

    Token lexIdentifier() {
        size_t start_offset = offset();

        do {
            consume(1);
        } while (lexer::is_ident(peek()));

        llvm::StringRef lexeme = src().slice(start_offset, offset());
        
        TokenType type = TokenType::Identifier;
        auto keywords = lexer::get_keywords();
        if (keywords.contains(lexeme))
            type = keywords[lexeme];

        return Token{type, std::move(lexeme)};
    }

    Token lexNumber() {
        size_t start_offset = offset();

        do {
            consume(1);
        } while (!eof() && lexer::is_digit(peek()));

        TokenType type = TokenType::Int;
        if (peek() == '.') {
            if (eof()) {
                llvm::outs() << "Invalid float literal.\n";
                return Token{TokenType::Unknown, ""};
            }

            type = TokenType::Float;
            do {
                consume(1);
            } while (!eof() && lexer::is_digit(peek()));
            // FIXME: This might allow incomplete float literals, for example "0.". Requires testing
        }

        return Token{type, src().slice(start_offset, offset())};
    }

    Token lexStringLiteral() {
        assert(false && "String literals not yet supported.");
    }

    Token lexCharLiteral() {
        assert(false && "Char literals not yet supported.");
    }

    Token lexOperatorOrPunct() {
        size_t start_offset = offset();

        // First character can't be EOF so save it and consume 1 to make it 
        // easier to match the second character for multi character expressions
        const char c = peek();
        consume(1);

        // This switch statement makes my head hurt...
        TokenType type;
        switch (c) {
            default:
                return Token{TokenType::Unknown, ""};

            // Punctuation
            case '(': { type = TokenType::LParen; }     // (
            case ')': { type = TokenType::RParen; }     // )
            case '{': { type = TokenType::LBrace; }     // {
            case '}': { type = TokenType::RBrace; }     // }
            case '[': { type = TokenType::LBrack; }     // [
            case ']': { type = TokenType::RBrack; }     // ]

            case '.': { type = TokenType::Dot; }        // .
            case ',': { type = TokenType::Comma; }      // ,

            case ':': {
                if (peek() == ':') {
                    if (!eof()) consume(1);
                    type = TokenType::DoubleColon;      // ::
                } else type = TokenType::Colon;         // :
            }

            // Operators
            case '=': {
                if (peek() == '=') {
                    if (!eof()) consume(1);
                    type = TokenType::DoubleEqual;      // ==
                } else type = TokenType::Equal;         // =
            }
            // Logical
            case '&': {
                if (peek() == '&') {
                    if (!eof()) consume(1);
                    type = TokenType::LogAND;           // &&
                } else type = TokenType::Ampersand;     // &
            }
            case '|': {
                if (peek() == '|') {
                    if (!eof()) consume(1);
                    type = TokenType::LogOR;            // ||
                } else type = TokenType::Pipe;          // |
            }
            case '!': {
                if (peek() == '=') {
                    if (!eof()) consume(1);
                    type = TokenType::NotEqual;         // !=
                } else if (peek() == '|') {
                    if (!eof()) consume(1);
                    type = TokenType::BitXOR;           // !|
                } else type = TokenType::Exclamation;   // !
            }
            // Bit shift and logical
            case '<': {
                if (peek() == '=') {
                    if (!eof()) consume(1);
                    type = TokenType::LTEqual;          // <=
                } else if (peek() == '<') {
                    if (!eof()) consume(1);
                    type = TokenType::LShift;           // <<
                } else type = TokenType::LAngle;        // <
            }
            case '>': {
                if (peek() == '=') {
                    if (!eof()) consume(1);
                    type = TokenType::MTEqual;          // >=
                } else if (peek() == '>') {
                    if (!eof()) consume(1);
                    type = TokenType::RShift;           // >>
                } else type = TokenType::RAngle;        // >
            }
            // Compund assignment and mathematical
            case '*': { // And pointer
                if (peek() == '=') {
                    if (!eof()) consume(1);
                    type = TokenType::TimesEqual;       // *=
                } else type = TokenType::Star;          // *
            }
            case '/': {
                if (peek() == '=') {
                    if (!eof()) consume(1);
                    type = TokenType::DivideEqual;      // /=
                } else type = TokenType::Slash;         // /
            }
            case '+': {
                if (peek() == '+') {
                    if (!eof()) consume(1);
                    type = TokenType::PlusPlus;         // ++
                } else if (peek() == '=') {
                    if (!eof()) consume(1);
                    type = TokenType::PlusEqual;        // +=
                } else type = TokenType::Plus;          // +
            }
            case '-': { // And arrow
                if (peek() == '-') {
                    if (!eof()) consume(1);
                    type = TokenType::MinusMinus;       // --
                } if (peek() == '=') {
                    if (!eof()) consume(1);
                    type = TokenType::MinusEqual;       // -=
                } else if (peek() == '>') {
                    if (!eof()) consume(1);
                    type = TokenType::Arrow;           // ->
                } else type = TokenType::Minus;         // -
            } 
        }

        return Token{type, src().slice(start_offset, offset())};
    }
};

} // namespace c
