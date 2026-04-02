#include "../include/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void error_at(Parser *p, const char *msg)
{
    fprintf(stderr, "Error at line %d: %s\n", p->current.line, msg);
    p->errors++;
}

static void advance(Parser *p)
{
    p->current = lexer_next(&p->lexer);
}

/* Case-insensitive string compare for keywords */
static int kw(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower(*a) != tolower(*b)) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

static void expect(Parser *p, const char *kwd)
{
    if (!kw(p->current.text, kwd)) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Expected '%s', got '%s'", kwd, p->current.text);
        error_at(p, msg);
        advance(p); /* consume the wrong token to avoid infinite loop */
    } else {
        advance(p);
    }
}

/* ── Forward declarations ── */
static Expr *parse_expr(Parser *p, int precedence);
static Stmt  *parse_statement(Parser *p);

/* ── Expressions (precedence climbing) ── */

static Expr *parse_primary(Parser *p)
{
    Expr *e = NULL;
    if (p->current.kind == T_STR_LIT) {
        e = expr_str(p->current.text, p->current.line);
        advance(p);
    } else if (p->current.kind == T_INT_LIT) {
        e = expr_int(atoi(p->current.text), p->current.line);
        advance(p);
    } else if (p->current.kind == T_DBL_LIT) {
        e = expr_dbl(atof(p->current.text), p->current.line);
        advance(p);
    } else if (p->current.kind == T_IDENT) {
        char name[256];
        int line = p->current.line;
        strncpy(name, p->current.text, sizeof(name) - 1);
        advance(p);

        /* Check for function call */
        if (p->current.kind == T_LPAREN) {
            advance(p); /* skip ( */
            Expr *args[8];
            int nargs = 0;
            while (p->current.kind != T_RPAREN && p->current.kind != T_EOF) {
                if (nargs > 0 && p->current.kind == T_COMMA)
                    advance(p);
                args[nargs++] = parse_expr(p, 0);
            }
            if (p->current.kind == T_RPAREN) advance(p);
            e = expr_call(name, args, nargs, line);
        } else {
            e = expr_ident(name, line);
        }
    } else if (p->current.kind == T_LPAREN) {
        advance(p);
        e = parse_expr(p, 0);
        if (p->current.kind == T_RPAREN) advance(p);
    } else if (kw(p->current.text, "TRUE")) {
        e = expr_int(1, p->current.line);
        advance(p);
    } else if (kw(p->current.text, "FALSE")) {
        e = expr_int(0, p->current.line);
        advance(p);
    } else {
        error_at(p, "Unexpected token in expression");
        e = expr_int(0, p->current.line);
        if (p->current.kind != T_EOF) advance(p);
    }
    return e;
}

/* Precedence levels (lower = binds looser)
 *  0: OR
 *  1: AND
 *  2: NOT (prefix)
 *  3: = <> < <= > >=
 *  4: + -
 *  5: * /
 */
static Expr *parse_expr(Parser *p, int min_prec)
{
    Expr *left;

    /* Unary minus */
    if (p->current.kind == T_MINUS) {
        Token t = p->current;
        advance(p);
        Expr *operand = parse_expr(p, 5);
        left = expr_binop(expr_int(0, t.line), "-", operand, t.line);
    }
    /* NOT prefix */
    else if (p->current.kind == T_NOT) {
        advance(p);
        Expr *operand = parse_expr(p, 2);
        left = expr_binop(operand, "!", NULL, p->current.line);
    }
    else {
        left = parse_primary(p);
    }

    while (1) {
        int prec = -1;
        const char *op = NULL;

        if (p->current.kind == T_PLUS)       { prec = 4; op = "+"; }
        else if (p->current.kind == T_MINUS) { prec = 4; op = "-"; }
        else if (p->current.kind == T_STAR)  { prec = 5; op = "*"; }
        else if (p->current.kind == T_SLASH) { prec = 5; op = "/"; }
        else if (p->current.kind == T_AND)   { prec = 1; op = "&&"; }
        else if (p->current.kind == T_OR)    { prec = 0; op = "||"; }
        else if (p->current.kind == T_AMPERSAND) { prec = 4; op = "&"; }
        else if (p->current.kind == T_ASSIGN){ prec = 3; op = "=="; }
        else if (p->current.kind == T_NEQ)   { prec = 3; op = "!="; }
        else if (p->current.kind == T_LT)    { prec = 3; op = "<"; }
        else if (p->current.kind == T_GT)    { prec = 3; op = ">"; }
        else if (p->current.kind == T_LTE)   { prec = 3; op = "<="; }
        else if (p->current.kind == T_GTE)   { prec = 3; op = ">="; }

        if (prec < 0 || prec < min_prec) break;

        int next_prec = prec + 1; /* left-associative */
        advance(p);
        Expr *right = parse_expr(p, next_prec);
        left = expr_binop(left, op, right, p->current.line);
    }

    return left;
}

