#ifndef PARSER_H
#define PARSER_H

#include "../include/lexer.h"
#include "../include/ast.h"

typedef struct {
    Lexer lexer;
    Token current;
    int errors;
    /* Track array variable names for disambiguation */
    char array_vars[32][64];
    int array_count;
} Parser;

void   parser_init(Parser *p, char *source);
Program *parser_parse(Parser *p);
void   parser_free(Parser *p);

#endif
