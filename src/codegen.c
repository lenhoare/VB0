#include "../include/codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define BUF_INITIAL 8192

typedef struct VarInfo {
    char name[64];
    TypeKind type;
    int array_size;  /* -1 = scalar, >=0 number of elements */
    int array_size2; /* -1 = not 2D, >=0 2nd dim size */
    struct VarInfo *next;
} VarInfo;

typedef struct CodeGen {
    char *output;
    int   len;
    int   cap;
    int   indent;
    /* Symbol tables for type lookup */
    VarInfo *vars;
    Proc *procs;  /* all procs for func return type lookup */
    char *loop_break_label; /* for EXIT DO / EXIT FOR */
    char *loop_continue_label;
    int label_counter;
} _CodeGen;

static void cg_emit_raw(CodeGen *cg, const char *fmt, ...);

static int next_label(CodeGen *cg)
{
    _CodeGen *c = (_CodeGen *)cg;
    return ++c->label_counter;
}

void codegen_init(CodeGen *cg)
{
    cg->output = malloc(BUF_INITIAL);
    cg->output[0] = '\0';
    cg->len = 0;
    cg->cap = BUF_INITIAL;
    cg->indent = 0;
    /* Cast to internal struct for extra fields */
    _CodeGen *ic = (_CodeGen *)cg;
    ic->vars = NULL;
    ic->procs = NULL;
    ic->loop_break_label = NULL;
    ic->loop_continue_label = NULL;
    ic->label_counter = 0;
}

static void cg_emit(CodeGen *cg, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    while (cg->len + needed + 3 > cg->cap) {
        cg->cap *= 2;
        cg->output = realloc(cg->output, cg->cap);
    }
    /* Indent */
    for (int i = 0; i < cg->indent; i++) {
        cg->output[cg->len++] = ' ';
        cg->output[cg->len++] = ' ';
    }
    va_start(args, fmt);
    cg->len += vsprintf(cg->output + cg->len, fmt, args);
    va_end(args);
}

static void cg_emit_raw(CodeGen *cg, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    while (cg->len + needed + 1 > cg->cap) {
        cg->cap *= 2;
        cg->output = realloc(cg->output, cg->cap);
    }
    va_start(args, fmt);
    cg->len += vsprintf(cg->output + cg->len, fmt, args);
    va_end(args);
}

void codegen_emit(CodeGen *cg, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    while (cg->len + needed + 3 > cg->cap) {
        cg->cap *= 2;
        cg->output = realloc(cg->output, cg->cap);
    }
    for (int i = 0; i < cg->indent; i++) {
        cg->output[cg->len++] = ' ';
        cg->output[cg->len++] = ' ';
    }
    va_start(args, fmt);
    cg->len += vsprintf(cg->output + cg->len, fmt, args);
    va_end(args);
}

void codegen_free(CodeGen *cg)
{
    free(cg->output);
}

/* ── Symbol table helpers ── */

static void add_var(CodeGen *cg, const char *name, TypeKind type, int array_size, int array_size2)
{
    _CodeGen *ic = (_CodeGen *)cg;
    /* Don't add duplicate */
    VarInfo *v = ic->vars;
    while (v) {
        if (strcmp(v->name, name) == 0) { return; }
        v = v->next;
    }
    VarInfo *ni = calloc(1, sizeof(VarInfo));
    strncpy(ni->name, name, sizeof(ni->name) - 1);
    ni->type = type;
    ni->array_size = array_size;
    ni->array_size2 = array_size2;
    ni->next = ic->vars;
    ic->vars = ni;
}

static TypeKind lookup_var_type(CodeGen *cg, const char *name)
{
    _CodeGen *ic = (_CodeGen *)cg;
    VarInfo *v = ic->vars;
    while (v) {
        if (strcmp(v->name, name) == 0) return v->type;
        v = v->next;
    }
    return TYPE_INT;
}

static TypeKind lookup_func_return(CodeGen *cg, const char *name)
{
    _CodeGen *ic = (_CodeGen *)cg;
    Proc *p = ic->procs;
    while (p) {
        if (strcmp(p->name, name) == 0 && p->kind == PROC_FUNCTION)
            return p->return_type;
        p = p->next;
    }
    return TYPE_INT;
}

