#pragma once

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"

#include "include/lexer/token.hpp"

namespace c {
namespace lexer {

/*
 * is_alpha() -> A-Za-z
 * is_digit() -> 0-9
 * is_alnum() -> A-Za-z0-9
 * is_space() -> \\s\f\n\r\t\v
 * is_ident() -> _A-Za-z0-9
 * is_ident_start() -> _A-Za-z
 */

inline bool is_alpha(char c) { return llvm::isAlpha(c); }
inline bool is_digit(char c) { return llvm::isDigit(c); }
inline bool is_alnum(char c) { return llvm::isAlnum(c); }

inline bool is_space(char c) { return llvm::isSpace(c); }

inline bool is_ident_start(char c)  { return c == '_' || is_alpha(c); }
inline bool is_ident(char c)        { return c == '_' || is_alnum(c); }

// inline bool is_operator(char c);

constexpr const llvm::StringMap<TokenType>& get_keywords() {
    return llvm::StringMap<TokenType>({
        {   "const",    TokenType::Const    },
        {   "let",      TokenType::Let      },
        {   "mut",      TokenType::Mut      },
        {   "temp",     TokenType::Temp     },
        {   "type",     TokenType::Type     },
        {   "move",     TokenType::Move     },
        {   "self",     TokenType::Self     },

        {   "end",      TokenType::End      },

        {   "if",       TokenType::If       },
        {   "elif",     TokenType::Elif     },
        {   "else",     TokenType::Else     },
        {   "match",    TokenType::Match    },
        {   "case",     TokenType::Case     },

        {   "while",    TokenType::While    },
        {   "for",      TokenType::For      },
        {   "break",    TokenType::Break    },

        {   "fn",       TokenType::Fn       },
        {   "return",   TokenType::Return   },

        {   "enum",     TokenType::Enum     },
        {   "struct",   TokenType::Struct   },
        {   "impl",     TokenType::Impl     },
        {   "trait",    TokenType::Trait    },

        {   "import",   TokenType::Import   },
        {   "export",   TokenType::Export   }
    });
}

// constexpr const llvm::StringMap<TokenType>& get_operators();

} // namespace lexer
} // namespace c
