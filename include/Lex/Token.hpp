#pragma once

#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringExtras.h"

#include "include/AST/Type.hpp"
#include "include/IO/SrcSpan.hpp"

namespace c {

enum class TokenKind {
    Unknown,    // Unknown or uninitialized token. Mostly just used as a placeholder
    Eof,        // End Of File

    Comment,
    Indent,

    // Identifiers and literals
    Identifier, // Identifier name
    Integer,    // int literal declaration. For example 31415
    FloatPoint, // float literal declaration. For example 3.1415
    String,     // String literal declaration. For example "Hello World!"
    Character,  // Character literal declaration. For example 'c'

    // Keywords
    Const,      // Declare a compile-time constant
    Let,        // Declare immutable variable
    Mut,        // Declare mutable variable
    Type,       // Declare type aliases or unions
    Self,       // Used like "this" but as a reference, or as a type alias for the struct
    Pub,        // Declare an attribute or method of a struct or impl as public. Private by default
    From,       // Used in imports to import specific attributes or derived impls to a struct

    End,        // Used to declare the end of a scope in place of curly brackets

    If,         // Standard "if" statement
    Elif,       // Alias for "else if" statement
    Else,       // Standard "else" statement
    Match,      // Like "switch"
    Case,       // Used with "match" to create a case

    While,      // Standard "while" loop
    For,        // Standard "for" loop
    Break,      // Break from a loop
    Continue,   // Continue to the next iteration of the loop

    Fn,         // Used to declare a function
    Ret,        // Standard "return" statement

    Enum,       // Declare an enum class with advanced typing allowed
    Struct,     // Standard "Struct" declaration
    Impl,       // Used to declare functions for a struct
    Trait,      // Trait containing functions that can be inherited by a struct

    Import,     // Import other files/modules

    // Operators
    Plus,           // +    Positive number cast or result of a + b
    Minus,          // -    Negative number cast or result of a - b
    Star,           // *    Get/dereference raw pointer or result of a * b
    Slash,          // /    Result of A divided by B

    PlusPlus,       // ++   Increments by one
    MinusMinus,     // --   Decrements by one

    PlusEqual,      // +=   Increments a with b
    MinusEqual,     // -=   Decrements a with b
    StarEqual,      // *=   Multiplies a with b
    SlashEqual,     // /=   Divides    a with b

    EqualEqual,     // ==   a is equal to b
    ExclEqual,      // !=   a is not equal to b
    MoreEqual,      // >=   a is more than or equal to b
    LessEqual,      // <=   a is less than or equal to b

    Less,           // <    Less
    More,           // >    More

    AmpAmp,         // &&   Logical and
    PipePipe,       // ||   Logical or

    Exclamation,    // !    Regular and Bit "not" oprerator. Converts true to false and vice versa
    Equal,          // =    Assignment operator
    Ampersand,      // &    Get safe pointer or bit AND operator
    Pipe,           // |    Bit OR opreator

    ExclPipe,       // !|   Bit XOR opreator
    LShift,         // <<   Left bit shift
    RShift,         // >>   Right bit shift

    Arrow,          // ->

    // Punctuation
    Colon,          // :
    ColonColon,     // ::
    Comma,          // ,
    Dot,            // .

    LParen,         // (
    RParen,         // )
    LCurly,         // {
    RCurly,         // }
    LSquare,        // [
    RSquare,        // ]
};

class [[nodiscard]] Token {
private:
    SrcSpan Span = {};
    llvm::StringRef Data = {};
    TokenKind Kind = TokenKind::Unknown;
public:
    Token() = default;
    Token(TokenKind k, SrcSpan s, llvm::StringRef data)
        : Span(s), Data(data), Kind(k) {}
    Token(TokenKind k, SrcLoc loc, const char* data)
        : Span(loc, loc), Data(data, 1), Kind(k) {}
    ~Token() = default;

    Token(Token&&) = default;
    Token& operator=(Token&&) = default;

    Token(const Token&) = default;
    Token& operator=(const Token&) = default;

    TokenKind getType() const {
        return Kind;
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

    size_t getLength() const {
        return Span.getLength();
    }

    bool is(TokenKind k) const {
        return Kind == k;
    }

    bool isNot(TokenKind k) const {
        return Kind != k;
    }

    llvm::StringRef getData() const {
        return Data;
    }

    bool isLiteral() const;

    bool isKeyword() const;

    bool isOperator() const;
};

namespace lex {

inline bool isIdent(char c) {
    return c == '_' || llvm::isAlnum(c);
}

inline bool isIdentStart(char c) {
    return c == '_' || llvm::isAlpha(c);
}

llvm::StringRef strTokenKind(TokenKind type);

const llvm::StringMap<TokenKind>& getKeywords();

const llvm::StringMap<TokenKind>& getOperators();

const llvm::StringMap<BuiltinType::BuiltinKind>& getBuiltinMap();

} // namespace lex
} // namespace c