static const char *c_type(TypeKind t)
{
    switch (t) {
        case TYPE_INT: return "int";
        case TYPE_DBL: return "double";
        case TYPE_STR: return "char*";
    }
    return "int";
}

/* ── Expression to C ── */

/* Forward declare */
static char *expr_to_c(CodeGen *cg, Expr *e);

static char *escape_string(const char *s)
{
    int len = strlen(s);
    char *out = malloc(len * 2 + 3);
    out[0] = '"';
    int j = 1;
    for (int i = 0; i < len; i++) {
        switch (s[i]) {
            case '\\': out[j++] = '\\'; out[j++] = '\\'; break;
            case '"':  out[j++] = '\\'; out[j++] = '"'; break;
            case '\n': out[j++] = '\\'; out[j++] = 'n'; break;
            case '\t': out[j++] = '\\'; out[j++] = 't'; break;
            default:   out[j++] = s[i]; break;
        }
    }
    out[j++] = '"';
    out[j] = '\0';
    return out;
}

static TypeKind expr_type(CodeGen *cg, Expr *e)
{
    if (!e) return TYPE_INT;
    switch (e->kind) {
        case EXPR_INT:  return TYPE_INT;
        case EXPR_DBL:  return TYPE_DBL;
        case EXPR_STR:  return TYPE_STR;
        case EXPR_IDENT:return lookup_var_type(cg, e->name);
        case EXPR_CALL: return lookup_func_return(cg, e->name);
        case EXPR_ARRAY_ACCESS: return lookup_var_type(cg, e->name);
        case EXPR_BINOP: {
            if (e->op[0] == '&' && e->op[1] == '\0')
                return TYPE_STR;
            if (e->op[0] == '+' && e->op[1] == '\0') {
                TypeKind lt = expr_type(cg, e->left);
                TypeKind rt = expr_type(cg, e->right);
                if (lt == TYPE_STR || rt == TYPE_STR) return TYPE_STR;
                if (lt == TYPE_DBL || rt == TYPE_DBL) return TYPE_DBL;
            } else {
                /* For other binary ops: -, *, /, comparisons */
                TypeKind lt = expr_type(cg, e->left);
                TypeKind rt = expr_type(cg, e->right);
                if (lt == TYPE_DBL || rt == TYPE_DBL) return TYPE_DBL;
            }
            return TYPE_INT;
        }
    }
    return TYPE_INT;
}

static char *promote_to_double(CodeGen *cg, Expr *e)
{
    char *c = expr_to_c(cg, e);
    TypeKind t = expr_type(cg, e);
    char *result;
    if (t == TYPE_INT) {
        result = malloc(strlen(c) + 20);
        sprintf(result, "(double)(%s)", c);
    } else {
        result = malloc(strlen(c) + 1);
        strcpy(result, c);
    }
    free(c);
    return result;
}

static char *expr_to_c(CodeGen *cg, Expr *e)
{
    if (!e) return strdup("0");

    char buf[2048];

    switch (e->kind) {
        case EXPR_INT:
            snprintf(buf, sizeof(buf), "%ld", e->int_val);
            break;
        case EXPR_DBL:
            snprintf(buf, sizeof(buf), "%g", e->dbl_val);
            break;
        case EXPR_STR: {
            char *escaped = escape_string(e->str_val);
            snprintf(buf, sizeof(buf), "%s", escaped);
            free(escaped);
            break;
        }
        case EXPR_IDENT:
            snprintf(buf, sizeof(buf), "%s", e->name);
            break;
        case EXPR_BINOP: {
            if (e->op[0] == '&' && e->op[1] == '\0') {
                /* String concatenation */
                char *lc = expr_to_c(cg, e->left);
                char *rc = expr_to_c(cg, e->right);
                snprintf(buf, sizeof(buf), "_vb0_strcat(%s, %s)", lc, rc);
                free(lc);
                free(rc);
            } else if (e->right) {
                char *lc = expr_to_c(cg, e->left);
                char *rc = expr_to_c(cg, e->right);
                snprintf(buf, sizeof(buf), "(%s %s %s)", lc, e->op, rc);
                free(lc);
                free(rc);
            } else {
                char *lc = expr_to_c(cg, e->left);
                snprintf(buf, sizeof(buf), "(!%s)", lc);
                free(lc);
            }
            break;
        }
        case EXPR_ARRAY_ACCESS: {
            char *i1 = expr_to_c(cg, e->left);
            if (e->right) {
                /* 2D: arr[i][j] */
                char *i2 = expr_to_c(cg, e->right);
                snprintf(buf, sizeof(buf), "%s[%s][%s]", e->name, i1, i2);
                free(i2);
            } else {
                /* 1D: arr[i] */
                snprintf(buf, sizeof(buf), "%s[%s]", e->name, i1);
            }
            free(i1);
            break;
        }
        case EXPR_CALL: {
            char args[1024] = {0};
            int offset = 0;
            for (int i = 0; i < e->arg_count; i++) {
                char *ac = expr_to_c(cg, e->args[i]);
                offset += snprintf(args + offset, sizeof(args) - offset,
                                   "%s%s", i > 0 ? ", " : "", ac);
                free(ac);
            }
            snprintf(buf, sizeof(buf), "%s(%s)", e->name, args);
            break;
        }
        default:
            snprintf(buf, sizeof(buf), "0");
            break;
    }

    return strdup(buf);
}

