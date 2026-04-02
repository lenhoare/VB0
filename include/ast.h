#ifndef AST_H
#define AST_H

#include "lexer.h"

/* Variable type - must come before Expr and Stmt */
typedef enum {
    TYPE_INT,
    TYPE_DBL,
    TYPE_STR,
} TypeKind;

/* Expression kinds */
typedef enum {
    EXPR_INT,
    EXPR_DBL,
    EXPR_STR,
    EXPR_IDENT,
    EXPR_ARRAY_ACCESS,
    EXPR_BINOP,
    EXPR_CALL,
    EXPR_GROUP,
} ExprKind;

typedef struct Expr {
    ExprKind kind;
    TypeKind type;  /* inferred type: TYPE_INT, TYPE_DBL, TYPE_STR */
    struct Expr *left;
    struct Expr *right;
    char  op[4];
    char  name[64];
    char  str_val[256];
    double dbl_val;
    long   int_val;

    /* For function calls */
    int       arg_count;
    struct Expr *args[8];

    int line;
} Expr;

/* Statement kinds */
typedef enum {
    STMT_PRINT,
    STMT_ASSIGN,
    STMT_IF,
    STMT_DO_LOOP,
    STMT_FOR_NEXT,
    STMT_RETURN,
    STMT_DIM,
    STMT_BLOCK,
    STMT_EXIT_DO,
    STMT_EXIT_FOR,
    STMT_SUB_CALL,
} StmtKind;

/* Declaration of a variable or parameter */
typedef struct VarDecl {
    char name[64];
    TypeKind type;
    int array_size; /* -1 = scalar, >= 0 = number of elements */
    struct VarDecl *next;
} VarDecl;

typedef struct Stmt {
    StmtKind kind;
    Expr  *cond;         /* IF, DO */
    Expr  *expr;         /* DIM, PRINT (sometimes), ASSIGN (right hand) */
    Expr  *index;        /* Array index expression for assignment arr(i) = x */
    char   var[64];      /* DIM/ASSIGN target */
    TypeKind var_type;   /* DIM type */
    int    array_size;   /* DIM: -1 scalar, >=0 array upper bound (size = bound+1) */
    struct Stmt *block;  /* IF THEN block */
    struct Stmt *else_block; /* ELSE block */
    struct Stmt *next_stmt;  /* linked list of statements */
    struct Stmt *body;
    struct Stmt *init;   /* FOR init */
    struct Stmt *step;   /* FOR step */

    /* FOR/NEXT */
    char loop_var[64];
    Expr *loop_start;
    Expr *loop_end;
    Expr *loop_step;

    /* Sub/function call */
    int call_args;
    Expr *call_arg_list[8];

    int line;
} Stmt;

/* Procedure kinds */
typedef enum {
    PROC_SUB,
    PROC_FUNCTION,
} ProcKind;

typedef struct Proc {
    ProcKind kind;
    char name[64];
    TypeKind return_type; /* only for FUNCTION */
    VarDecl *params;
    Stmt *body;
    struct Proc *next;
} Proc;

/* Top-level program = main + procedures + globals */
typedef struct {
    Proc *main;
    Proc *first_proc;
    Proc *last_proc;
} Program;

/* Constructors */
Expr *expr_int(long val, int line);
Expr *expr_dbl(double val, int line);
Expr *expr_str(const char *val, int line);
Expr *expr_ident(const char *name, int line);
Expr *expr_binop(Expr *l, const char *op, Expr *r, int line);
Expr *expr_call(const char *name, Expr **args, int nargs, int line);
Expr *expr_array_access(const char *name, Expr *index, int line);

Stmt *stmt_assign_index(const char *v, Expr *idx, Expr *e, int line);

Stmt *stmt_print(Expr *e, int line);
Stmt *stmt_if(Expr *cond, Stmt *then_block, Stmt *else_block, int line);
Stmt *stmt_do_loop(Expr *cond, Stmt *body, int line);
Stmt *stmt_for_next(const char *var, Expr *start, Expr *end, Expr *step_val, Stmt *body, int line);
Stmt *stmt_exit_do(int line);
Stmt *stmt_exit_for(int line);
Stmt *stmt_return(Expr *e, int line);
Stmt *stmt_dim(const char *name, TypeKind type, int line);
Stmt *stmt_dim_arr(const char *name, TypeKind type, int array_size, int line);
Stmt *stmt_block(void);
Stmt *stmt_sub_call(const char *name, Expr **args, int nargs, int line);
Stmt *stmt_append(Stmt *block, Stmt *s);

Proc *proc_new(ProcKind kind, const char *name, TypeKind ret_type, VarDecl *params, Stmt *body);

#endif
