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
    /* Track class type names for WITH block resolution */
    char class_names[16][64];
    int class_count;
    /* Track DIM x AS NEW ClassName: maps var_name -> class_name */
    char class_var_names[64][64];
    char class_var_types[64][64];
    int class_var_count;
} Parser;

void   parser_init(Parser *p, char *source);
Program *parser_parse(Parser *p);
void   parser_free(Parser *p);

#endif
