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
    STMT_WITH,
    STMT_WITH_PROP_SET,
    STMT_CLASS,
    STMT_PROPERTY_GET,
    STMT_PROPERTY_LET,
} StmtKind;

/* Declaration of a variable or parameter */
typedef struct VarDecl {
    char name[64];
    TypeKind type;
    int array_size; /* -1 = scalar, >= 0 = number of elements */
    int array_size2; /* second dimension: -1 = not 2D, >= 0 = number of elements */
    struct VarDecl *next;
} VarDecl;

typedef struct Stmt Stmt;
typedef struct Proc Proc;
typedef struct ClassDef ClassDef;
typedef struct PropertyDef PropertyDef;

/* A property (getter or setter) within a class */
typedef struct PropertyDef {
    char name[64];
    TypeKind type;
    VarDecl *params;
    Stmt *body;
    int is_let;
    struct PropertyDef *next;
} PropertyDef;

/* A class definition */
typedef struct ClassDef {
    char name[64];
    VarDecl *fields;
    Proc *methods;
    PropertyDef *properties;
    struct ClassDef *next;
} ClassDef;

/* Class info: tracks user-defined types */
typedef struct ClassInfo {
    char name[64];
    VarDecl *fields;
    struct ClassInfo *next;
} ClassInfo;

typedef struct Stmt {
    StmtKind kind;
    Expr  *cond;         /* IF, DO */
    Expr  *expr;         /* DIM, PRINT (sometimes), ASSIGN (right hand) */
    Expr  *index;        /* 1st index for arr(i) = x or arr(i,j) = x */
    Expr  *index2;       /* 2nd index for arr(i,j) = x */
    char   var[64];      /* DIM/ASSIGN target */
    TypeKind var_type;   /* DIM type */
    int    array_size;   /* DIM: -1 scalar, >=0 array upper bound (size = bound+1) */
    int    array_size2;  /* DIM 2nd dim: -1 = not 2D, >=0 2nd dim upper bound */
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

/* Top-level program = main + procedures + globals + classes */
typedef struct {
    Proc *main;
    Proc *first_proc;
    Proc *last_proc;
    ClassDef *first_class;
    ClassDef *last_class;
} Program;

/* Constructors */
Expr *expr_int(long val, int line);
Expr *expr_dbl(double val, int line);
Expr *expr_str(const char *val, int line);
Expr *expr_ident(const char *name, int line);
Expr *expr_binop(Expr *l, const char *op, Expr *r, int line);
Expr *expr_call(const char *name, Expr **args, int nargs, int line);
Expr *expr_array_access(const char *name, Expr *index1, Expr *index2, int line);

Stmt *stmt_assign_index2(const char *v, Expr *idx1, Expr *idx2, Expr *e, int line);

Stmt *stmt_print(Expr *e, int line);
Stmt *stmt_assign(const char *v, Expr *e, int line);
Stmt *stmt_if(Expr *cond, Stmt *then_block, Stmt *else_block, int line);
Stmt *stmt_do_loop(Expr *cond, Stmt *body, int line);
Stmt *stmt_for_next(const char *var, Expr *start, Expr *end, Expr *step_val, Stmt *body, int line);
Stmt *stmt_exit_do(int line);
Stmt *stmt_exit_for(int line);
Stmt *stmt_return(Expr *e, int line);
Stmt *stmt_dim(const char *name, TypeKind type, int line);
Stmt *stmt_dim_arr(const char *name, TypeKind type, int array_size, int array_size2, int line);
Stmt *stmt_block(void);
Stmt *stmt_sub_call(const char *name, Expr **args, int nargs, int line);
Stmt *stmt_append(Stmt *block, Stmt *s);
Stmt *stmt_with_block(const char *with_var, Stmt *body, int line);
Stmt *stmt_with_prop_set(const char *prop_name, Expr *e, const char *with_var, int line);

ClassDef *classdef_new(const char *name);
void classdef_append(ClassDef *cls, void *item, int item_type);
/* item_type: 0 = VarDecl(field), 1 = Proc(method), 2 = PropertyDef(property) */

Proc *proc_new(ProcKind kind, const char *name, TypeKind ret_type, VarDecl *params, Stmt *body);
PropertyDef *property_new(const char *name, TypeKind type, int is_let, VarDecl *params, Stmt *body);

#endif
