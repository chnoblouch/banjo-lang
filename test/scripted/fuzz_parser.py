import random
import os
import subprocess
import time

TOKENS = [
    "var",
    "const",
    "func",
    "i8",
    "i16",
    "i32",
    "i64",
    "u8",
    "u16",
    "u32",
    "u64",
    "f32",
    "f64",
    "usize",
    "bool",
    "addr",
    "void",
    "except",
    "as",
    "if",
    "else",
    "switch",
    "while",
    "for",
    "in",
    "break",
    "continue",
    "return",
    "struct",
    "self",
    "enum",
    "union",
    "case",
    "false",
    "true",
    "null",
    "none",
    "undefined",
    "import",
    "export",
    "use",
    "pub",
    "native",
    "meta",
    "type",
    "identifier",
    "+",
    "+=",
    "-",
    "-=",
    "*",
    "*=",
    "/",
    "/+",
    "%",
    # "%=",
    "&",
    "&&",
    "&=",
    "|",
    "||",
    "|=",
    "!",
    "!=",
    "^",
    "~",
    ">",
    ">>",
    ">=",
    ">>=",
    "<",
    "<<",
    "<=",
    "<<=",
    "=",
    "==",
    "(",
    ")",
    "{",
    "}",
    "[",
    "]",
    ";",
    ":",
    ",",
    ".",
    "..",
    "...",
    "?",
    "0",
    "10",
    "0.5",
    "10.5",
]

FRAGMENTS = [
    "func f",
    "func f(",
    "func f(a",
    "func f(a:",
    "func f(a: i32",
    "func f(a: i32,",
    "func f(a: i32, b",
    "func f(a: i32, b:",
    "func f(a: i32, b: f32",
    "func f(a: i32, b: f32)",
    "func f(a: i32, b: f32) ->",
    "func f(a: i32, b: f32) -> bool",

    "struct",
    "struct S",
    "struct S {",

    "union",
    "union U",
    "union U {",
    "union U { case",
    "union U { case A",
    "union U { case A(",
]

FILE_NAME = "tmp.bnj"

if __name__ == "__main__":
    for i in range(10000):
        print("iteration " + str(i) + " ... ", end="")

        # string = "func var func func = identifier 0.0 = 0.0 } { = case { case identifier identifier { "
    
        string = ""
        for j in range(random.randrange(5, 50)):
            if random.randrange(2) == 0:
                string += random.choice(TOKENS) + " "
            else:
                string += random.choice(FRAGMENTS) + " "

        tmp_file = open(FILE_NAME, "w")
        tmp_file.write(string)
        tmp_file.close()

        command = ["banjo-compiler.exe", "--path", "."]
        process = subprocess.Popen(command, stderr=subprocess.DEVNULL)
        
        start_time = time.time()
        timed_out = True

        while time.time() - start_time < 0.5:
            if process.poll() is not None:
                timed_out = False
                break
        
        if timed_out:
            print(f"failed: compiler timed out\n  {string}")
            process.kill()
            process.wait()
            input()
        elif process.returncode != 0 and process.returncode != 1:
            print(f"failed: unexpected exit code {process.returncode}\n  {string}")
            input()
        else:
            print("ok")

        os.remove(FILE_NAME)
