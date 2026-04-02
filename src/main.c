#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/codegen.h"

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open '%s'\n", path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: vb0 <input.vb0> [-o output.c]\n");
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = "-"; /* stdout */

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        }
    }

    /* Read source */
    char *source = read_file(input_file);

    /* Parse */
    Parser parser;
    parser_init(&parser, source);
    Program *prog = parser_parse(&parser);
    parser_free(&parser);

    if (parser.errors > 0) {
        fprintf(stderr, "Warning: %d error(s) during parsing, continuing with available code\n", parser.errors);
        /* Don't abort - error recovery: compile what we can */
    }

    /* Code generation */
    CodeGen cg;
    codegen_init(&cg);
    codegen_program(&cg, prog);

    /* Write output */
    if (strcmp(output_file, "-") == 0) {
        printf("%s", cg.output);
    } else {
        FILE *f = fopen(output_file, "w");
        if (!f) {
            fprintf(stderr, "Error: cannot write to '%s'\n", output_file);
            codegen_free(&cg);
            free(source);
            return 1;
        }
        fprintf(f, "%s", cg.output);
        fclose(f);
        printf("Compiled %s -> %s (%d bytes)\n", input_file, output_file, cg.len);
    }

    codegen_free(&cg);
    free(source);
    return 0;
}
