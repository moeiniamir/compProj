COMPILER = main
PRODUCTS = main
default: main

SRCS = ast.cpp globals.cpp scopeHandler.cpp codegen.cpp main.cpp
OBJS = y.tab.o lex.yy.o $(patsubst %.cpp, %.o, $(filter %.cpp,$(SRCS))) $(patsubst %.c, %.o, $(filter %.c, $(SRCS)))
JUNK =  *.o lex.yy.c dpp.yy.c y.tab.c y.tab.h *.core core main.purify purify.log

CC= g++
LD = g++
LEX = flex
YACC = bison

.yy.o: $*.yy.c
	g++ -g  -Wall -Wno-unused -Wno-sign-compare -c -o $@ $*.cpp

lex.yy.c: scanner.l  parser.y y.tab.h
	flex -d scanner.l

y.tab.o: y.tab.c
	g++ -g  -Wall -Wno-unused -Wno-sign-compare -c -o y.tab.o y.tab.c

y.tab.h y.tab.c: parser.y
	bison -dvty parser.y
.cpp.o: $*.cpp
	g++ -g  -Wall -Wno-unused -Wno-sign-compare -c -o $@ $*.cpp

main :  $(OBJS)
	g++ -o $@ $(OBJS) -lc -lm

main.purify : $(OBJS)
	purify -log-file=purify.log -cache-dir=/tmp/$(USER) -leaks-at-exit=no g++ -o $@ $(OBJS) -lc -lm

strip : main
	strip main
	rm -rf $(JUNK)

depend:
	makedepend -- -g  -Wall -Wno-unused -Wno-sign-compare -- $(SRCS)

clean:
	rm -f $(JUNK) y.output main

