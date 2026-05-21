#pragma once

#include <optional>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBufferRef.h"
#include "llvm/Support/Error.h"

#include "include/Lexer/Token.hpp"
#include "include/Lexer/utils.hpp"
#include "include/IO/SrcSpan.hpp"

// TODO: Clean up comments

namespace c {

// FIXME: FIXME: FIXME:
// Bug in the lexer where it will skip over a 
// character if the character after it is EOF.
class Lexer {
private:
    FileID FID;
    llvm::StringRef Src;
    size_t Offset;
    // Location loc; // TODO: Implement when file stream manager class is implemented
public:
    Lexer(FileID id, llvm::StringRef source)
        : FID(id), Src(source), Offset(0) {}
    // Lexer(const llvm::MemoryBufferRef& source) 
    //     : Src(source.getBufferStart(), source.getBufferSize()), Offset(0) {}
    // Lexer(llvm::StringRef source, Location start) : src(std::move(source)), loc(std::move(start)) {}
    ~Lexer() = default;

    inline FileID getFileID() const {
        return FID;
    }

    inline llvm::StringRef getSource() const {
        return Src;
    }

    Token next() {
        skip_trivial();
        if (isEof()) 
            return Token(TokenType::Eof, createSpan(Offset));

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
    inline char peek() const {
        return Src[Offset]; 
    }

    inline void consume(size_t count) {
        Offset++;
    }

    inline bool isEof() const {
        return Offset >= Src.size() - 1;
    }

    inline SrcSpan createSpan(size_t start) const {
        return SrcSpan(FID, start, Offset);
    }

    void skip_trivial() {
        for (; !isEof(); consume(1))
            if (!lexer::is_space(peek()))
                break;
    }

    Token lexIdentifier() {
        size_t start = Offset;

        do {
            consume(1);
        } while (!isEof() && lexer::is_ident(peek()));

        
        TokenType type = TokenType::Identifier;
        llvm::StringRef lexeme = Src.slice(start, Offset);
        auto keywords = lexer::getKeywords();
        if (keywords.contains(lexeme))
            type = keywords[lexeme];

        return Token(type, createSpan(start), Src.slice(start, Offset));
    }

    Token lexNumber() {
        size_t start = Offset;

        do {
            consume(1);
        } while (!isEof() && lexer::is_digit(peek()));

        TokenType type = TokenType::Int;
        if (peek() == '.') {
            if (isEof()) {
                llvm::outs() << "Invalid float literal.\n";
                return Token(TokenType::Unknown, createSpan(Offset));
            }

            type = TokenType::Float;
            do {
                consume(1);
            } while (!isEof() && lexer::is_digit(peek()));
            // FIXME: This might allow incomplete float literals, for example "0.". Requires testing
        }

        return Token(type, createSpan(start), Src.slice(start, Offset));
    }

    // TODO: Implement
    Token lexStringLiteral() {
        assert(false && "String literals not yet supported.");
        return Token(TokenType::Unknown, createSpan(Offset));
    }

    // TODO: Implement
    Token lexCharLiteral() { 
        assert(false && "Char literals not yet supported.");
        return Token(TokenType::Unknown, createSpan(Offset));
    }

    Token lexOperatorOrPunct() {
        size_t start = Offset;

        // First character can't be EOF so save it and consume 1 to make it 
        // easier to match the second character for multi character expressions
        const char c = peek();
        consume(1);

        // This switch statement makes my head hurt...
        TokenType type;
        switch (c) {
            default:
                return Token(TokenType::Unknown, createSpan(start));

            // Punctuation
            case '(':  type = TokenType::LParen; break;      // (
            case ')':  type = TokenType::RParen; break;      // )
            case '{':  type = TokenType::LBrace; break;      // {
            case '}':  type = TokenType::RBrace; break;      // }
            case '[':  type = TokenType::LBrack; break;      // [
            case ']':  type = TokenType::RBrack; break;      // ]

            case '.':  type = TokenType::Dot; break;         // .
            case ',':  type = TokenType::Comma; break;       // ,

            case ':': 
                if (peek() == ':') {
                    if (!isEof()) consume(1);
                    type = TokenType::ColonColon;       // ::
                } else type = TokenType::Colon;         // :
                break;

            // Operators
            case '=': 
                if (peek() == '=') {
                    if (!isEof()) consume(1);
                    type = TokenType::EqualEqual;       // ==
                } else type = TokenType::Equal;         // =
                break;

            // Logical
            case '&': 
                if (peek() == '&') {
                    if (!isEof()) consume(1);
                    type = TokenType::AmpAmp;           // &&
                } else type = TokenType::Ampersand;     // &
                break;
            case '|': 
                if (peek() == '|') {
                    if (!isEof()) consume(1);
                    type = TokenType::PipePipe;            // ||
                } else type = TokenType::Pipe;          // |
                break;
            case '!': 
                if (peek() == '=') {
                    if (!isEof()) consume(1);
                    type = TokenType::ExclEqual;        // !=
                } else if (peek() == '|') {
                    if (!isEof()) consume(1);
                    type = TokenType::BitXOR;           // !|
                } else type = TokenType::Exclamation;   // !
                break;

            // Bit shift and logical
            case '<': 
                if (peek() == '=') {
                    if (!isEof()) consume(1);
                    type = TokenType::LessEqual;        // <=
                } else if (peek() == '<') {
                    if (!isEof()) consume(1);
                    type = TokenType::LShift;           // <<
                } else type = TokenType::Less;          // <
                break;
            case '>': 
                if (peek() == '=') {
                    if (!isEof()) consume(1);
                    type = TokenType::MoreEqual;        // >=
                } else if (peek() == '>') {
                    if (!isEof()) consume(1);
                    type = TokenType::RShift;           // >>
                } else type = TokenType::More;          // >
                break;

            // Compund assignment and mathematical
            case '*':  // And pointer
                if (peek() == '=') {
                    if (!isEof()) consume(1);
                    type = TokenType::StarEqual;        // *=
                } else type = TokenType::Star;          // *
                break;
            case '/': 
                if (peek() == '=') {
                    if (!isEof()) consume(1);
                    type = TokenType::SlashEqual;       // /=
                } else type = TokenType::Slash;         // /
                break;
            case '+': 
                if (peek() == '+') {
                    if (!isEof()) consume(1);
                    type = TokenType::PlusPlus;         // ++
                } else if (peek() == '=') {
                    if (!isEof()) consume(1);
                    type = TokenType::PlusEqual;        // +=
                } else type = TokenType::Plus;          // +
                break;

            case '-':  // And arrow
                if (peek() == '-') {
                    if (!isEof()) consume(1);
                    type = TokenType::MinusMinus;       // --
                } if (peek() == '=') {
                    if (!isEof()) consume(1);
                    type = TokenType::MinusEqual;       // -=
                } else if (peek() == '>') {
                    if (!isEof()) consume(1);
                    type = TokenType::Arrow;           // ->
                } else type = TokenType::Minus;         // -
                break;
        }

        return Token(type, createSpan(start));
    }
};

} // namespace c
