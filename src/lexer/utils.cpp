#include "include/lexer/utils.hpp"

using namespace c;

constexpr const llvm::StringMap<TokenType>& lexer::get_keywords() {
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

constexpr const llvm::StringMap<TokenType>& lexer::get_operators() {
    return llvm::StringMap<TokenType>({
        {   "+",    TokenType::Plus         },
        {   "-",    TokenType::Minus        },
        {   "*",    TokenType::Star         },
        {   "/",    TokenType::Slash        },

        {   "+=",   TokenType::PlusEqual    },
        {   "-=",   TokenType::MinusEqual   },
        {   "*=",   TokenType::TimesEqual   },
        {   "/=",   TokenType::DivideEqual  },

        {   "==",   TokenType::DoubleEqual  },
        {   "!=",   TokenType::NotEqual     },
        {   ">=",   TokenType::MTEqual      },
        {   "<=",   TokenType::LTEqual      },

        {   "&&",   TokenType::LogAND       },
        {   "||",   TokenType::LogOR        },
        {   "!|",   TokenType::BitXOR       },

        {   "!",    TokenType::Exclamation  },
        {   "=",    TokenType::Equal        },
        {   "&",    TokenType::Ampersand    },
        {   "|",    TokenType::Pipe         },

        {   ":",    TokenType::Colon        },
        {   "::",   TokenType::DoubleColon  },
        {   ",",    TokenType::Comma        },
        {   ".",    TokenType::Dot          },

        {   "->",   TokenType::RArrow       },

        {   ">>",   TokenType::LShift       },
        {   "<<",   TokenType::RShift       },

        {   ">",    TokenType::LAngle       },
        {   "<",    TokenType::RAngle       },
    });
}