void codegen_expr(CodeGen *cg, Expr *e)
{
    char *c = expr_to_c(cg, e);
    codegen_emit(cg, "%s", c);
    free(c);
}

/* ── Statement code generation ── */

static void codegen_body(CodeGen *cg, Stmt *s)
{
    if (!s || s->kind != STMT_BLOCK) return;
    s = s->next_stmt;
    while (s) {
        codegen_statement(cg, s);
        s = s->next_stmt;
    }
}

void codegen_statement(CodeGen *cg, Stmt *s)
{
    if (!s) return;

    switch (s->kind) {
        case STMT_BLOCK:
            codegen_body(cg, s);
            break;

        case STMT_PRINT: {
            TypeKind t = expr_type(cg, s->expr);
            if (t == TYPE_STR) {
                codegen_emit(cg, "printf(\"%%s\\n\", ");
                codegen_expr(cg, s->expr);
                codegen_emit(cg, ");\n");
            } else if (t == TYPE_DBL) {
                codegen_emit(cg, "printf(\"%%g\\n\", ");
                codegen_expr(cg, s->expr);
                codegen_emit(cg, ");\n");
            } else {
                codegen_emit(cg, "printf(\"%%d\\n\", ");
                codegen_expr(cg, s->expr);
                codegen_emit(cg, ");\n");
            }
            break;
        }

        case STMT_ASSIGN:
            if (s->index2) {
                /* 2D: arr[i][j] = val */
                char *i1 = expr_to_c(cg, s->index);
                char *i2 = expr_to_c(cg, s->index2);
                codegen_emit(cg, "%s[%s][%s] = ", s->var, i1, i2);
                free(i1);
                free(i2);
            } else if (s->index) {
                /* 1D: arr[i] = val */
                char *idx_c = expr_to_c(cg, s->index);
                codegen_emit(cg, "%s[%s] = ", s->var, idx_c);
                free(idx_c);
            } else {
                codegen_emit(cg, "%s = ", s->var);
            }
            codegen_expr(cg, s->expr);
            codegen_emit(cg, ";\n");
            break;

        case STMT_DIM:
            add_var(cg, s->var, s->var_type, s->array_size, s->array_size2);
            if (s->array_size > 0 && s->array_size2 > 0) {
                /* 2D array: type arr[rows][cols] */
                codegen_emit(cg, "%s %s[%d][%d];\n", c_type(s->var_type), s->var, s->array_size, s->array_size2);
                if (s->var_type == TYPE_STR) {
                    for (int i = 0; i < s->array_size; i++)
                        for (int j = 0; j < s->array_size2; j++)
                            codegen_emit(cg, "%s[%d][%d] = NULL;\n", s->var, i, j);
                }
            } else if (s->array_size > 0) {
                /* 1D array */
                codegen_emit(cg, "%s %s[%d];\n", c_type(s->var_type), s->var, s->array_size);
                if (s->var_type == TYPE_STR) {
                    for (int i = 0; i < s->array_size; i++)
                        codegen_emit(cg, "%s[%d] = NULL;\n", s->var, i);
                }
            } else if (s->var_type == TYPE_STR) {
                codegen_emit(cg, "%s %s = NULL;\n", c_type(s->var_type), s->var);
            } else {
                codegen_emit(cg, "%s %s = 0;\n", c_type(s->var_type), s->var);
            }
            break;

        case STMT_IF: {
            codegen_emit(cg, "if (");
            codegen_expr(cg, s->cond);
            codegen_emit(cg, ") {\n");
            cg->indent++;
            codegen_body(cg, s->block);
            cg->indent--;
            codegen_emit(cg, "}\n");
            if (s->else_block) {
                codegen_emit(cg, "else {\n");
                cg->indent++;
                codegen_body(cg, s->else_block);
                cg->indent--;
                codegen_emit(cg, "}\n");
            }
            break;
        }

        case STMT_DO_LOOP: {
            int lbl = next_label(cg);
            char *brk = malloc(32);
            char *cont = malloc(32);
            snprintf(brk, 32, "_loop_break_%d", lbl);
            snprintf(cont, 32, "_loop_cont_%d", lbl);

            _CodeGen *ic = (_CodeGen *)cg;
            char *old_brk = ic->loop_break_label;
            char *old_cont = ic->loop_continue_label;
            ic->loop_break_label = brk;
            ic->loop_continue_label = cont;

            /* Check if condition is always true (infinite loop) */
            if (s->cond && s->cond->kind == EXPR_INT && s->cond->int_val == 1) {
                codegen_emit(cg, "while (1) {\n");
            } else {
                codegen_emit(cg, "while (");
                codegen_expr(cg, s->cond);
                codegen_emit(cg, ") {\n");
            }
            cg->indent++;
            codegen_body(cg, s->body);
            cg->indent--;
            codegen_emit(cg, "}\n");
            codegen_emit(cg, "%s: ;\n", brk);

            free(brk);
            free(cont);
            ic->loop_break_label = old_brk;
            ic->loop_continue_label = old_cont;
            break;
        }

        case STMT_FOR_NEXT: {
            codegen_emit(cg, "{\n");
            cg->indent++;
            char *start_c = expr_to_c(cg, s->loop_start);
            char *end_c = expr_to_c(cg, s->loop_end);
            char *step_c = expr_to_c(cg, s->loop_step);

            /* Determine if step is negative to pick correct comparison */
            int negative_step = 0;
            /* Check if step is a literal negative number */
            if (s->loop_step->kind == EXPR_INT && s->loop_step->int_val < 0)
                negative_step = 1;
            if (s->loop_step->kind == EXPR_DBL && s->loop_step->dbl_val < 0)
                negative_step = 1;
            /* Also check if it's unary minus: -N represented as (0 - N) */
            if (s->loop_step->kind == EXPR_BINOP &&
                strcmp(s->loop_step->op, "-") == 0 &&
                s->loop_step->left->kind == EXPR_INT &&
                s->loop_step->left->int_val == 0)
                negative_step = 1;

            codegen_emit(cg, "double _i;\n");
            if (negative_step) {
                codegen_emit(cg, "for (_i = %s; _i >= %s; _i += %s) {\n",
                             start_c, end_c, step_c);
            } else {
                codegen_emit(cg, "for (_i = %s; _i <= %s; _i += %s) {\n",
                             start_c, end_c, step_c);
            }
            cg->indent++;
            codegen_emit(cg, "%s = (int)_i;\n", s->loop_var);
            codegen_body(cg, s->body);
            cg->indent--;
            codegen_emit(cg, "}\n");
            cg->indent--;
            codegen_emit(cg, "}\n");
            free(start_c);
            free(end_c);
            free(step_c);
            break;
        }

        case STMT_EXIT_DO:
        case STMT_EXIT_FOR: {
            _CodeGen *ic = (_CodeGen *)cg;
            if (ic->loop_break_label)
                codegen_emit(cg, "goto %s;\n", ic->loop_break_label);
            else
                codegen_emit(cg, "break;\n");
            break;
        }

        case STMT_RETURN:
            if (s->expr) {
                codegen_emit(cg, "return ");
                codegen_expr(cg, s->expr);
                codegen_emit(cg, ";\n");
            } else {
                codegen_emit(cg, "return;\n");
            }
            break;

        case STMT_SUB_CALL:
            codegen_emit(cg, "%s(", s->var);
            for (int i = 0; i < s->call_args; i++) {
                if (i > 0) codegen_emit(cg, ", ");
                codegen_expr(cg, s->call_arg_list[i]);
            }
            codegen_emit(cg, ");\n");
            break;

        default:
            fprintf(stderr, "Warning: unknown statement kind %d at line %d\n",
                    s->kind, s->line);
            break;
    }
}

