CC=gcc
CFLAGS= -Wall -ggdb
COMPILE= $(CC) $(CFLAGS)
OBJDIR= ../.objective_files
ALL= $(OBJDIR)/ast.o $(OBJDIR)/code_generator.o $(OBJDIR)/dicc.o $(OBJDIR)/lex.o \
	$(OBJDIR)/parser.o $(OBJDIR)/runtime_table.o $(OBJDIR)/stack.o $(OBJDIR)/utils.o

$(VERBOSE).SILENT: $(ALL) all

$(shell mkdir -p $(OBJDIR))

all: $(ALL)
	$(COMPILE) $(OBJDIR)/* -o ../dicc

$(OBJDIR)/ast.o: ast.c ast.h types.h
	$(COMPILE) -c ast.c -o $@

$(OBJDIR)/code_generator.o: code_generator.c code_generator.h \
								types.h utils.h runtime_table.h
	$(COMPILE) -c code_generator.c -o $@

$(OBJDIR)/dicc.o: dicc.c types.h utils.h lex.h parser.h ast.h code_generator.h 
	$(COMPILE) -c dicc.c -o $@

$(OBJDIR)/lex.o: lex.c lex.h types.h utils.h parser.h ast.h code_generator.h
	$(COMPILE) -c lex.c -o $@

$(OBJDIR)/parser.o: parser.c parser.h utils.h types.h ast.h
	$(COMPILE) -c parser.c -o $@

$(OBJDIR)/runtime_table.o: runtime_table.c runtime_table.h types.h
	$(COMPILE) -c runtime_table.c -o $@

$(OBJDIR)/stack.o: stack.c stack.h types.h
	$(COMPILE) -c stack.c -o $@

$(OBJDIR)/utils.o: utils.c utils.h
	$(COMPILE) -c utils.c -o $@

.PHONY: all clean

clean:
	rm -rf $(OBJDIR)
