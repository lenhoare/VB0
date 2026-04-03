#include "../include/lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

static int is_keyword_start(char c)
{
    return isalpha(c) || c == '_';
}

/* Keywords -- all uppercase, case-insensitive matching happens later */
static const char *keywords[] = {
    "AND", "AS", "DIM", "DO", "ELSE", "EXIT", "FOR", "FUNCTION",
    "IF", "INT", "DOUBLE", "STRING", "LOOP", "NEXT", "NOT",
    "OR", "PRINT", "RETURN", "SUB", "THEN", "TO", "STEP",
    "TRUE", "FALSE", "EXIT_DO", "EXIT_FOR", "END",
    NULL
};

TokenKind lookup_keyword(const char *text)
{
    char upper[256];
    int i;
    for (i = 0; text[i] && i < 255; i++)
        upper[i] = toupper((unsigned char)text[i]);
    upper[i] = '\0';

    if (strcmp(upper, "AND") == 0) return T_AND;
    if (strcmp(upper, "OR") == 0) return T_OR;
    if (strcmp(upper, "NOT") == 0) return T_NOT;
    /* Note: most keywords are handled in the parser by string comparison
     * because the parser already has the text, but we mark them as
     * identifiers in the lexer. */
    return T_IDENT;
}

void lexer_init(Lexer *lex, char *source)
{
    lex->input = source;
    lex->pos = 0;
    lex->line = 1;
}

static char peek(Lexer *lex)
{
    if (lex->input[lex->pos] == '\0') return '\0';
    return lex->input[lex->pos];
}

static char advance(Lexer *lex)
{
    char c = lex->input[lex->pos];
    if (c == '\n') lex->line++;
    lex->pos++;
    return c;
}

static void skip_ws(Lexer *lex)
{
    while (1) {
        while (peek(lex) == ' ' || peek(lex) == '\t' ||
               peek(lex) == '\r' || peek(lex) == '\n')
            advance(lex);
        /* Skip comments: ' to end of line */
        if (peek(lex) == '\'') {
            while (peek(lex) != '\0' && peek(lex) != '\n')
                advance(lex);
            /* Loop back to consume the \n we stopped at */
            continue;
        }
        break;
    }
}

const char *token_kind_str(TokenKind kind)
{
    switch (kind) {
        case T_EOF:      return "EOF";
        case T_IDENT:    return "IDENT";
        case T_INT_LIT:  return "INT";
        case T_STR_LIT:  return "STRING";
        case T_DBL_LIT:  return "DOUBLE";
        case T_PLUS:     return "+";
        case T_MINUS:    return "-";
        case T_STAR:     return "*";
        case T_SLASH:    return "/";
        case T_ASSIGN:   return "=";
        case T_EQ:       return "==";
        case T_NEQ:      return "<>";
        case T_LT:       return "<";
        case T_GT:       return ">";
        case T_LTE:      return "<=";
        case T_GTE:      return ">=";
        case T_AND:      return "AND";
        case T_OR:       return "OR";
        case T_NOT:      return "NOT";
        case T_LPAREN:   return "(";
        case T_RPAREN:   return ")";
        case T_COMMA:    return ",";
        case T_AMPERSAND: return "&";
        default:         return "UNKNOWN";
    }
}

Token lexer_next(Lexer *lex)
{
    Token tok;
    memset(&tok, 0, sizeof(tok));
    tok.line = lex->line;
    tok.kind = T_EOF;

    skip_ws(lex);
    if (peek(lex) == '\0') {
        return tok;
    }

    char c = peek(lex);

    /* String literal */
    if (c == '"') {
        advance(lex); /* skip opening quote */
        int i = 0;
        while (peek(lex) != '\0' && peek(lex) != '"') {
            tok.text[i++] = advance(lex);
        }
        tok.text[i] = '\0';
        token_kind_str(T_EOF); /* silence unused warning */
        if (peek(lex) == '"') advance(lex); /* skip closing quote */
        tok.kind = T_STR_LIT;
        tok.line = lex->line;
        return tok;
    }

    /* Number literals */
    if (isdigit(c) || (c == '.' && isdigit(lex->input[lex->pos+1]))) {
        int i = 0;
        int has_dot = 0;
        while (isdigit(peek(lex)) || peek(lex) == '.') {
            if (peek(lex) == '.') {
                if (has_dot) break;
                has_dot = 1;
            }
            tok.text[i++] = advance(lex);
        }
        tok.text[i] = '\0';
        tok.kind = has_dot ? T_DBL_LIT : T_INT_LIT;
        return tok;
    }

    /* Identifiers and keywords */
    if (is_keyword_start(c)) {
        int i = 0;
        while (isalnum(peek(lex)) || peek(lex) == '_')
            tok.text[i++] = advance(lex);
        tok.text[i] = '\0';
        tok.kind = lookup_keyword(tok.text);
        return tok;
    }

    /* Operators and punctuation */
    switch (c) {
        case '+': advance(lex); tok.kind = T_PLUS; break;
        case '-': advance(lex); tok.kind = T_MINUS; break;
        case '*': advance(lex); tok.kind = T_STAR; break;
        case '/': advance(lex); tok.kind = T_SLASH; break;
        case '(': advance(lex); tok.kind = T_LPAREN; break;
        case ')': advance(lex); tok.kind = T_RPAREN; break;
        case ',': advance(lex); tok.kind = T_COMMA; break;
        case '=': advance(lex); tok.kind = T_ASSIGN; break;
        case '<':
            advance(lex);
            if (peek(lex) == '>') {
                advance(lex);
                tok.kind = T_NEQ;
            } else if (peek(lex) == '=') {
                advance(lex);
                tok.kind = T_LTE;
            } else {
                tok.kind = T_LT;
            }
            break;
        case '>':
            advance(lex);
            if (peek(lex) == '=') {
                advance(lex);
                tok.kind = T_GTE;
            } else {
                tok.kind = T_GT;
            }
            break;
        case '&':
            advance(lex);
            tok.kind = T_AMPERSAND;
            break;
        case '.':
            advance(lex);
            tok.kind = T_DOT;
            break;
        default:
            fprintf(stderr, "Unknown character '%c' at line %d\n", c, lex->line);
            tok.kind = T_EOF;
            advance(lex);
            break;
    }

    return tok;
}

void lexer_free(Lexer *lex)
{
    (void)lex;
}
