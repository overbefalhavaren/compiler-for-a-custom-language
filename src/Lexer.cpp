#include "include/Lex/Lexer.hpp"
#include "include/Lex/Token.hpp"

#include <string>

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ErrorHandling.h"

#include "include/IO/SrcSpan.hpp"

namespace c {

Token Lexer::lex() {
    static bool is_line_start = false;
    if (isEOF())
        return Token(TokenKind::Eof,
                     SrcLoc(Src.getFileID(), Offset),
                     Data.data() + Offset);

    if (Offset != 0)
        Offset++;

    for (; !isEOF(); Offset++) {
        char c = Data[Offset];
        if (is_line_start && LexIndentation) {
            if (c == '\t') {
                return Token(TokenKind::Indent,
                             SrcLoc(Src.getFileID(), Offset),
                             Data.data() + Offset);
            } else if (Data.slice(Offset, Data.size()
                       ).starts_with(std::string(' ', IndentationSpaces))) {
                size_t start = Offset;
                Offset += IndentationSpaces;
                return Token(TokenKind::Indent,
                             SrcSpan(Src.getFileID(), start, Offset),
                             Data.slice(start, Offset));
            }
        }

        if (Data[Offset] == '\n') {
            is_line_start = true;
        } else if (!llvm::isSpace(Data[Offset]))
            break;
    }

    if (!isEOF() && Data[Offset] == '/') {
        std::optional<Token> result = std::nullopt;
        char next = Data[Offset + 1];
        if (next == '/') {
            Offset += 2;
            result = lexSingleComment();
        } else if (next == '*') {
            Offset += 2;
            result = lexLiteral(TokenKind::Comment, Offset - 2, "*/");
        }

        if (LexComments) {
            return *result;
        } else if (result)
            return lex();
    }

    if (llvm::isDigit(Data[Offset])) {
        return lexNumber();
    } else if (lex::isIdentStart(Data[Offset])) {
        return lexIdentifier();
    }

    size_t start = Offset;
    TokenKind kind = lexOperator();
    if (kind == TokenKind::String) {
        return lexLiteral(kind, start, "\"");
    } else if (kind == TokenKind::Character) {
        return lexLiteral(kind, start, "'");
    } else if (kind == TokenKind::Unknown)
        return Token(TokenKind::Unknown,
                     SrcLoc(Src.getFileID(), start),
                     Data.data() + Offset);

    SrcSpan span(Src.getFileID(), start, Offset);
    return Token(kind, std::move(span), Data.slice(start, Offset));
}

Token Lexer::lexSingleComment() {
    size_t start = Offset;

    do {
        Offset++;
    } while (!isEOF() && Data[Offset] != '\n');

    SrcSpan span(Src.getFileID(), start, Offset);
    return Token(TokenKind::Comment, std::move(span), Data.slice(start, Offset));
}

Token Lexer::lexIdentifier() {
    size_t start = Offset;

    do {
        Offset++;
    } while (lex::isIdent(Data[Offset]));

    TokenKind kind = TokenKind::Identifier;
    llvm::StringRef lexeme = Data.slice(start, Offset);

    auto keywords = lex::getKeywords();
    auto result = keywords.find(lexeme);
    if (result != keywords.end())
        kind = result->second;

    SrcSpan span(Src.getFileID(), start, Offset);
    return Token(kind, std::move(span), lexeme);
}

Token Lexer::lexNumber() {
    size_t start = Offset;

    do {
        Offset++;
    } while (llvm::isDigit(Data[Offset]));

    TokenKind kind = TokenKind::Integer;
    if (Data[Offset] == '.') {
        kind = TokenKind::FloatPoint;
        while (llvm::isDigit(Data[Offset]))
            Offset++;
    }

    SrcSpan span(Src.getFileID(), start, Offset);
    return Token(kind, std::move(span), Data.slice(start, Offset));
}

TokenKind Lexer::lexOperator() {
    char start = Data[Offset];
    switch (start) {
        default:
            return TokenKind::Unknown;

        case '(':
            return TokenKind::LParen;

        case ')':
            return TokenKind::RParen;

        case '[':
            return TokenKind::LSquare;

        case ']':
            return TokenKind::RSquare;

        case '{':
            return TokenKind::LCurly;

        case '}':
            return TokenKind::RCurly;

        case ',':
            return TokenKind::Comma;

        case '.':
            return TokenKind::Dot;

        case '"':
            return TokenKind::String;

        case '\'':
            return TokenKind::Character;

        case '+':
            if (Data[Offset] == '+') {
                Offset++;
                return TokenKind::PlusPlus;
            }

            if (Data[Offset] == '=') {
                Offset++;
                return TokenKind::PlusEqual;
            }

            return TokenKind::Plus;

        case '-':
            if (Data[Offset] == '-') {
                Offset++;
                return TokenKind::MinusMinus;
            }

            if (Data[Offset] == '=') {
                Offset++;
                return TokenKind::MinusEqual;
            }

            if (Data[Offset] == '>') {
                Offset++;
                return TokenKind::Arrow;
            }

            return TokenKind::Minus;

        case '*':
            if (Data[Offset] == '=') {
                Offset++;
                return TokenKind::StarEqual;
            }

            return TokenKind::Star;

        case '/':
            if (Data[Offset] == '=') {
                Offset++;
                return TokenKind::SlashEqual;
            }

            return TokenKind::Slash;

        case '=':
            if (Data[Offset] == '=') {
                Offset++;
                return TokenKind::EqualEqual;
            }

            return TokenKind::Equal;

        case '<':
            if (Data[Offset] == '<') {
                Offset++;
                return TokenKind::LShift;
            }

            if (Data[Offset] == '=') {
                Offset++;
                return TokenKind::LessEqual;
            }

            return TokenKind::Less;

        case '>':
            if (Data[Offset] == '>') {
                Offset++;
                return TokenKind::RShift;
            }

            if (Data[Offset] == '=') {
                Offset++;
                return TokenKind::MoreEqual;
            }

            return TokenKind::More;

        case '&':
            if (Data[Offset] == '&') {
                Offset++;
                return TokenKind::AmpAmp;
            }

            return TokenKind::Ampersand;

        case '|':
            if (Data[Offset] == '|') {
                Offset++;
                return TokenKind::PipePipe;
            }

            return TokenKind::Pipe;

        case '!':
            if (Data[Offset] == '=') {
                Offset++;
                return TokenKind::ExclEqual;
            }

            if (Data[Offset] == '|') {
                Offset++;
                return TokenKind::ExclPipe;
            }

            return TokenKind::Exclamation;

        case ':':
            if (Data[Offset] == ':') {
                Offset++;
                return TokenKind::ColonColon;
            }

            return TokenKind::Colon;
    }
}

Token Lexer::lexLiteral(TokenKind kind, size_t start, llvm::StringRef closing) {
    for (; !isEOF(); Offset++)
        if (Data.slice(Offset, Data.size()).starts_with(closing))
            break;

    Offset += closing.size();
    SrcSpan span(Src.getFileID(), start, Offset);
    return Token(kind, std::move(span), Data.slice(start, Offset));
}

bool Token::isLiteral() const {
    switch (Kind) {
        default:
            return false;
        case TokenKind::Integer:
        case TokenKind::FloatPoint:
        case TokenKind::String:
        case TokenKind::Character:
            return true;
    }
}

bool Token::isKeyword() const {
    switch (Kind) {
        default:
            return false;
        case TokenKind::Const:
        case TokenKind::Let:
        case TokenKind::Mut:
        case TokenKind::Type:
        case TokenKind::Self:
        case TokenKind::Pub:
        case TokenKind::From:
        case TokenKind::End:
        case TokenKind::If:
        case TokenKind::Elif:
        case TokenKind::Else:
        case TokenKind::Match:
        case TokenKind::Case:
        case TokenKind::While:
        case TokenKind::For:
        case TokenKind::Break:
        case TokenKind::Fn:
        case TokenKind::Ret:
        case TokenKind::Enum:
        case TokenKind::Struct:
        case TokenKind::Impl:
        case TokenKind::Trait:
        case TokenKind::Import:
            return true;
    }
}

bool Token::isOperator() const {
    switch (Kind) {
        default:
            return false;
        case TokenKind::Plus:
        case TokenKind::Minus:
        case TokenKind::Star:
        case TokenKind::Slash:
        case TokenKind::PlusPlus:
        case TokenKind::MinusMinus:
        case TokenKind::PlusEqual:
        case TokenKind::MinusEqual:
        case TokenKind::StarEqual:
        case TokenKind::SlashEqual:
        case TokenKind::EqualEqual:
        case TokenKind::ExclEqual:
        case TokenKind::MoreEqual:
        case TokenKind::LessEqual:
        case TokenKind::AmpAmp:
        case TokenKind::PipePipe:
        case TokenKind::Exclamation:
        case TokenKind::Equal:
        case TokenKind::Ampersand:
        case TokenKind::Pipe:
        case TokenKind::Less:
        case TokenKind::More:
        case TokenKind::LShift:
        case TokenKind::RShift:
            return true;
    }
}

namespace lex {

llvm::StringRef strTokenKind(TokenKind type) {
    switch (type) {
        default:
            llvm_unreachable("Unknown token can't be converted to string.");

        case TokenKind::Unknown:    return "Unknown";
        case TokenKind::Eof:        return "Eof";

        case TokenKind::Comment:    return "Comment";
        case TokenKind::Indent:     return "Indent";

        case TokenKind::Identifier: return "Identifier";
        case TokenKind::Integer:    return "Integer";
        case TokenKind::FloatPoint: return "FloatPoint";
        case TokenKind::String:     return "String";
        case TokenKind::Character:  return "Character";

        case TokenKind::Const:      return "Const";
        case TokenKind::Let:        return "Let";
        case TokenKind::Mut:        return "Mut";
        case TokenKind::Type:       return "Type";
        case TokenKind::Self:       return "Self";

        case TokenKind::End:        return "End";

        case TokenKind::If:         return "If";
        case TokenKind::Elif:       return "Elif";
        case TokenKind::Else:       return "Else";
        case TokenKind::Match:      return "Match";
        case TokenKind::Case:       return "Case";

        case TokenKind::While:      return "While";
        case TokenKind::For:        return "For";
        case TokenKind::Break:      return "Break";

        case TokenKind::Fn:         return "Fn";
        case TokenKind::Ret:        return "Ret";

        case TokenKind::Enum:       return "Enum";
        case TokenKind::Struct:     return "Struct";
        case TokenKind::Impl:       return "Impl";
        case TokenKind::Trait:      return "Trait";

        case TokenKind::Import:     return "Import";

        case TokenKind::Plus:       return "Plus";
        case TokenKind::Minus:      return "Minus";
        case TokenKind::Star:       return "Star";
        case TokenKind::Slash:      return "Slash";

        case TokenKind::PlusPlus:   return "PlusPlus";
        case TokenKind::MinusMinus: return "MinusMinus";

        case TokenKind::PlusEqual:  return "PlusEqual";
        case TokenKind::MinusEqual: return "MinusEqual";
        case TokenKind::StarEqual:  return "StarEqual";
        case TokenKind::SlashEqual: return "SlashEqual";

        case TokenKind::EqualEqual: return "DoubleEqual";
        case TokenKind::ExclEqual:  return "NotEqual";
        case TokenKind::MoreEqual:  return "MoreEqual";
        case TokenKind::LessEqual:  return "LessEqual";

        case TokenKind::AmpAmp:     return "AmpAmp";
        case TokenKind::PipePipe:   return "PipePipe";
        case TokenKind::ExclPipe:   return "ExclPipe";

        case TokenKind::Exclamation:return "Exclamation";
        case TokenKind::Equal:      return "Equal";
        case TokenKind::Ampersand:  return "Ampersand";
        case TokenKind::Pipe:       return "Pipe";

        case TokenKind::Colon:      return "Colon";
        case TokenKind::ColonColon: return "ColonColon";
        case TokenKind::Comma:      return "Comma";
        case TokenKind::Dot:        return "Dot";

        case TokenKind::Arrow:      return "Arrow";

        case TokenKind::LShift:     return "LShift";
        case TokenKind::RShift:     return "RShift";

        case TokenKind::More:       return "More";
        case TokenKind::Less:       return "Less";

        case TokenKind::LParen:     return "LParen";
        case TokenKind::RParen:     return "RParen";

        case TokenKind::LCurly:     return "LCurly";
        case TokenKind::RCurly:     return "RCurly";

        case TokenKind::LSquare:    return "LSquare";
        case TokenKind::RSquare:    return "RSquare";
    }
}

const llvm::StringMap<TokenKind>& getKeywords() {
    static llvm::StringMap<TokenKind> Keywords({
        {   "const",    TokenKind::Const    },
        {   "let",      TokenKind::Let      },
        {   "mut",      TokenKind::Mut      },
        {   "type",     TokenKind::Type     },
        {   "self",     TokenKind::Self     },

        {   "end",      TokenKind::End      },

        {   "if",       TokenKind::If       },
        {   "elif",     TokenKind::Elif     },
        {   "else",     TokenKind::Else     },
        {   "match",    TokenKind::Match    },
        {   "case",     TokenKind::Case     },

        {   "while",    TokenKind::While    },
        {   "for",      TokenKind::For      },
        {   "break",    TokenKind::Break    },

        {   "fn",       TokenKind::Fn       },
        {   "ret",      TokenKind::Ret      },

        {   "enum",     TokenKind::Enum     },
        {   "struct",   TokenKind::Struct   },
        {   "impl",     TokenKind::Impl     },
        {   "trait",    TokenKind::Trait    },

        {   "import",   TokenKind::Import   },
    });

    return Keywords;
}

const llvm::StringMap<TokenKind>& getOperators() {
    static llvm::StringMap<TokenKind> Operators({
        {   "+",    TokenKind::Plus         },
        {   "-",    TokenKind::Minus        },
        {   "*",    TokenKind::Star         },
        {   "/",    TokenKind:: Slash       },

        {   "++",   TokenKind::PlusPlus     },
        {   "--",   TokenKind::MinusMinus   },

        {   "+=",   TokenKind::PlusEqual    },
        {   "-=",   TokenKind::MinusEqual   },
        {   "*=",   TokenKind::StarEqual    },
        {   "/=",   TokenKind::SlashEqual   },

        {   "==",   TokenKind::EqualEqual   },
        {   "!=",   TokenKind::SlashEqual   },
        {   ">=",   TokenKind::MoreEqual    },
        {   "<=",   TokenKind::LessEqual    },

        {   "&&",   TokenKind::AmpAmp       },
        {   "||",   TokenKind::PipePipe     },
        {   "!|",   TokenKind::ExclPipe     },

        {   "!",    TokenKind::Exclamation  },
        {   "=",    TokenKind::Equal        },
        {   "&",    TokenKind::Ampersand    },
        {   "|",    TokenKind::Pipe         },

        {   "::",   TokenKind::ColonColon   },
        {   ".",    TokenKind::Dot          },

        {   "->",   TokenKind::Arrow        },

        {   "<",    TokenKind::Less         },
        {   ">",    TokenKind::More         },
        {   "<<",   TokenKind::LShift       },
        {   ">>",   TokenKind::RShift       },
    });

    return Operators;
}

const llvm::StringMap<BuiltinType::BuiltinKind>& getBuiltinMap() {
    static llvm::StringMap<BuiltinType::BuiltinKind> Builtins({
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

    return Builtins;
}

} // namespace lex

} // namespace c
