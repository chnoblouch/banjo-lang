# The Banjo Compiler

The compiler is a tool that takes in source code and outputs either [object
files](https://en.wikipedia.org/wiki/Object_file) or [assembly
code](https://en.wikipedia.org/wiki/Assembly_language). This process is split into different phases.
Each phase takes the output from the previous one and transforms it into something that is a bit
closer to the final output.

## Compilation Phases

### Module Loading

The module manager looks for files ending with `.bnj` in the search paths and adds them to a list of
modules. The source code of each module is then loaded into memory and processed by the lexer and
parser.

#### Lexer

The [lexer](https://en.wikipedia.org/wiki/Lexical_analysis) takes the input source code and breaks
it up into _tokens_. A token can be thought of as a "word". Tokens store their value, location and
type. The type specifies the class a token belongs to. The lexer strips out whitespace and comments.

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

The [parser](https://en.wikipedia.org/wiki/Parsing#Parser) takes the tokens from the lexer and
parses them into an [abstract syntax tree
(AST)](https://en.wikipedia.org/wiki/Abstract_syntax_tree). The AST stores the program as a tree of
nodes. Each function, statement, or expression is represented as a different node.

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

The parsing technique used is [recursive
descent](https://en.wikipedia.org/wiki/Recursive_descent_parser). When the parser encounters a
syntax error, it emits an error report and tries to recover by skipping tokens until it finds an
opportunity to continue parsing. Examples of recovery points are the end of a block (`}`) or the
start of a variable declaration (`var`). The parser is able to generate partial ASTs for broken
code. These partial ASTs can be further analyzed by the semantic analyzer to produce more complete
error reports.

### SIR Generation

After the AST is constructed, it gets translated into an [intermediate representation
(IR)](https://en.wikipedia.org/wiki/Intermediate_representation) that is is more suitable for
representing semantics than the AST. This 'SIR' looks pretty similar to the AST, but it contains
extra things like types or references between variables.

For example, this function:
```
func main() {
    for i in 0..10 {
        var a = 2 * i;
        println(a);
    }
}
```

is represented like this in SIR:
```
FuncDef
  ident: main
  type: FuncType
    params: 
    return_type: PrimitiveType
      primitive: VOID
  block: 
    ForStmt
      by_ref: false
      ident: i
      range: RangeExpr
        lhs: IntLiteral
          type: none
          value: 0
        rhs: IntLiteral
          type: none
          value: 10
      block: 
        VarStmt
          local: Local
            name: a
            type: none
          value: BinaryExpr
            type: none
            op: MUL
            lhs: IntLiteral
              type: none
              value: 2
            rhs: IdentExpr
              value: "i"
        Expr
          value: CallExpr
            type: none
            callee: IdentExpr
              value: "println"
            args: 
              IdentExpr
                value: "a"
```

You might haved noticed that types are still missing at this point, which is why the next phase is
the...

### Semantic Analyzer

The [semantic analyzer](https://en.wikipedia.org/wiki/Semantic_analysis_(compilers)) checks the
semantics of the program. It does things like type checking, collecting symbols (functions,
variables, structs, etc.) into symbol tables, resolving identifiers into references, or tracking
resource lifetimes. It also desugars many constructs into simpler ones to simplify the next stage
(SSA generation). For example, `WhileStmt`s and `ForStmts`s both get turned into `LoopStmt`s.

The semantic analyzer is split into multiple stages:

1. Symbol collecting: Looks for symbols in the program and inserts them into symbol tables.
2. Meta expansion: Evaluates `meta if` and `meta for` statements and replaces them with their
   generated SIR nodes.
3. Use resolution: Resolves targets of `use` declarations.
4. Type alias resolution: Resolves the underlying type of type aliases.
5. Declaration interface analysis: Analyzes the public interface of declarations (function parameter
   types, union cases, enum variant types, etc.)
6. Declaration body analysis: Analyzes the body of declarations (function blocks, const values,
   etc.)
7. Resource analysis: Collects resources, and checks that they are initialized, moved, and
   deinitialized correctly.

The advantage of a multi-stage semantic analyzer is that the declaration order of types, functions
and variables doesn't matter. You can use a global constant in a function even if it is declared
further down.

Here's what a simple function that just prints `10` looks like before semantic analysis:
```
FuncDef
  ident: main
  type: FuncType
    params: 
    return_type: PrimitiveType
      primitive: VOID
  block: 
    Expr
      value: CallExpr
        type: none
        callee: IdentExpr
          value: "println"
        args: 
          IntLiteral
            type: none
            value: 10
```

And after semantic analysis:
```
FuncDef
  ident: main
  type: FuncType
    params: 
    return_type: PrimitiveType
      primitive: VOID
  block: 
    Expr
      value: CallExpr
        type: PrimitiveType
          primitive: VOID
        callee: SymbolExpr
          type: FuncType
            params: 
              Param
                name: value
                type: PrimitiveType
                  primitive: I32
            return_type: PrimitiveType
              primitive: VOID
          name: "println"
        args: 
          IntLiteral
            type: PrimitiveType
              primitive: I32
            value: 10
```

### SSA Generation

After semantic analysis, the program is transformed into another IR that is based on the [static
single-assignment form](https://en.wikipedia.org/wiki/Static_single-assignment_form). SSA IR is
commonly used in compilers because it simplifies many optimizations. In SSA, each variable is only
defined once, which means that for each variable assignment in the original code, a new variable has
to be created. Control flow in SSA is specified using _basic blocks_. Each basic block contains a
list of _instructions_ that perform some operation on _variables_. Each basic block ends with a
_branch instruction_ that transfers control flow either to a fixed basic block or to one of two
basic blocks depending on a condition. Values are propagated to other basic blocks using _basic
block arguments_ (whereas compiler text books generally use _phi functions_).

The most important instructions of the IR are:
- `alloca`: Allocate a new stack slot for storing local values
- `load`: Load a value from memory
- `store`: Store a value to memory
- `loadarg`: Load a function argument by its index
- `add`, `sub`, `mul`, `sdiv`, `srem`, `udiv`, `urem`: Integer arithmetic
- `fadd`, `fsub`, `fmul`, `fdiv`: Floating-point arithmetic
- `and`, `or`, `xor`, `shl`, `shr`: Bitwise operations
- `jmp`: Unconditional branching
- `cjmp`, `fcjmp`: Conditional branching
- `select`: Select one of two values depending on a condition
- `call`: Function call
- `ret`: Function return
- `memberptr`: Compute the pointer to a struct member
- `offsetptr`: Add an offset to a pointer
- `copy`: Copy data from one memory address to another

Here's an example of a function transformed into SSA:

Source:
```
func sum(numbers: *i32, count: i32) -> i32 {
    var sum = 0;

    for i in 0..count {
        sum += numbers[i];
    }

    return sum;
}
```

SSA IR:
```
func i32 sum(addr, i32)
    %0 = loadarg addr, addr 0
    %1 = loadarg i32, addr 1
    jmp for.header(0, 0)

for.header(i32 %2, i32 %3):
    cjmp i32 %3, ne, i32 %1, for.block, for.exit

for.block:
    %4 = offsetptr addr %0, i32 %3, i32
    %5 = load i32, addr %4
    %6 = add i32 %2, i32 %5
    %7 = add i32 %3, i32 1
    jmp for.header(%6, %7)

for.exit:
    ret i32 %2
```

Note that this is an already optimized version of the SSA IR and not what the SSA generator actually
generates for the above function. The initial IR uses stack slots instead of SSA variables for
pretty much everything, which means that even a simple `add` function turns out as complicated as
this:

```
func i32 add(i32, i32)
    %0 = alloca i32
    %1 = alloca i32 !arg_store
    %2 = alloca i32 !arg_store
    %3 = loadarg i32, 0 !save_arg
    store i32 %3, addr %1 !save_arg
    %4 = loadarg i32, 1 !save_arg
    store i32 %4, addr %2 !save_arg
    %5 = load i32, addr %1
    %6 = load i32, addr %2
    %7 = add i32 %5, i32 %6
    store i32 %7, %0
    jmp block.exit

block.exit:
    %8 = load i32, addr %0
    ret i32 %8
```

Here, the compiler stores the arguments in stack slots at the start of the function, loads the
values back in order to add them, stores the result in another stack slot, and finally loads the
value back in the exit block to return it.

### Optimization

This is the fun part of compiler construction. The compiler runs multiple passes over the IR and
transforms it step-by-step into a more efficient form. Each pass looks for a specific pattern in the
code and simplifies it. Some optimizations enable each other, which is why some passes are run
multiple times.

#### Dead Function Elimination

Removes functions that are never called or referenced.

#### Peephole Optimizer

Performs simple optimizations such as transforming `b = a + 0` into `b = a`.

Here's a list of optimization tricks the peephole optimizer currently knows:
- Replacing `x + 0` and `x - 0` with `x`
- Replacing `x * 1` with `x`
- Replacing multiplies by powers of two with bitshifts: `x * 4` -> `x << 2`
- Replacing unsigned divides by powers of two with bitshifts: `x / 8` -> `x >> 3`
- Replacing calls to `fsqrt` from libc with the IR instruction `sqrt` that gets lowered to a machine
  instruction during codegen

If you want to know what other peephole optimizations are possible, look at the source code for the
[LLVM InstCombine pass](https://llvm.org/doxygen/InstructionCombining_8cpp_source.html), which has
over 5000 lines of code.

#### Control Flow Optimization

Simplifies the [control flow graph (CFG)](https://en.wikipedia.org/wiki/Control-flow_graph). The CFG
stores the predecessors and successors of each basic block of a function. The CFG optimization pass
merges basic blocks into their predecessors if they only have one and removes unreachable basic
blocks.

#### Stack To Register Pass

Converts stack slots to SSA variables if possible. This improves performance greatly because SSA
variables are most likely put into registers later on, whereas stack slots have to be put into slow
stack memory.

#### Loop Inversion

Converts `while` loops into `do/while` loops. This is more efficient because `while` loops require
two branch instructions: one to branch from the end of the loop to its header and another to
conditionally branch from the header either into the loop body or to the exit. `do/while` loops only
require one branch instruction before entering the loop to make sure it's run at least once and one
branch instruction per iteration to conditionally branch from the end back to the header. The
compiler might even realize later on that a loop is always entered, and reduce it to a single branch
instruction.

#### Branch Elimination

Replaces assigning one of two values to a variable by branching with a `select` instruction.
`select`s pick a value depending on a condition and are lowered to conditional move instructions
that avoid costly [branch mispredictions](https://en.wikipedia.org/wiki/Branch_predictor) during
execution.

#### Inlining

Replaces function calls with the body of the callee. This optimization not only reduces the number
of branching due to call instructions, but also boosts other optimizations because all the code of
the callee becomes visible to the optimizer. For example, a call like `add(5, 10)` can be folded to
`15` because the optimizer sees that the function returns the sum of its arguments and is only
called with constants.

#### Loop-Invariant Code Motion

Moves calculations out of loops if their value doesn't change during the loop.

### Code Generation

Codegen is the target-specific phase of the compiler. This phase generates machine instructions
based on the SSA IR. The IR for machine instructions is called _MCode_.

#### IR Lowering

The IR is lowered into actual machine instructions. The registers and stack slots are not assigned yet to physical
registers during this phase. The variables from the IR are converted to _virtual registers_ instead.

The lowerer doesn't just mechanically translate from SSA instructions to machine instructions. It's
also capable of detecting patterns and optimizing them. For example, the instruction `%2 = mul %1,
3` is lowered to `lea %2, [%1 + 2 * %1]` on x86-64, because `LEA` instructions have lower latency
than `IMUL` instructions. The lowerer also tries to minimize memory accesses to float constants by
storing the constant in a register if it is used by multiple instructions that are close to each
other.

#### Register Allocation

Register allocation is the process of mapping the virtual registers to a limited set of physical
registers. This is super hard because a typical instruction set has 16 or 32 physical registers
while there might be thousands of virtual registers. Even worse, some machine instructions expect
their arguments to be in specific registers and function calls have to follow a _calling convention_
that specifies in which registers arguments are passed and values are returned.

These problems are tackled using [liveness
analysis](https://en.wikipedia.org/wiki/Live-variable_analysis), an analysis that tells us for each
virtual register, during which intervals it is used. The register allocator uses this information to
assign physical registers while making sure that the same physical register is never used for
multiple different virtual registers at the same time. When the register allocator runs out of
physical registers, it moves virtual registers to the stack.

Register allocation is done in multiple steps:
1. Compute the liveness of virtual and physical registers. Liveness analysis first computes which
   variables are alive at the start and end of a block (_ins_ and _outs_) and then computes precise
   _live ranges_ from there. A live range records the start and end _program point_ where a register
   is defined and/or used.
2. Assign target-specific register classes to the virtual registers. This records which class of
   physical registers may be assigned to a virtual register. For example, on x86-64 there are two
   classes: general purpose registers (`rax`-`r15`) and SSE registers (`xmm0`-`xmm15`).
3. Reserve fixed ranges for physical registers. This is used when function call arguments have to be
   prepared or when instructions read/write specific registers.
4. For each virtual register, create a _bundle_ of live ranges that are allocated as one unit.
5. _Coalesce_ bundles that are connected by a copy instruction and don't interfere. This reduces
   copying by assigning the same physical register to related bundles.
6. Put the bundles into a queue and allocate them one by one. Bundles with longer live ranges are
   put at the front of the queue as they tend to be more constrained. The allocator puts each bundle
   in one of multiple places:
    1. First, try to assign one of the registers suggested by the backend for this particular
       instruction.
    2. Next, try to assign any of the registers that are legal for the current register class.
    3. Next, try to _evict_ an existing allocation (this is currently only done for live ranges that
       were created by spilling)
    4. If nothing else works, _spill_ the virtual register to the stack. This splits the live range
       into smaller ranges that are put back into the queue. At the start of each sub-range, the
       value is loaded from the stack into a temporary register and stored back on the stack at the
       end of the range.
7. Apply the allocations by replacing the virtual registers with physical registers and inserting
   spill code.
8. Clean up the code by removing redundant instructions that may have resulted from register
   allocation. For example, `mov %1, %2` might have been transformed into `mov rdx, rdx` due to
   coalescing and can now be removed.

#### Stack Frame Building

The [stack frame](https://en.wikipedia.org/wiki/Call_stack) stores the local variables of a function
invocation. The stack frame builder assigns stack slots to actual locations in the stack frame.
These locations are typically offsets from the hardware stack pointer.

#### Prolog/Epilog Insertion

The prolog and epilog of a function are a few machine instructions that set up the stack frame and
save/restore registers that have to be preserved according to the calling convention.

### Emission

The built-in assembler takes the machine instructions and encodes them into binary machine code.
This machine code is packed together with some other data like symbol tables, relocation tables,
globals, etc. and emitted into an [object
file](https://en.wikipedia.org/wiki/Object_file), ready to be read by a linker to produce an
executable. The format of the object file is dependent on the target operating system. The compiler
currently supports the [Portable Executable (PE)](https://en.wikipedia.org/wiki/Portable_Executable)
format for Windows, the [Executable and Linkable Format
(ELF)](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) for Linux, and the
[Mach-O](https://en.wikipedia.org/wiki/Mach-O) format for macOS.
