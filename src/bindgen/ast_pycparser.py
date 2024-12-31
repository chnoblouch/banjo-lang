import pycparser
from pycparser import c_ast
from bindings import *
from pathlib import Path


def parse(filename, include_paths):
    include_dir = Path(__file__).parent.parent / "utils" / "fake_libc_include"

    cpp_args = ["-E", "-nostdinc", f"-I{include_dir.absolute()}"]
    cpp_args += [f"-I{path}" for path in include_paths]

    return pycparser.parse_file(
        filename,
        use_cpp=True,
        cpp_path="clang",
        cpp_args=cpp_args
    )

class ASTConverter:

    def __init__(self, ast):
        self.ast = ast

    def generate(self) -> Bindings:
        symbols = {}

        for child in self.ast:
            if type(child) is c_ast.Decl:
                if type(child.type) is c_ast.FuncDecl:
                    _, func = self.gen_func(child.type)
                    symbols[func.name] = func
                elif type(child.type) is c_ast.Struct:
                    struct = self.gen_struct(child.type)
                    symbols[struct.name] = struct
                elif type(child.type) is c_ast.Enum:
                    enum = self.gen_enum(child)
                    symbols[enum.name] = enum
                elif "const" in child.type.quals:
                    const = self.gen_const(child)
                    symbols[const.name] = const
                else:
                    print(child)
            elif type(child) is c_ast.Typedef:
                typedef = self.gen_typedef(child)
                symbols[typedef.name] = typedef

        return Bindings(symbols.values())

    def gen_func(self, type_):
        name, result = self.gen_decl(type_)

        params = []
        if type_.args is not None:
            for arg_decl in type_.args:
                param_name, param_type = self.gen_decl(arg_decl)
                params.append(Param(param_name, param_type))

        return name, Function(name, params, result)

    def gen_const(self, type_):
        value = self.gen_value(type_.init)
        name, type_ = self.gen_decl(type_)

        return Constant(name, type_, value)

    def gen_typedef(self, typedef):
        name, type_ = self.gen_decl(typedef)

        if type(type_) is Struct or type(type_) is Enum:
            return type_
        else:
            return TypeAlias(name, type_)

    def gen_decl(self, decl) -> tuple[str, Type]:
        if type(decl.type) is c_ast.PtrDecl:
            name, type_ = self.gen_decl(decl.type)
            type_ = PtrType(type_)
        elif type(decl.type) is c_ast.ArrayDecl:
            name, type_ = self.gen_decl(decl.type)
            length = self.gen_value(decl.type.dim)
            type_ = ArrayType(type_, length)
        elif type(decl.type) is c_ast.FuncDecl:
            name, type_ = self.gen_func(decl.type)
        elif type(decl.type) is c_ast.TypeDecl:
            name = decl.type.declname
            type_ = self.gen_basic_type(decl.type.type)
        else:
            name, type_ = None, None

        return name, type_

    def gen_basic_type(self, type_):
        if type(type_) is c_ast.IdentifierType:
            return IdentifierType(type_.names[0])
        elif type(type_) is c_ast.Struct:
            return self.gen_struct(type_)
        elif type(type_) is c_ast.Enum:
            return self.gen_enum(type_)
        elif type(type_) is c_ast.Union:
            return self.gen_union(type_)
        else:
            print(type(type_))
            return None

    def gen_struct(self, struct) -> Struct:
        members = []

        if struct.decls:
            for member_decl in struct.decls:
                member = self.gen_decl(member_decl)
                members.append(member)

        return Struct(struct.name, members)

    def gen_enum(self, enum) -> Enum:
        name = enum.name

        variants = []
        if hasattr(enum, "values"):
            for enumerator in enum.values.enumerators:
                value = self.gen_value(enumerator.value)
                variants.append((enumerator.name, value))

        return Enum(name, variants)

    def gen_union(self, union):
        name = union.name
        return Union(name)

    def gen_value(self, const):
        if type(const) == c_ast.UnaryOp and const.op == "-":
            return -self.gen_value(const.expr)

        if type(const) != c_ast.Constant:
            return None

        literal = const.value.lower()

        if literal.endswith("u"):
            literal = literal[:-1]
        elif literal.endswith("ull"):
            literal = literal[:-3]
        elif literal.endswith("ll"):
            literal = literal[:-2]

        if literal.startswith("0x"):
            return int(literal[2:], 16)
        else:
            return int(literal)
