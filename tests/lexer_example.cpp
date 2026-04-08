#include <string>

typedef enum TokenType {
    IDENTIFIER,
    INTLITERAL,
    PLUS,
    EQUAL,
    SEMICOLON,
    INT
} TokenType;

// void Token(TokenType type) {
//     return;
// }

// void Token(TokenType type, std::string data) {
//     return;
// }

class Token {
public:
    Token(TokenType type) {}
    Token(TokenType type, std::string data) {}
};


int main() {





    // Variable declaration in C:
    int x = 10 + 5;

    // Lexer output:
    { Token(INT),               // "int" keyword
      Token(IDENTIFIER, "x"),   // Identifier called "x"
      Token(EQUAL),             // "=" operator
      Token(INTLITERAL, "10"),  // Integer literal. Value: 10
      Token(PLUS),              // "+" operator
      Token(INTLITERAL, "5"),   // Integer literal. Value: 5
      Token(SEMICOLON)          // Punctuation: ";"
    }





    return 0;
}
