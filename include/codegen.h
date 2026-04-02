#ifndef CODEGEN_H
#define CODEGEN_H

#include "../include/ast.h"

typedef struct {
    char *output;
    int   len;
    int   cap;
    int   indent;
} CodeGen;

void  codegen_init(CodeGen *cg);
void  codegen_emit(CodeGen *cg, const char *fmt, ...);
void  codegen_free(CodeGen *cg);

/* Top-level generation */
void  codegen_program(CodeGen *cg, Program *prog);
void  codegen_proc(CodeGen *cg, Proc *proc);
void  codegen_statement(CodeGen *cg, Stmt *s);
void  codegen_expr(CodeGen *cg, Expr *e);

#endif
