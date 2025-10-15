#include "include/lexer/lexer.hpp"

#include "include/lexer/utils.hpp"

using namespace c;

// class Lexer
ExpectSpan<Token> Lexer::next() {
    if (eof()) 
        return Token{TokenType::Eof, "", m_loc};
    else skip_trivia();

    if (lexer::is_ident_start(peek())) {

    } else if ()
}

Token Lexer::match_identifier() {
    size_t start_offset = offset();

    do {
        advance(1);
    } while (!eof() && lexer::is_ident(peek()));

    llvm::StringRef lexeme = m_src.slice(start_offset, offset());

    TokenType type = TokenType::Identifier;
    auto keywords = lexer::get_keywords();
    if (keywords.contains(lexeme))
        type = keywords.find(lexeme)->second;

    tb::Location start{start_offset, line(), column()};
    return Token{type, std::move(start), loc()};
}

void Lexer::skip_trivia() {
    for (; !eof(); advance(1)) {
        if (peek() == '\n') {
            advance_line();
        } else if (lexer::is_space(peek()))
            advance(1);
    }
}
// Lexer
