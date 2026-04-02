#ifndef LEXER_H
#define LEXER_H

/* Token kinds */
typedef enum {
    T_EOF,
    T_IDENT,
    T_INT_LIT,
    T_STR_LIT,
    T_DBL_LIT,
    /* Operators */
    T_PLUS,
    T_MINUS,
    T_STAR,
    T_SLASH,
    T_ASSIGN,
    T_EQ,
    T_NEQ,
    T_LT,
    T_GT,
    T_LTE,
    T_GTE,
    T_AND,
    T_OR,
    T_NOT,
    /* Punctuation */
    T_LPAREN,
    T_RPAREN,
    T_COMMA,
    T_AMPERSAND,
} TokenKind;

typedef struct {
    TokenKind kind;
    char      text[256];
    int       line;
} Token;

typedef struct {
    char *input;
    int   pos;
    int   line;
} Lexer;

void    lexer_init(Lexer *lex, char *source);
Token   lexer_next(Lexer *lex);
void    lexer_free(Lexer *lex);
const char *token_kind_str(TokenKind kind);

#endif