/* ── Statements ── */

static Stmt *parse_statement(Parser *p)
{
    int line = p->current.line;

    /* END */
    if (kw(p->current.text, "END")) {
        advance(p);
        return NULL;
    }

    /* DIM */
    if (kw(p->current.text, "DIM")) {
        advance(p);
        char name[256] = {0};
        if (p->current.kind == T_IDENT) {
            strncpy(name, p->current.text, sizeof(name) - 1);
            advance(p);
        }

        TypeKind type = TYPE_INT;
        if (kw(p->current.text, "AS")) {
            advance(p);
            if (kw(p->current.text, "INT") || kw(p->current.text, "INTEGER")) {
                type = TYPE_INT;
            } else if (kw(p->current.text, "DOUBLE")) {
                type = TYPE_DBL;
            } else if (kw(p->current.text, "STRING")) {
                type = TYPE_STR;
            }
            advance(p);
        }
        return stmt_dim(name, type, line);
    }

    /* PRINT */
    if (kw(p->current.text, "PRINT")) {
        advance(p);
        Expr *e = parse_expr(p, 0);
        return stmt_print(e, line);
    }

    /* IF */
    if (kw(p->current.text, "IF")) {
        advance(p);
        Expr *cond = parse_expr(p, 0);

        if (kw(p->current.text, "THEN")) {
            advance(p);
        }

        /* Single-line IF: IF x > 5 THEN PRINT "hi" */
        if (p->current.kind != T_EOF) {
            /* Peek to see if we have a multi-line block */
            /* Multi-line IF: THEN is followed by something and eventually END IF */
            /* We detect single-line by: THEN followed by a statement that is NOT
             * a keyword that starts a block, OR we just try parsing one statement
             * and check if END follows */

            /* Simple detection: if the token after THEN is END, it's IF x THEN END IF
             * (empty body). If it's a keyword like PRINT, DIM, IF, DO, FOR, SUB, FUNCTION -
             * parse as block. Otherwise single-line. */
            int is_statement_kw =
                kw(p->current.text, "PRINT") ||
                kw(p->current.text, "DIM") ||
                kw(p->current.text, "IF") ||
                kw(p->current.text, "DO") ||
                kw(p->current.text, "FOR") ||
                kw(p->current.text, "RETURN") ||
                kw(p->current.text, "END") ||
                kw(p->current.text, "EXIT") ||
                (p->current.kind == T_IDENT);  /* could be assignment or sub call */

            if (is_statement_kw) {
                /* Multi-line block */
                Stmt *then_block = stmt_block();
                while (p->current.kind != T_EOF &&
                       !kw(p->current.text, "ELSE") &&
                       !kw(p->current.text, "END")) {
                    Stmt *s = parse_statement(p);
                    if (s) stmt_append(then_block, s);
                }

                Stmt *else_block = NULL;
                if (kw(p->current.text, "ELSE")) {
                    advance(p);
                    else_block = stmt_block();
                    while (p->current.kind != T_EOF &&
                           !kw(p->current.text, "END")) {
                        Stmt *s = parse_statement(p);
                        if (s) stmt_append(else_block, s);
                    }
                }

                expect(p, "END");
                if (kw(p->current.text, "IF")) advance(p);

                return stmt_if(cond, then_block, else_block, line);
            } else {
                /* Single-line IF */
                Stmt *then_stmt = parse_statement(p);
                return stmt_if(cond, then_stmt, NULL, line);
            }
        } else {
            error_at(p, "Expected statement after THEN");
            return stmt_if(cond, NULL, NULL, line);
        }
    }

    /* DO / DO WHILE / DO UNTIL */
    if (kw(p->current.text, "DO")) {
        advance(p);
        Expr *cond = NULL;

        if (kw(p->current.text, "WHILE")) {
            advance(p);
            cond = parse_expr(p, 0);
        } else if (kw(p->current.text, "UNTIL")) {
            advance(p);
            Expr *inner = parse_expr(p, 0);
            cond = expr_binop(inner, "!", NULL, inner->line);
        }

        Stmt *body = stmt_block();
        while (p->current.kind != T_EOF && !kw(p->current.text, "LOOP")) {
            Stmt *s = parse_statement(p);
            if (s) stmt_append(body, s);
        }

        expect(p, "LOOP");

        if (cond == NULL) {
            if (kw(p->current.text, "WHILE")) {
                advance(p);
                cond = parse_expr(p, 0);
            } else if (kw(p->current.text, "UNTIL")) {
                advance(p);
                Expr *inner = parse_expr(p, 0);
                cond = expr_binop(inner, "!", NULL, inner->line);
            } else {
                cond = expr_int(1, line);
            }
        }

        return stmt_do_loop(cond, body, line);
    }

    /* FOR */
    if (kw(p->current.text, "FOR")) {
        advance(p);
        char var[256] = {0};
        if (p->current.kind == T_IDENT) {
            strncpy(var, p->current.text, sizeof(var) - 1);
            advance(p);
        }

        if (p->current.kind != T_ASSIGN) {
            error_at(p, "Expected '=' after FOR variable");
        } else {
            advance(p);
        }
        Expr *start = parse_expr(p, 0);
        if (!kw(p->current.text, "TO")) {
            error_at(p, "Expected 'TO' in FOR loop");
        } else {
            advance(p);
        }
        Expr *en = parse_expr(p, 0);

        Expr *step_val = expr_int(1, p->current.line);
        if (kw(p->current.text, "STEP")) {
            advance(p);
            step_val = parse_expr(p, 0);
        }

        Stmt *body = stmt_block();
        while (p->current.kind != T_EOF && !kw(p->current.text, "NEXT")) {
            Stmt *s = parse_statement(p);
            if (s) stmt_append(body, s);
        }
        expect(p, "NEXT");

        return stmt_for_next(var, start, en, step_val, body, line);
    }

    /* EXIT DO / EXIT FOR */
    if (kw(p->current.text, "EXIT")) {
        advance(p);
        if (kw(p->current.text, "DO")) {
            advance(p);
            return stmt_exit_do(line);
        }
        if (kw(p->current.text, "FOR")) {
            advance(p);
            return stmt_exit_for(line);
        }
        error_at(p, "Expected DO or FOR after EXIT");
        return stmt_exit_do(line);
    }

    /* RETURN */
    if (kw(p->current.text, "RETURN")) {
        advance(p);
        Expr *e = NULL;
        if (p->current.kind != T_EOF)
            e = parse_expr(p, 0);
        return stmt_return(e, line);
    }

    /* Assignment: var = expr  OR  Sub call: name(args) */
    if (p->current.kind == T_IDENT) {
        char name[256] = {0};
        int name_line = p->current.line;
        /* Save name without advancing */
        strncpy(name, p->current.text, sizeof(name) - 1);

        /* Peek ahead: check next token */
        Token saved = p->current;
        advance(p);

        if (p->current.kind == T_ASSIGN) {
            /* It's an assignment: var = expr */
            advance(p);
            Expr *e = parse_expr(p, 0);
            return stmt_assign(name, e, line);
        } else if (p->current.kind == T_LPAREN) {
            /* It's a sub/function call: name(args) */
            advance(p);
            Expr *args[8];
            int nargs = 0;
            while (p->current.kind != T_RPAREN && p->current.kind != T_EOF) {
                if (nargs > 0 && p->current.kind == T_COMMA)
                    advance(p);
                args[nargs++] = parse_expr(p, 0);
            }
            if (p->current.kind == T_RPAREN) advance(p);
            /* If used as a statement (not in expression context), treat as sub call */
            return stmt_sub_call(name, args, nargs, name_line);
        } else {
            /* Identifier not followed by = or ( - it's an unknown statement */
            /* The saved token is the bad identifier itself. We've already
               advanced past it (p->current is the next token), just report
               the error and let the caller's loop continue. The bad token
               (THISWILLFAIL in the test) has been consumed. */
            error_at(p, "Unknown statement or undeclared keyword");
            return NULL;
        }    }

    error_at(p, "Unknown statement");
    if (p->current.kind != T_EOF) advance(p);
    return NULL;
}

