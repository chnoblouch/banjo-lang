# The Banjo Compiler

The compiler is a piece of software that takes in source code and outputs either [object files](https://en.wikipedia.org/wiki/Object_file) or [assembly code](https://en.wikipedia.org/wiki/Assembly_language). This process is split up into different phases. Each phase takes the output from the previous one and transforms it into something that is a bit closer to the final output.

## Compilation Phases

### Module Loading

The module manager looks for files ending with `.bnj` in the search paths and adds them to a list of modules. The source code of each module is then loaded into memory and processed by the lexer and parser.

#### Lexer

The [lexer](https://en.wikipedia.org/wiki/Lexical_analysis) takes the input source code and breaks it up into _tokens_. A token can be thought of as a "word". Tokens store their value, location and type. The type specifies the class a token belongs to. The lexer strips out whitespace and comments.

For example, this source code:
```
func main() {
    var a = 100;
}
```

gets tokenized into:
```
FUNC
IDENTIFIER: "main"
LPAREN
RPAREN
LBRACE
VAR
IDENTIFIER: "a"
EQ
LITERAL: "100"
SEMI
RBRACE
```

#### Parser

The [parser](https://en.wikipedia.org/wiki/Parsing#Parser) takes the tokens from the lexer and parses them into an [abstract syntax tree (AST)](https://en.wikipedia.org/wiki/Abstract_syntax_tree). The AST stores the program as a tree of nodes. Each function, statement, or expression is represented as a different node.

For example, this function:
```
func print_number(number: i32) {	
    if number == 2 {
        println("Two");
    } else {
        println("Not two");
    }
}
```

gets parsed into this AST:
```
FUNCTION_DEFINITION
  QUALIFIER_LIST
  IDENTIFIER: "print_number"
  PARAM_LIST
    PARAM
      IDENTIFIER: "number"
      I32
  VOID
  BLOCK
    IF_CHAIN
      IF
        OPERATOR_EQ
          IDENTIFIER: "number"
          INT_LITERAL: "2"
        BLOCK
          FUNCTION_CALL
            IDENTIFIER: "println"
            FUNCTION_ARGUMENT_LIST
              STRING_LITERAL: "Two"
      ELSE
        BLOCK
          FUNCTION_CALL
            IDENTIFIER: "println"
            FUNCTION_ARGUMENT_LIST
              STRING_LITERAL: "Not two"
```

### Semantic Analyzer

The [semantic analyzer](https://en.wikipedia.org/wiki/Semantic_analysis_(compilers)) validates the AST and annotates it with additional information. One of its tasks is to find all [symbols](https://en.wikipedia.org/wiki/Symbol_table) in the code. A symbol is a code entity that has a name and can be referenced from other places in the code. The semantic analyzer also performs [type checking](https://en.wikipedia.org/wiki/Type_system#Type_checking), the process of verifying the typing of the program. It's also in this phase where generics and meta statements are expanded.

Semantic analysis is performed in multiple stages.

#### Name Stage

Looks for symbols in the code and stores them in the AST. This stage
only analyzes the name of the symbols, types are handled later on.

#### Meta Expansion Stage

Expands `meta if` and `meta for` statements.

#### Use Resolution Stage

Resolves `use` declarations to actual symbols in other modules.

#### Type Stage

Analyzes the types of the declarations and stores them in the AST.

#### Body Stage

Analyzes the bodies of functions and the values of variables.

### SSA Generation

After semantic analysis, the program is transformed into a custom [intermediate representation (IR)](https://en.wikipedia.org/wiki/Intermediate_representation). The IR used is based on the [static single-assignment form](https://en.wikipedia.org/wiki/Static_single-assignment_form), a data structure that simplifies many optimizations. In SSA, each variable is only defined once, which means that for each variable assignment in the original code, a new variable has to be created. Control flow in SSA is specified using _basic blocks_. Each basic block contains a list of _instructions_ that perform some operation on _variables_. Each basic block ends with a _branch instruction_ that transfers control flow either to a fixed basic block or to one of two basic blocks depending on a condition. Values are propagated to other basic blocks using _basic block arguments_ (whereas compiler text books generally use _phi functions_).

### Optimization

This is the fun part of compiler construction. The compiler runs multiple passes over the IR and transforms it step-by-step into a more efficient form. Each pass looks for a specific pattern in the code and simplifies it. Some optimizations enable each other, which is why some passes are run multiple times.

#### Dead Function Elimination

Removes functions that are never called or referenced.

#### Peephole Optimizer

Performs simple optimizations such as transforming `b = a + 0` into `a = b`.

Here's a list of optimization tricks the peephole optimizer currently knows:
- Replacing `x + 0` and `x - 0` with `x`
- Replacing `x * 1` with `x`
- Replacing multiplies by powers of two with bitshifts: `x * 4` -> `x << 2`
- Replacing unsigned divides by powers of two with bitshifts: `x / 8` -> `x >> 3`
- Replacing calls to `fsqrt` from libc with the IR instruction `sqrt` that gets lowered to a machine instruction during codegen

If you want to know what other peephole optimizations are possible, look at the source code for the [LLVM InstCombine pass](https://llvm.org/doxygen/InstructionCombining_8cpp_source.html), which has over 5000 lines of code.

#### Control Flow Optimization

Simplies the [control flow graph (CFG)](https://en.wikipedia.org/wiki/Control-flow_graph). The CFG stores the predecessors and successors of each basic block of a function. The CFG optimization pass merges basic blocks into their predecessors if they only have one and removes unreachable basic blocks.

#### Stack To Register Pass

Converts stack slots to SSA variables if possible. This improves performance greatly because SSA variables are most likely put into registers later on, whereas stack slots have to be put into slow stack memory.

#### Loop Inversion

Converts `while` loops into `do/while` loops. This is more efficient because `while` loops require two branch instructions: one to branch from the end of the loop to its header and another to conditionally branch from the header either into the loop body or to the exit. `do/while` loops only require one branch instruction before running the loop to make sure it's run at least once and one branch instruction per iteration to conditionally branch from the end back to the header. The compiler might even realize later on that a loop is always entered, and reduce it to a single branch instruction.

#### Branch Elimination

Replaces assigning one of two values to a variable by branching with a `select` instruction. `select`s pick a value depending on a condition and are lowered to much more efficient machine instructions than branches.

#### Inlining

Replaces function calls with the body of the callee. This optimization not only reduces the number of branching due to call instructions, but also boosts other optimizations because all the code of the callee becomes visible to the optimizer. For example, a call like `add(5, 10)` can be folded to `15` because the optimizer sees that the function returns the sum of its arguments and is only called with constants.

#### Loop-Invariant Code Motion

Moves calculations out of loops if their value doesn't change during the loop.

### Code Generation

Codegen is the target-specific phase of the compiler. This phase generates machine instructions based on the SSA IR. The IR for machine instructions is called _MCode_.

#### IR Lowering

The IR is lowered into actual machine instructions. The registers and stack slots are not assigned yet to physical registers during this phase. The variables from the IR are converted to _virtual registers_ instead.

#### Register Allocation

Register allocation is the process of assigning virtual registers to physical registers. This is super hard because a typical instruction set has around 16-32 physical registers while there might be thousands of virtual registers. Also not helping is the fact that some machine instructions expect their arguments to be in specific registers and function calls have to follow a _calling convention_ that also specifies where the arguments to a function call should be stored. This problem is tackled using [liveness analysis](https://en.wikipedia.org/wiki/Live-variable_analysis), a process that tells us for each virtual register, during which intervals it is used. The register allocator uses this information to allocate physical registers while making sure that the same physical register is never used for multiple different virtual registers at the same time. When the register allocator runs out of physical registers, it _spills_ virtual registers to the stack.

#### Stack Frame Building

The [stack frame](https://en.wikipedia.org/wiki/Call_stack) stores the local variables of a function invocation. The stack frame builder assigns stack slots to actual locations in the stack frame. These locations are typically offsets from the so-called _stack pointer_, which points to the current top of the stack.

#### Prolog/Epilog Insertion

The prolog and epilog of a function are a few machine instructions that set up the stack frame and save/restore some registers that have to be preserved according to the calling convention.

### Emission

The built-in assembler takes the machine instructions and encodes them into binary machine code. Currently there is only an encoder for x86-64. For AArch64 targets, assembly code is emitted instead, which has to be processed by an assembler first. The compiler adds some additional information to the machine code like global data or the location of functions in the code. All this data is packed together and emitted into an [object file](https://en.wikipedia.org/wiki/Object_file), ready to be read by a linker to produce an executable. The format of the object file is dependent on the target operating system. The compiler currently supports the [Portable Executble (PE)](https://en.wikipedia.org/wiki/Portable_Executable) format for Windows and the [Executable and Linkable Format (ELF)](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) for Linux.
