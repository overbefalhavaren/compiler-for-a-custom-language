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

llvm::StringRef strTokenType(TokenType type) {
    switch (type) {
        case TokenType::Unknown: return "Unknown";
        case TokenType::Eof: return "Eof";

        case TokenType::Identifier: return "Identifier";
        case TokenType::Int: return "Int";
        case TokenType::Float: return "Float";
        case TokenType::String: return "String";
        case TokenType::Character: return "Character";

        case TokenType::Const: return "Const";
        case TokenType::Let: return "Let";
        case TokenType::Mut: return "Mut";
        case TokenType::Temp: return "Temp";
        case TokenType::Type: return "Type";
        case TokenType::Move: return "Move";
        case TokenType::Self: return "Self";
        
        case TokenType::End: return "End";

        case TokenType::If: return "If";
        case TokenType::Elif: return "Elif";
        case TokenType::Else: return "Else";
        case TokenType::Match: return "Match";
        case TokenType::Case: return "Case";

        case TokenType::While: return "While";
        case TokenType::For: return "For";
        case TokenType::Break: return "Break";

        case TokenType::Fn: return "Fn";
        case TokenType::Return: return "Return";

        case TokenType::Enum: return "Enum";
        case TokenType::Struct: return "Struct";
        case TokenType::Impl: return "Impl";
        case TokenType::Trait: return "Trait";

        case TokenType::Import: return "Import";
        case TokenType::Export: return "Export";

        case TokenType::Plus: return "Plus";
        case TokenType::Minus: return "Minus";
        case TokenType::Star: return "Star";
        case TokenType::Slash: return "Slash";

        case TokenType::PlusPlus: return "PlusPlus";
        case TokenType::MinusMinus: return "MinusMinus";

        case TokenType::PlusEqual: return "PlusEqual";
        case TokenType::MinusEqual: return "MinusEqual";
        case TokenType::TimesEqual: return "TimesEqual";
        case TokenType::DivideEqual: return "DivideEqual";

        case TokenType::DoubleEqual: return "DoubleEqual";
        case TokenType::NotEqual: return "NotEqual";
        case TokenType::MTEqual: return "MTEqual";
        case TokenType::LTEqual: return "LTEqual";

        case TokenType::LogAND: return "LogAND";
        case TokenType::LogOR: return "LogOR";
        case TokenType::BitXOR: return "BitXOR";

        case TokenType::Exclamation: return "Exclamation";
        case TokenType::Equal: return "Equal";
        case TokenType::Ampersand: return "Ampersand";
        case TokenType::Pipe: return "Pipe";

        case TokenType::Colon: return "Colon";
        case TokenType::DoubleColon: return "DoubleColon";
        case TokenType::Comma: return "Comma";
        case TokenType::Dot: return "Dot";

        case TokenType::Arrow: return "Arrow";

        case TokenType::LShift: return "LShift";
        case TokenType::RShift: return "RShift";

        case TokenType::LAngle: return "LAngle";
        case TokenType::RAngle: return "RAngle";

        case TokenType::LParen: return "LParen";
        case TokenType::RParen: return "RParen";

        case TokenType::LBrace: return "LBrace";
        case TokenType::RBrace: return "RBrace";

        case TokenType::LBrack: return "LBrack";
        case TokenType::RBrack: return "RBrack";

        default:
            assert(false && "Unknown token can't be converted to string.");
            return ""; // To avoid compiler warning
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

    inline bool is(TokenType T) const { return type == T; }
    
    inline bool isOperator() const {
        switch (type) {
            case TokenType::Plus:
            case TokenType::Minus:
            case TokenType::Star:
            case TokenType::Slash:

            case TokenType::PlusPlus:
            case TokenType::MinusMinus:

            case TokenType::PlusEqual:
            case TokenType::MinusEqual:
            case TokenType::TimesEqual:
            case TokenType::DivideEqual:

            case TokenType::DoubleEqual:
            case TokenType::NotEqual:
            case TokenType::MTEqual:
            case TokenType::LTEqual:

            case TokenType::RAngle: // More than
            case TokenType::LAngle: // Less than

            case TokenType::LogAND:
            case TokenType::LogOR:
            case TokenType::BitXOR:

            case TokenType::Exclamation:
            case TokenType::Equal:
            case TokenType::Ampersand:
            case TokenType::Pipe:

            case TokenType::LShift:
            case TokenType::RShift:
                return true;

            default:
                return false;
        }
    }

    std::string str() const {
        return std::string() + "Token{type=TokenType::" + strTokenType(type).str() + ", lexeme=\"" + lexeme.str() + "\"}";
    }
};

} // namespace c
