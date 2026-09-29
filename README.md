# A compiler for a C-like language, targeting NASM assembly

A compiler written from scratch in C++17, translating a small C-like language into
x86-64 assembly. It covers the whole pipeline — lexical analysis, parsing, abstract
syntax tree, intermediate representation, optimisation and code generation — with no
compiler framework underneath: only Flex for the scanner and Bison for the parser.

Semester project for the *Formal Approaches, Programming Languages and Compilers*
course at ISEP — École d'ingénieurs du numérique, Paris, autumn 2025. Individual work.

## What the language looks like

```c
int x;
x = 5;

int y;
y = 10;

prints("Hello world!");
print(x);
print(y);
```

The grammar covers integer declarations and assignments, arithmetic, comparison,
logical, bitwise and shift operators, increment and decrement, conditions, and the two
output statements `print` (integers) and `prints` (strings). The full EBNF grammar is in
[`docs/grammar.txt`](docs/grammar.txt).

Printing is not delegated to libc: the generated assembly carries its own
`_print_num` and `_print_string` routines, which convert an integer to its decimal
representation digit by digit and write to stdout through a syscall.

## The five phases

The repository keeps the phases separate, because the point of the project was the
progression rather than the final artefact alone. Each directory builds and runs on its
own.

| Directory | What it adds |
|---|---|
| [`1-scanner/`](1-scanner) | Flex scanner: source text to a token stream |
| [`2-parser-ast/`](2-parser-ast) | hand-written recursive-descent parser, building an AST |
| [`3-bison-parser-ir/`](3-bison-parser-ir) | the parser rewritten in Bison, plus a three-address intermediate representation |
| [`4-code-generation/`](4-code-generation) | **the complete compiler**: IR to NASM x86-64 assembly |
| [`5-optimization/`](5-optimization) | optimisation passes over the IR |

Phase 2 is worth a look next to phase 3: the same grammar, once parsed by hand and once
declared to Bison. Writing the recursive-descent parser first is what made the
generated one legible.

## Optimisations

The pass in [`5-optimization/src/ir.cpp`](5-optimization/src/ir.cpp) works on the
intermediate representation and implements:

- **constant propagation** — an identifier bound to a literal is replaced by the literal;
- **constant folding** — an operation between two literals is evaluated at compile time;
- **common subexpression elimination** — a repeated expression reuses the temporary that
  already holds it, with commutative operations canonicalised so that `a+b` and `b+a`
  are recognised as the same expression;
- **basic-block boundaries** — the expression table is reset at every label and jump, so
  that no value is carried across a branch it does not dominate.

## Building and running

Requires a C++17 compiler, CMake 3.16 or later, Flex and Bison. On macOS the Homebrew
versions of Flex and Bison are needed, since the ones shipped with the system are too old.

```bash
cd 4-code-generation
cmake -B build
cmake --build build
./build/compiler example-program.txt
```

The compiler writes `output.asm` next to the input file. To turn it into an executable:

```bash
nasm -f elf64 output.asm -o output.o
ld output.o -o program
./program
```

Running the compiler without an argument makes it look for a `read.txt` in a few
sensible places, which is how the course submissions were expected to behave.

## Documentation

[`docs/final-report.pdf`](docs/final-report.pdf) is the report submitted with the final
phase: design decisions, the structure of the IR, and the reasoning behind the code
generation strategy.
