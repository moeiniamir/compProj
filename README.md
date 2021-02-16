# decaf-compiler

## Description
This is a compiler project for the Decaf programming language. Decaf is a Java-like object-oriented programming language. The compiler uses Flex and Binson to do lexical and syntax analysis. After building an abstract syntax tree (AST), the compiler performs multiple passes of semantic analysis to support features of the Decaf language. Once all error checking is done, the compiler generates three-address code (TAC) as intermediate representation (IR), and emits MIPS assembly code. The MIPS assembly code can be executed with the SPIM simulator.

## Festures of the Decaf compiler
* Perform lexical and syntax analysis and build AST
* Build a hash-table-based symbol table
* Perform multi-pass semantic analysis on AST including scope analysis and inheritance analysis
* Create TAC as IR
* Create VTable to support dynamically dispatching of virtual methods
* Emit MIPS assembly that can be executed by SPIM simulator

## Usage
Before building the Decaf compiler, please make sure `g++`, `flex` and `bison` commands are avilable. Then we can run `make` in `src` directory to build the Decaf compiler `dcc`. The `dcc` compiler reads a Decaf source file from `stdin` and outputs MIPS assembly to `stdout`.
```
cd src
make
./dcc < ../tests/4_codegen/tictactoe.decaf > tictactoe.asm
```
Under the `src` directory, there is a `run` script to compile a Decaf source file and launch the spim simulator. Please make sure `spim` command is available before executing the script.
```
./run ../tests/4_codegen/tictactoe.decaf
```
The Decaf compiler also supports a debugging option `-d` with arguments such as `ast`, `st` and `tac` to dump abstract syntax tree, symbol table and three-address code. Usage examples are as follows. There are a few other verbose debugging switches including `lex`, `parser`, `ast+`, `sttrace` and `tac+`.
```
./dcc -d ast < ../tests/4_codegen/tictactoe.decaf > debug.txt
./dcc -d ast st tac < ../tests/4_codegen/tictactoe.decaf > debug.txt
```

## Multi-pass Lexical/Syntax/Semantic Analysis, IR Generation and Code Emission
The compiler traverses source code and AST for multiple passes to perform lexical/syntax/semantic analysis, generate IR and emit MIPS assembly. The multi-pass analysis is implementated as `Program::BuildSymTable()`, `Program::Check()` and `Program::Emit()` in `ast_stmt.cc`. 
* Pass 0: Traverse source code and build AST
  * The scanner and parser traverse source code and build the AST
  * The compiler reports all lexical errors and syntax errors in this pass
* Pass 1: Build symbol table
  * The compiler traverses AST and reports declaration conflict errors in local scope
  * With this pass, the compiler supports the feature that all declarations in a scope are immediately visible when entering the scope
* Pass 2: Check identifier declaration
  * The compiler traverses AST and reports errors of undeclared indentifiers based on the symbol table built in previous pass
* Pass 3: Analyze class/interface inheritance
  * The compiler traverses AST and reports inheritance-related errors such as mismatched overriden class members and unimplemented interfaces
* Pass 4: Check all other semantic errors
  * The compiler traverses AST and reports type errors, function argument errors, bread, field access and function calls
  * Until this pass, the compiler can fully understand scopes and inheritance to perform accurate semantic analysis
* Pass 5: Generate TAC as IR
  * The compiler checks the existance of main function with signature `void main() {}`
  * The compiler assigns offsets for class members and global variables, and it adds prefix to all functions and class methods
  * The compiler traverses AST and generate TAC
  * The compiler creates VTables to support dynamic dispatch of class virtual methods
* Pass 6: Emit MIPS assembly based on TAC
  * The compiler emits MIPS assembly that can be executed by the SPIM simulator

## Source Code Structure
* src/Makefile
* src/ast.h, ast.cc
* src/ast_decl.h, ast_decl.cc
* src/ast_expr.h, ast_expr.cc
* src/ast_stmt.h, ast_stmt.cc
* src/ast_type.h, ast_type.cc
* src/codegen.h, codegen.cc
* src/defs.asm
* src/errors.h, errors.cc
* src/hashtable.h, hashtable.cc
* src/list.h
* src/location.h
* src/main.cc
* src/mips.h, mips.cc
* src/parser.h, parser.y
* src/run
* src/scanner.h, scanner.l
* src/symtab.h, symtab.cc
* src/tac.h, tac.cc
* src/trap.handler
* src/utility.h, utility.cc
* tests/1_ast
* tests/2_semantic
* tests/3_semantic
* tests/4_codegen

## References
* The Decaf Language
  > https://parasol.tamu.edu/courses/decaf/students/decafOverview.pdf
* CPTR 415 Compiler Construction - The Decaf Programming Language
  > http://computing.southern.edu/halterman/Courses/Spring2008/415/Assignments/decaf.html
* Abstract Syntax Tree (AST)
  > https://en.wikipedia.org/wiki/Abstract_syntax_tree
* Intermediate representation
  > https://en.wikipedia.org/wiki/Intermediate_representation
* Three-Address Code (TAC)
  > https://en.wikipedia.org/wiki/Three-address_code
* Flex
  > https://en.wikipedia.org/wiki/Flex_(lexical_analyser_generator)
* Bison
  > https://en.wikipedia.org/wiki/GNU_bison
* MIPS architecture
  > https://en.wikipedia.org/wiki/MIPS_architecture
* SPIM simulator - A MIPS processor similator
  > https://en.wikipedia.org/wiki/SPIM
* The MIPS Info Sheet
  > http://www.cs.tufts.edu/comp/140/lectures/Day_3/mips_summary.pdf