/* ── Procedure code generation ── */

static void gen_param_types(CodeGen *cg, VarDecl *params)
{
    VarDecl *v = params;
    while (v) {
        add_var(cg, v->name, v->type, -1, -1);
        v = v->next;
    }
}

void codegen_proc(CodeGen *cg, Proc *proc)
{
    _CodeGen *ic = (_CodeGen *)cg;
    ic->procs = ic->procs; /* already set by codegen_program */

    if (proc->kind == PROC_FUNCTION) {
        cg_emit(cg, "%s %s(", c_type(proc->return_type), proc->name);
    } else {
        cg_emit(cg, "void %s(", proc->name);
    }

    VarDecl *v = proc->params;
    int first = 1;
    while (v) {
        if (!first) cg_emit(cg, ", ");
        cg_emit(cg, "%s %s", c_type(v->type), v->name);
        first = 0;
        v = v->next;
    }
    cg_emit(cg, ") {\n");

    /* Add params to symbol table */
    v = proc->params;
    while (v) {
        add_var(cg, v->name, v->type, -1, -1);
        v = v->next;
    }

    cg->indent++;
    codegen_body(cg, proc->body);
    cg->indent--;

    if (proc->kind == PROC_FUNCTION) {
        cg_emit(cg, "    return 0;\n");
    }
    cg_emit(cg, "}\n\n");
}

