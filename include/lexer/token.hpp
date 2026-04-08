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
    Const,  // Declare a compile-time constant
    Let,    // Declare immutable variable
    Mut,    // Declare mutable variable
    Temp,   // Declare a scope or a variable that is deleted after one use
    Type,   // Declare type aliases or unions
    Move,   // Used to give up ownership, like std::move
    Self,   // Used like "this"
    Pub,    // Declare an attribute or method of a struct or impl as public. Private by default
    From,   // Used in imports to import specific attributes or derived impls to a struct

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

llvm::StringRef strTokenType(TokenType type) {
    switch (type) {
        case TokenType::Unknown:    return "Unknown";
        case TokenType::Eof:        return "Eof";

        case TokenType::Identifier: return "Identifier";
        case TokenType::Int:        return "Int";
        case TokenType::Float:      return "Float";
        case TokenType::String:     return "String";
        case TokenType::Character:  return "Character";

        case TokenType::Const:      return "Const";
        case TokenType::Let:        return "Let";
        case TokenType::Mut:        return "Mut";
        case TokenType::Temp:       return "Temp";
        case TokenType::Type:       return "Type";
        case TokenType::Move:       return "Move";
        case TokenType::Self:       return "Self";
        
        case TokenType::End:        return "End";

        case TokenType::If:         return "If";
        case TokenType::Elif:       return "Elif";
        case TokenType::Else:       return "Else";
        case TokenType::Match:      return "Match";
        case TokenType::Case:       return "Case";

        case TokenType::While:      return "While";
        case TokenType::For:        return "For";
        case TokenType::Break:      return "Break";

        case TokenType::Fn:         return "Fn";
        case TokenType::Return:     return "Return";

        case TokenType::Enum:       return "Enum";
        case TokenType::Struct:     return "Struct";
        case TokenType::Impl:       return "Impl";
        case TokenType::Trait:      return "Trait";

        case TokenType::Import:     return "Import";
        case TokenType::Export:     return "Export";

        case TokenType::Plus:       return "Plus";
        case TokenType::Minus:      return "Minus";
        case TokenType::Star:       return "Star";
        case TokenType::Slash:      return "Slash";

        case TokenType::PlusPlus:   return "PlusPlus";
        case TokenType::MinusMinus: return "MinusMinus";

        case TokenType::PlusEqual:  return "PlusEqual";
        case TokenType::MinusEqual: return "MinusEqual";
        case TokenType::StarEqual:  return "StarEqual";
        case TokenType::SlashEqual: return "SlashEqual";

        case TokenType::EqualEqual: return "DoubleEqual";
        case TokenType::ExclEqual:  return "NotEqual";
        case TokenType::MoreEqual:  return "MoreEqual";
        case TokenType::LessEqual:  return "LessEqual";

        case TokenType::AmpAmp:     return "AmpAmp";
        case TokenType::PipePipe:   return "PipePipe";
        case TokenType::BitXOR:     return "BitXOR";

        case TokenType::Exclamation:return "Exclamation";
        case TokenType::Equal:      return "Equal";
        case TokenType::Ampersand:  return "Ampersand";
        case TokenType::Pipe:       return "Pipe";

        case TokenType::Colon:      return "Colon";
        case TokenType::ColonColon: return "ColonColon";
        case TokenType::Comma:      return "Comma";
        case TokenType::Dot:        return "Dot";

        case TokenType::Arrow:      return "Arrow";

        case TokenType::LShift:     return "LShift";
        case TokenType::RShift:     return "RShift";

        case TokenType::More:       return "More";
        case TokenType::Less:       return "Less";

        case TokenType::LParen:     return "LParen";
        case TokenType::RParen:     return "RParen";

        case TokenType::LBrace:     return "LBrace";
        case TokenType::RBrace:     return "RBrace";

        case TokenType::LBrack:     return "LBrack";
        case TokenType::RBrack:     return "RBrack";

        default:
            assert(false && "Unknown token can't be converted to string.");
            return ""; // To avoid compiler warning
    }
}

