CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
SRCDIR = src
BUILDDIR = build
TARGET = vb0

SRC = $(SRCDIR)/main.c $(SRCDIR)/lexer.c $(SRCDIR)/ast.c $(SRCDIR)/parser.c $(SRCDIR)/codegen.c
OBJ = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRC))

all: $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILDDIR) $(TARGET)

# Convenience: compile and run a .vb0 test
.PHONY: test
test: $(TARGET)
	@echo "=== Running $(TEST) ==="
	./$(TARGET) $(TEST)
	@echo "--- Generated C ---"
	./$(TARGET) $(TEST) -o /tmp/test.c
	@echo "--- Compiling ---"
	gcc -Wall -Wextra -o /tmp/test /tmp/test.c
	@echo "--- Output ---"
	/tmp/test

.PHONY: testall
testall: $(TARGET)
	@for f in tests/*.vb0; do \
		echo "========= $$f ========="; \
		./$(TARGET) "$$f" -o /tmp/test.c && \
		gcc -Wall -Wextra -o /tmp/test /tmp/test.c && \
		/tmp/test; \
		echo ""; \
	done