/* ── Program code generation ── */

void codegen_program(CodeGen *cg, Program *prog)
{
    _CodeGen *ic = (_CodeGen *)cg;

    /* Header + runtime helpers */
    cg_emit_raw(cg, "#include <stdio.h>\n");
    cg_emit_raw(cg, "#include <stdlib.h>\n");
    cg_emit_raw(cg, "#include <string.h>\n\n");

    /* Runtime: string concatenation helper */
    cg_emit_raw(cg,
        "static char* _vb0_strcat(char* a, char* b) {\n"
        "    if (!a) a = \"\";\n"
        "    if (!b) b = \"\";\n"
        "    /* Convert int to string if needed */\n"
        "    int len_a = strlen(a);\n"
        "    int len_b = strlen(b);\n"
        "    char *r = malloc(len_a + len_b + 1);\n"
        "    strcpy(r, a);\n"
        "    strcat(r, b);\n"
        "    return r;\n"
        "}\n\n"
    );

    /* Forward declarations for all procs */
    Proc *p = prog->first_proc;
    while (p) {
        if (p->kind == PROC_FUNCTION) {
            cg_emit_raw(cg, "%s %s(", c_type(p->return_type), p->name);
        } else {
            cg_emit_raw(cg, "void %s(", p->name);
        }
        VarDecl *v = p->params;
        int first = 1;
        while (v) {
            if (!first) cg_emit_raw(cg, ", ");
            cg_emit_raw(cg, "%s %s", c_type(v->type), v->name);
            first = 0;
            v = v->next;
        }
        cg_emit_raw(cg, ");\n");
        cg->indent = 0;
        p = p->next;
    }
    cg_emit_raw(cg, "\n");

    /* Save all procs for type lookup */
    ic->procs = prog->first_proc;

    /* Emit all auxiliary procedures */
    p = prog->first_proc;
    while (p) {
        codegen_proc(cg, p);
        p = p->next;
    }

    /* Emit main */
    if (prog->main) {
        cg_emit_raw(cg, "int main(void) {\n");
        cg->indent++;
        codegen_body(cg, prog->main->body);
        cg->indent--;
        cg_emit_raw(cg, "    return 0;\n}\n");
    }
}
