#pragma once

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"

#include "include/Lexer/Token.hpp"
#include "include/AST/Type.hpp"

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

inline bool is_alpha(char c) {
    return llvm::isAlpha(c);
}

inline bool is_digit(char c) {
    return llvm::isDigit(c); 
}

inline bool is_alnum(char c) {
    return llvm::isAlnum(c);
}

inline bool is_space(char c) {
    return llvm::isSpace(c);
}

inline bool is_ident_start(char c) {
    return c == '_' || is_alpha(c);
}

inline bool is_ident(char c) {
    return c == '_' || is_alnum(c);
}

inline llvm::StringMap<TokenType> getKeywords() {
    return llvm::StringMap<TokenType>({
        {   "const",    TokenType::Const    },
        {   "let",      TokenType::Let      },
        {   "mut",      TokenType::Mut      },
        {   "type",     TokenType::Type     },
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
    });
}

inline llvm::StringMap<BuiltinType::BuiltinKind> getBuiltins() {
    return llvm::StringMap<BuiltinType::BuiltinKind>({
        {   "bool",     BuiltinType::Bool   },

        {   "i8",       BuiltinType::I8     },
        {   "i16",      BuiltinType::I16    },
        {   "i32",      BuiltinType::I32    },
        {   "i64",      BuiltinType::I64    },

        {   "u8",       BuiltinType::U8     },
        {   "u16",      BuiltinType::U16    },
        {   "u32",      BuiltinType::U32    },
        {   "u64",      BuiltinType::U64    },
        
        {   "f32",      BuiltinType::F32    },
        {   "f64",      BuiltinType::F64    },
    });
}

} // namespace lexer
} // namespace c