class [[nodiscard]] Token {
    friend class Lexer; // So the lexer can get access to the constructor
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

    inline TokenType getType() const {
        return Type;
    }

    inline SrcSpan getSpan() const {
        return Span;
    }

    inline SrcLoc getStartLoc() const {
        return Span.getStart();
    }

    inline SrcLoc getEndLoc() const {
        return Span.getEnd();
    }

    inline size_t getLength() const {
        return Span.getLength();
    }

    inline bool is(TokenType T) const {
        return Type == T;
    }

    inline bool isNot(TokenType T) const {
        return Type != T;
    }

    inline llvm::StringRef getData() const {
        return Data;
    }

    bool isLiteral() const {
        return is(TokenType::String) || is(TokenType::Int) || is(TokenType::Float);
    }

    bool isKeyword() const {
        switch (Type) {
            default: 
                return false;
            case TokenType::Const:
            case TokenType::Let:
            case TokenType::Mut:
            case TokenType::Temp:
            case TokenType::Type:
            case TokenType::Move:
            case TokenType::Self:
            case TokenType::Pub:
            case TokenType::From:
            case TokenType::End:
            case TokenType::If:
            case TokenType::Elif:
            case TokenType::Else:
            case TokenType::Match:
            case TokenType::Case:
            case TokenType::While:
            case TokenType::For:
            case TokenType::Break:
            case TokenType::Fn:
            case TokenType::Return:
            case TokenType::Enum:
            case TokenType::Struct:
            case TokenType::Impl:
            case TokenType::Trait:
            case TokenType::Import:
            case TokenType::Export:
                return true;
        }
    }

    bool isOperator() const {
        switch (Type) {
            default:
                return false;
            case TokenType::Plus:
            case TokenType::Minus:
            case TokenType::Star:
            case TokenType::Slash:
            case TokenType::PlusPlus:
            case TokenType::MinusMinus:
            case TokenType::PlusEqual:
            case TokenType::MinusEqual:
            case TokenType::StarEqual:
            case TokenType::SlashEqual:
            case TokenType::EqualEqual:
            case TokenType::ExclEqual:
            case TokenType::MoreEqual:
            case TokenType::LessEqual:
            case TokenType::AmpAmp:
            case TokenType::PipePipe:
            case TokenType::Exclamation:
            case TokenType::Equal:
            case TokenType::Ampersand:
            case TokenType::Pipe:
            case TokenType::Less:
            case TokenType::More:
            case TokenType::LShift:
            case TokenType::RShift:
                return true;
        }
    }
};

/*
struct Token {
    TokenType type;
    SrcSpan span; // TODO: Switch "lexeme" for "span" when the file stream manager is implemented

    // TODO: Implement a file management system

    // Temporary lexeme attribute until proper spans and a file stream 
    // management system can be implemented so that you can get the lexeme 
    // from a span using the file stream manager.
    // llvm::StringRef lexeme; // TODO: Deprecate this

    Token() = default;
    // Token(TokenType type, llvm::StringRef lexeme) : type(type), lexeme(lexeme) {}

    // TODO: Implement when the file stream manager class is implemented
    Token(TokenType type, SrcSpan span) 
        : type(type), span(span) {}
    Token(TokenType type, SrcLoc offset) 
        : type(type), span(offset, offset) {}
    Token(TokenType type, SrcLoc start, SrcLoc end)
        : type(type), span(start, end) {}

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
            case TokenType::StarEqual:
            case TokenType::SlashEqual:

            case TokenType::EqualEqual:
            case TokenType::ExclEqual:
            case TokenType::MoreEqual:
            case TokenType::LessEqual:

            case TokenType::More:
            case TokenType::Less:

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

    inline llvm::StringRef getLexeme() const { assert(false); return lexeme; }

    std::string str() const {
        return std::string() + "Token{type=TokenType::" + strTokenType(type).str() + ", lexeme=\"" + getLexeme().str() + "\"}";
    }
};
*/

} // namespace c
