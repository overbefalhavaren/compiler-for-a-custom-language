#pragma once

#include "llvm/ADT/StringRef.h"

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
    Const,  // Declare a compile-time constant
    Let,    // Declare immutable variable
    Mut,    // Declare mutable variable
    Temp,   // Declare a scope or a variable that is deleted after one use
    Type,   // Declare type aliases or unions
    Move,   // Used to give up ownership, like std::move
    Self,   // Used like "this"

    End,    // Used to declare the end of a scope in place of curly brackets

    If,     // Standard "if" statement
    Elif,   // Alias for "else if" statement
    Else,   // Standard "else" statement
    Match,  // Like "switch"
    Case,   // Used with "match"

    While,  // Standard "while" loop
    For,    // Standard "for" loop
    Break,  // Break from a loop

    Fn,     // Used to declare a function
    Return, // Standard "return" statement

    Enum,   // Declare an enum class with advanced typing allowed
    Struct, // Standard "Struct" declaration
    Impl,   // Used to declare functions for a struct
    Trait,  // Template containing functions that can be inherited by an impl

    Import, // Import other files/modules
    Export, // Used to declare what attributes of the file should be exported

    // Operators
    Plus,           // +
    Minus,          // -
    Star,           // *
    Slash,          // /

    PlusPlus,       // ++
    MinusMinus,     // --

    PlusEqual,      // +=
    MinusEqual,     // -=
    TimesEqual,     // *=
    DivideEqual,    // /=

    DoubleEqual,    // ==
    NotEqual,       // !=
    MTEqual,        // >=
    LTEqual,        // <=

    LogAND,         // &&
    LogOR,          // ||
/*  BitAND          // &  Same as "Ampersand"   */
/*  BitOR,          // |  Same as "Pipe"        */
    BitXOR,         // !|
/*  BitNOT,         // !  Same as "Exclamation" */

    Exclamation,    // !
    Equal,          // =
    Ampersand,      // &
    Pipe,           // |

    Colon,          // :
    DoubleColon,    // ::
    Comma,          // ,
    Dot,            // .

    Arrow,         // ->

    LShift, RShift, // << and >>
    LAngle, RAngle, // < and >
    LParen, RParen, // ( and )
    LBrace, RBrace, // { and }
    LBrack, RBrack, // [ and ]
};

constexpr llvm::StringRef strTokenType(TokenType type) {
    switch (type) {
        
    }
}

struct Token {
    TokenType type;
    // Span span; // TODO: Switch "lexeme" for "span" when the file stream manager is implemented

    // TODO: Implement a file management system

    // Temporary lexeme attribute until proper spans and a file stream 
    // management system can be implemented so that you can get the lexeme 
    // from a span using the file stream manager.
    llvm::StringRef lexeme; // TODO: Deprecate this

    Token() = default;
    Token(TokenType type, llvm::StringRef lexeme) : type(type), lexeme(lexeme) {}

    // TODO: Implement when the file stream manager class is implemented
    // Token(TokenType type, Span span);
    // Token(TokenType type, Location location);
    // Token(TokenType type, Location start, Location end);
};

} // namespace c
