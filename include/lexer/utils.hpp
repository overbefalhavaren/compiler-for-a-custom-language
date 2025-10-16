#pragma once

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"

#include "include/lexer/token.hpp"

namespace c {
namespace lexer {

inline bool is_alpha(char c) { return llvm::isAlpha(c); }
inline bool is_digit(char c) { return llvm::isDigit(c); }
inline bool is_alnum(char c) { return llvm::isAlnum(c); }

inline bool is_space(char c) { return llvm::isSpace(c); }

inline bool is_ident_start(char c)  { return c == '_' || is_alpha(c); }
inline bool is_ident(char c)        { return c == '_' || is_alnum(c); }

inline bool is_operator(char c);

constexpr const llvm::StringMap<TokenType>& get_keywords();
constexpr const llvm::StringMap<TokenType>& get_operators();

} // namespace lexer
} // namespace c
