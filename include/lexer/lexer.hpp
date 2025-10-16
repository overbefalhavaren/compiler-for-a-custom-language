#pragma once

#include <optional>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"

#include "include/lexer/token.hpp"
#include "include/io/traceback.hpp"

namespace c {

class Lexer {
private:
    llvm::StringRef m_src;
    tb::Location m_loc; 
public:
    ExpectSpan<Token> next() noexcept;
private:
    inline bool eof() const noexcept    {   return m_loc.offset >= m_src.size();    }

    inline bool in_bounds(size_t index) const noexcept  {   return index >= m_src.size();   }

    inline tb::Location loc() const noexcept    {   return m_loc;           }
    inline size_t offset() const noexcept       {   return m_loc.offset;    }
    inline size_t line() const noexcept         {   return m_loc.line;      }
    inline size_t column() const noexcept       {   return m_loc.column;    }

    inline char peek() const noexcept           {   return m_src[m_loc.offset];                     }
    inline void advance(size_t count) noexcept  {   m_loc.offset += count; m_loc.column += count;   }
    inline void advance_line() noexcept         {   m_loc.line++; m_loc.column = 1;                 }

    void skip_trivia() noexcept;

    Token match_identifier() noexcept;
    ExpectSpan<Token> match_number() noexcept;
    ExpectSpan<Token> match_literal() noexcept;
    ExpectSpan<Token> match_operator() noexcept;
    TokenType match_parentheses() const noexcept;
};

} // namepsace c