/* ── Procedure parsing ── */

static VarDecl *parse_param_list(Parser *p)
{
    VarDecl *first = NULL;
    VarDecl **next_ptr = &first;

    while (p->current.kind != T_EOF &&
           !kw(p->current.text, ")") &&
           p->current.kind != T_RPAREN) {

        if (first != NULL && p->current.kind == T_COMMA)
            advance(p);
        if (p->current.kind == T_EOF || p->current.kind == T_RPAREN ||
            kw(p->current.text, ")"))
            break;

        char name[256] = {0};
        if (p->current.kind == T_IDENT) {
            strncpy(name, p->current.text, sizeof(name) - 1);
            advance(p);
        }

        TypeKind type = TYPE_INT;
        if (kw(p->current.text, "AS")) {
            advance(p);
            if (kw(p->current.text, "INT") || kw(p->current.text, "INTEGER"))
                type = TYPE_INT;
            else if (kw(p->current.text, "DOUBLE"))
                type = TYPE_DBL;
            else if (kw(p->current.text, "STRING"))
                type = TYPE_STR;
            advance(p);
        }

        VarDecl *v = calloc(1, sizeof(VarDecl));
        strncpy(v->name, name, sizeof(v->name) - 1);
        v->type = type;
        *next_ptr = v;
        next_ptr = &v->next;
    }

    return first;
}

