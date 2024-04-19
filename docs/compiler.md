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

The lowerer doesn't just mechanically translate from SSA instructions to machine instructions. It's also capable of detecting patterns and optimizing them. For example, the instruction `%2 = mul %1, 3` is lowered to `lea %2, [%1 + 2 * %1]` on x86-64, because `LEA` instructions have lower latency than `IMUL` instructions. The lowerer also tries to minimize memory accesses to float constants by storing the constant in a register if it used by multiple instructions that are close to each other. 

#### Register Allocation

Register allocation is the process of mapping a virtual registers to a limited set of physical registers. This is super hard because a typical instruction set has 16 or 32 physical registers while there might be thousands of virtual registers. Even worse, some machine instructions expect their arguments to be in specific registers and function calls have to follow a _calling convention_ that specifies in which registers arguments are passed and values are returned.

These problems are tackled using [liveness analysis](https://en.wikipedia.org/wiki/Live-variable_analysis), an analysis that tells us for each virtual register, during which intervals it is used. The register allocator uses this information to assign physical registers while making sure that the same physical register is never used for multiple different virtual registers at the same time. When the register allocator runs out of physical registers, it moves virtual registers to the stack.

Register allocation is done in multiple steps:
1. Compute the liveness of virtual and physical registers. Liveness analysis first computes which variables are alive at the start and end of a block (_ins_ and _outs_) and then computes precise _live ranges_ from there. A live range records the start and end _program point_ where a register is defined and/or used.
2. Assign target-specific register classes to the virtual registers. This records which class of physical registers may be assigned to a virtual register. For example, on x86-64 there are two classes: general purpose registers (`rax`-`r15`) and SSE registers (`xmm0`-`xmm15`).
3. Reserve fixed ranges for physical registers. This is used when functions are called that expect their arguments in specific registers or when instructions read/write predefined registers.
4. For each virtual register, create a _bundle_ of live ranges that are allocated as one unit.
5. _Coalesce_ bundles that are connected by a copy instruction and don't interfere. This reduces copying by assigning the same physical register to related bundles.
6. Put the bundles into a queue and allocate them one by one. Bundles with longer live ranges are put at the front of the queue as they tend to be more constrained. The allocator puts each bundle in one of multiple places: 
    1. First, try to assign one of the registers suggested by the backend for this particular instruction.
    2. Next, try to assign any of the registers that are legal for the current register class.
    3. Next, try to _evict_ an existing allocation (this is currently only done for live ranges that were created by spilling)
    4. If nothing else works, _spill_ the virtual register to the stack. This splits the live range into smaller ranges that are put back into the queue. At the start of each sub-range, the value is loaded from the stack into a temporary register and stored back on the stack at the end of the range.
7. Apply the allocations by replacing the virtual registers with physical registers and inserting spill code.
8. Clean up the code by removing redundant instructions that may have resulted from register allocation. For example, `mov %1, %2` might have been transformed into `mov rdx, rdx` due to coalescing and can now be removed. 

#### Stack Frame Building

The [stack frame](https://en.wikipedia.org/wiki/Call_stack) stores the local variables of a function invocation. The stack frame builder assigns stack slots to actual locations in the stack frame. These locations are typically offsets from the so-called _stack pointer_, which points to the current top of the stack.

#### Prolog/Epilog Insertion

The prolog and epilog of a function are a few machine instructions that set up the stack frame and save/restore some registers that have to be preserved according to the calling convention.

### Emission

The built-in assembler takes the machine instructions and encodes them into binary machine code. Currently there is only an encoder for x86-64. For AArch64 targets, assembly code is emitted instead, which has to be processed by an assembler first. The compiler adds some additional information to the machine code like global data or the location of functions in the code. All this data is packed together and emitted into an [object file](https://en.wikipedia.org/wiki/Object_file), ready to be read by a linker to produce an executable. The format of the object file is dependent on the target operating system. The compiler currently supports the [Portable Executble (PE)](https://en.wikipedia.org/wiki/Portable_Executable) format for Windows and the [Executable and Linkable Format (ELF)](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) for Linux.
