#pragma once

#include "llvm/ADT/StringRef.h"

#include "include/IO/SrcSpan.hpp"

namespace c {

enum class TokenType {
    Unknown,    // Unknown or uninitialized token. Mostly just used as a placeholder
    Eof,        // End Of File  
    
    // Identifiers and literals
    Identifier, // Identifier name
    Int,        // int literal declaration. For example 31415
    Float,      // float literal declaration. For example 3.1415
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
    Return,     // Standard "return" statement

    Enum,       // Declare an enum class with advanced typing allowed
    Struct,     // Standard "Struct" declaration
    Impl,       // Used to declare functions for a struct
    Trait,      // Trait containing functions that can be inherited by a struct

    Import,     // Import other files/modules

    // Operators
    Plus,           // +
    Minus,          // -
    Star,           // *
    Slash,          // /

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

    AmpAmp,         // &&
    PipePipe,       // ||
/*  BitAND          // &  Same as "Ampersand"   */
/*  BitOR,          // |  Same as "Pipe"        */
    BitXOR,         // !|
/*  BitNOT,         // !  Same as "Exclamation" */

    Exclamation,    // !
    Equal,          // =    Assignment operator
    Ampersand,      // &
    Pipe,           // |

    Colon,          // :
    ColonColon,     // ::
    Comma,          // ,
    Dot,            // .

    Arrow,          // ->   Range Operator

    LShift, RShift, // << and >>
    Less, More,     // < and >  
    LParen, RParen, // ( and )
    LBrace, RBrace, // { and }
    LBrack, RBrack, // [ and ]
};

llvm::StringRef strTokenType(TokenType type);

class [[nodiscard]] Token {
private:    
    TokenType Type;
    SrcSpan Span;

    // Identifier or data for literals and identifiers.
    llvm::StringRef Data;
public:
    Token() = default;
    Token(TokenType type, SrcSpan&& span) 
        : Type(type), Span(std::move(span)) {}
    Token(TokenType type, SrcSpan&& span, llvm::StringRef data)
        : Type(type), Span(std::move(span)), Data(data) {}
    ~Token() = default;

    Token(Token&&) = default;
    Token& operator=(Token&&) = default;

    Token(const Token&) = default;
    Token& operator=(const Token&) = default;

    TokenType getType() const {
        return Type;
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

    bool is(TokenType T) const {
        return Type == T;
    }

    bool isNot(TokenType T) const {
        return Type != T;
    }

    llvm::StringRef getData() const {
        return Data;
    }

    bool isLiteral() const;

    bool isKeyword() const;

    bool isOperator() const;
};

} // namespace c