static Proc *parse_proc(Parser *p)
{
    ProcKind kind;

    if (kw(p->current.text, "SUB")) {
        kind = PROC_SUB;
    } else if (kw(p->current.text, "FUNCTION")) {
        kind = PROC_FUNCTION;
    } else {
        return NULL;
    }

    advance(p);

    char name[256] = {0};
    if (p->current.kind == T_IDENT) {
        strncpy(name, p->current.text, sizeof(name) - 1);
        advance(p);
    }

    VarDecl *params = NULL;
    if (p->current.kind == T_LPAREN) {
        advance(p);
        params = parse_param_list(p);
        if (p->current.kind == T_RPAREN || kw(p->current.text, ")")) advance(p);
    }

    TypeKind ret_type = TYPE_INT;
    if (kw(p->current.text, "AS")) {
        advance(p);
        if (kw(p->current.text, "INT") || kw(p->current.text, "INTEGER"))
            ret_type = TYPE_INT;
        else if (kw(p->current.text, "DOUBLE"))
            ret_type = TYPE_DBL;
        else if (kw(p->current.text, "STRING"))
            ret_type = TYPE_STR;
        advance(p);
    }

    Stmt *body = stmt_block();
    while (p->current.kind != T_EOF) {
        if (kw(p->current.text, "END")) {
            advance(p);
            if (p->current.kind != T_EOF) {
                /* Skip SUB/FUNCTION after END */
                if (p->current.kind == T_IDENT) advance(p);
            }
            break;
        }
        Stmt *s = parse_statement(p);
        if (s) stmt_append(body, s);
    }

    return proc_new(kind, name, ret_type, params, body);
}

/* ── Main parse entry ── */

void parser_init(Parser *p, char *source)
{
    memset(p, 0, sizeof(*p));
    lexer_init(&p->lexer, source);
    p->current = lexer_next(&p->lexer);
}

Program *parser_parse(Parser *p)
{
    Program *prog = calloc(1, sizeof(Program));
    Stmt *block = stmt_block();

    while (p->current.kind != T_EOF) {
        /* Check for SUB or FUNCTION definition */
        if (kw(p->current.text, "SUB") || kw(p->current.text, "FUNCTION")) {
            Proc *proc = parse_proc(p);
            if (proc) {
                if (!prog->first_proc)
                    prog->first_proc = proc;
                else
                    prog->last_proc->next = proc;
                prog->last_proc = proc;
                continue;
            }
        }

        /* Top-level statement */
        Stmt *s = parse_statement(p);
        if (s)
            stmt_append(block, s);
    }

    /* Create implicit main from top-level statements */
    Proc *main_proc = proc_new(PROC_SUB, "Main", TYPE_INT, NULL, block);
    prog->main = main_proc;

    return prog;
}

void parser_free(Parser *p)
{
    lexer_free(&p->lexer);
}
