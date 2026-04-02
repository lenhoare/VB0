#include "../include/ast.h"
#include <stdlib.h>
#include <string.h>

/* ── Expression constructors ── */

Expr *expr_int(long val, int line)
{
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = EXPR_INT;
    e->type = TYPE_INT;
    e->int_val = val;
    e->line = line;
    return e;
}

Expr *expr_dbl(double val, int line)
{
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = EXPR_DBL;
    e->type = TYPE_DBL;
    e->dbl_val = val;
    e->line = line;
    return e;
}

Expr *expr_str(const char *val, int line)
{
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = EXPR_STR;
    e->type = TYPE_STR;
    strncpy(e->str_val, val, sizeof(e->str_val) - 1);
    e->line = line;
    return e;
}

Expr *expr_ident(const char *name, int line)
{
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = EXPR_IDENT;
    e->type = TYPE_INT; /* default, resolved by codegen */
    strncpy(e->name, name, sizeof(e->name) - 1);
    e->line = line;
    return e;
}

Expr *expr_binop(Expr *l, const char *op, Expr *r, int line)
{
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = EXPR_BINOP;
    e->left = l;
    e->right = r;
    strncpy(e->op, op, sizeof(e->op) - 1);
    e->type = TYPE_INT;
    e->line = line;
    return e;
}

Expr *expr_call(const char *name, Expr **args, int nargs, int line)
{
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = EXPR_CALL;
    e->type = TYPE_INT;
    strncpy(e->name, name, sizeof(e->name) - 1);
    e->arg_count = nargs;
    int i;
    for (i = 0; i < nargs && i < 8; i++)
        e->args[i] = args[i];
    e->line = line;
    return e;
}

Expr *expr_array_access(const char *name, Expr *index1, Expr *index2, int line)
{
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = EXPR_ARRAY_ACCESS;
    e->type = TYPE_INT; /* resolved later by codegen */
    strncpy(e->name, name, sizeof(e->name) - 1);
    e->left = index1;
    e->right = index2;
    e->line = line;
    return e;
}

/* ── Statement constructors ── */

Stmt *stmt_print(Expr *e, int line)
{
    Stmt *s = calloc(1, sizeof(Stmt));
    s->kind = STMT_PRINT;
    s->expr = e;
    s->line = line;
    return s;
}

Stmt *stmt_assign(const char *v, Expr *e, int line)
{
    Stmt *s = calloc(1, sizeof(Stmt));
    s->kind = STMT_ASSIGN;
    strncpy(s->var, v, sizeof(s->var) - 1);
    s->expr = e;
    s->line = line;
    return s;
}

Stmt *stmt_assign_index2(const char *v, Expr *idx1, Expr *idx2, Expr *e, int line)
{
    Stmt *s = calloc(1, sizeof(Stmt));
    s->kind = STMT_ASSIGN;
    strncpy(s->var, v, sizeof(s->var) - 1);
    s->index = idx1;
    s->index2 = idx2;
    s->expr = e;
    s->line = line;
    return s;
}

Stmt *stmt_if(Expr *cond, Stmt *then_block, Stmt *else_block, int line)
{
    Stmt *s = calloc(1, sizeof(Stmt));
    s->kind = STMT_IF;
    s->cond = cond;
    s->block = then_block;
    s->else_block = else_block;
    s->line = line;
    return s;
}

Stmt *stmt_do_loop(Expr *cond, Stmt *body, int line)
{
    Stmt *s = calloc(1, sizeof(Stmt));
    s->kind = STMT_DO_LOOP;
    s->cond = cond;
    s->body = body;
    s->line = line;
    return s;
}

Stmt *stmt_for_next(const char *var, Expr *start, Expr *end, Expr *step_val, Stmt *body, int line)
{
    Stmt *s = calloc(1, sizeof(Stmt));
    s->kind = STMT_FOR_NEXT;
    strncpy(s->loop_var, var, sizeof(s->loop_var) - 1);
    s->loop_start = start;
    s->loop_end = end;
    s->loop_step = step_val;
    s->body = body;
    s->line = line;
    return s;
}

Stmt *stmt_exit_do(int line)
{
    Stmt *s = calloc(1, sizeof(Stmt));
    s->kind = STMT_EXIT_DO;
    s->line = line;
    return s;
}

Stmt *stmt_exit_for(int line)
{
    Stmt *s = calloc(1, sizeof(Stmt));
    s->kind = STMT_EXIT_FOR;
    s->line = line;
    return s;
}

Stmt *stmt_return(Expr *e, int line)
{
    Stmt *s = calloc(1, sizeof(Stmt));
    s->kind = STMT_RETURN;
    s->expr = e;
    s->line = line;
    return s;
}

Stmt *stmt_dim(const char *name, TypeKind type, int line)
{
    Stmt *s = calloc(1, sizeof(Stmt));
    s->kind = STMT_DIM;
    strncpy(s->var, name, sizeof(s->var) - 1);
    s->var_type = type;
    s->line = line;
    return s;
}

Stmt *stmt_dim_arr(const char *name, TypeKind type, int array_size, int array_size2, int line)
{
    Stmt *s = calloc(1, sizeof(Stmt));
    s->kind = STMT_DIM;
    strncpy(s->var, name, sizeof(s->var) - 1);
    s->var_type = type;
    s->array_size = array_size;
    s->array_size2 = array_size2;
    s->line = line;
    return s;
}

Stmt *stmt_block(void)
{
    Stmt *s = calloc(1, sizeof(Stmt));
    s->kind = STMT_BLOCK;
    return s;
}

Stmt *stmt_sub_call(const char *name, Expr **args, int nargs, int line)
{
    Stmt *s = calloc(1, sizeof(Stmt));
    s->kind = STMT_SUB_CALL;
    strncpy(s->var, name, sizeof(s->var) - 1);
    s->call_args = nargs;
    int i;
    for (i = 0; i < nargs && i < 8; i++)
        s->call_arg_list[i] = args[i];
    s->line = line;
    return s;
}

Stmt *stmt_append(Stmt *block, Stmt *s)
{
    if (!block || block->kind != STMT_BLOCK) return s;
    if (!s) return block;
    Stmt *tail = block;
    while (tail->next_stmt) tail = tail->next_stmt;
    tail->next_stmt = s;
    return block;
}

Proc *proc_new(ProcKind kind, const char *name, TypeKind ret_type, VarDecl *params, Stmt *body)
{
    Proc *p = calloc(1, sizeof(Proc));
    p->kind = kind;
    strncpy(p->name, name, sizeof(p->name) - 1);
    p->return_type = ret_type;
    p->params = params;
    p->body = body;
    return p;
}
