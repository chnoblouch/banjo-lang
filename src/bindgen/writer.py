from bindings import *
from generator import *
import utils


KEYWORDS = set([
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
    "try",
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
    "proto",
    "false",
    "true",
    "null",
    "none",
    "undefined",
    "use",
    "pub",
    "native",
    "meta",
    "type",
])


class Writer:
    def __init__(self, bindings: Bindings, filename):
        self.bindings = bindings
        self.file = open(filename, "w")

    def write(self):
        last_type = None

        for symbol in self.bindings.symbols:
            if last_type != type(symbol):
                if last_type not in (None, Struct, Union, Enum):
                    self.file.write("\n")
                last_type = type(symbol)

            if type(symbol) is Function:
                self.write_func(symbol)
            elif type(symbol) is Constant:
                self.write_const(symbol)
            elif type(symbol) is Struct:
                self.write_struct(symbol)
            elif type(symbol) is Union:
                self.write_union(symbol)
            elif type(symbol) is Enum:
                self.write_enum(symbol)
            elif type(symbol) is TypeAlias:
                self.write_type_alias(symbol)

    def write_func(self, func: Function):
        self.file.write(f"@[link_name={func.original_name}] native func ")
        self.write_identifier(utils.to_snake_case(func.name))

        self.file.write("(")
        self.write_params(func.params)
        self.file.write(")")

        if not (type(func.result) == IdentifierType and func.result.name == "void"):
            self.file.write(" -> ")
            self.write_type(func.result)

        self.file.write(";\n")

    def write_const(self, const: Constant):
        self.file.write("const ")
        self.file.write(const.name)
        self.file.write(": ")
        self.write_type(const.type)
        self.file.write(" = ")
        self.file.write(str(const.value))
        self.file.write(";\n")

    def write_struct(self, struct: Struct):
        self.file.write("struct ")
        self.write_identifier(struct.name)
        self.file.write(" {\n")

        for field in struct.fields:
            self.file.write("    var ")
            self.write_identifier(field.name)
            self.file.write(": ")
            self.write_field_type(field.type)
            self.file.write(";\n")

        self.file.write("}\n\n")

    def write_union(self, union: Union):
        self.file.write("@[layout=overlapping]\nstruct ")
        self.write_identifier(union.name)
        self.file.write(" {\n")

        for field in union.fields:
            self.file.write("    var ")
            self.write_identifier(field.name)
            self.file.write(": ")
            self.write_field_type(field.type)
            self.file.write(";\n")

        self.file.write("}\n\n")

    def write_enum(self, enum: Enum):
        self.file.write("enum ")
        self.write_identifier(enum.name)
        self.file.write(" {\n")

        for i, variant in enumerate(enum.variants):
            self.file.write("    ")
            self.write_identifier(variant.name)

            if variant.value:
                self.file.write(f" = {variant.value}")

            if i != len(enum.variants) - 1:
                self.file.write(",")
            self.file.write("\n")

        self.file.write("}\n\n")

    def write_type_alias(self, type_alias: TypeAlias):
        self.file.write("type ")
        self.write_identifier(type_alias.name)
        self.file.write(" = ")
        self.write_type(type_alias.type_)
        self.file.write(";\n")

    def write_type(self, type_):
        if type(type_) == PrimitiveType:
            self.file.write(type_.name)
        elif type(type_) == IdentifierType:
            if self.bindings.resolve_symbol(type_.name):
                type_ = self.resolve_type(type_)
                self.write_identifier(type_.name)
            else:
                print(f"    Warning: Could not resolve type '{type_.name}'")
                self.file.write("u64")
        elif type(type_) == Struct:
            self.write_identifier(type_.name)
        elif type(type_) == Enum:
            self.write_identifier(type_.name)
        elif type(type_) == Union:
            self.write_identifier(type_.name)
        elif type(type_) == Function:
            self.file.write("func(")
            self.write_params(type_.params)
            self.file.write(")")

            if not (type(type_.result) == IdentifierType and type_.result.name == "void"):
                self.file.write(" -> ")
                self.write_type(type_.result)
        elif type(type_) == PtrType:
            self.write_ptr_type(type_)
        elif type(type_) == ArrayType:
            self.write_ptr_type(type_)

    def write_ptr_type(self, type_):
        if type(type_.base) is PrimitiveType:
            if type_.base.name == "void":
                self.file.write("addr")
                return
        elif type(type_.base) is IdentifierType:
            if not self.bindings.resolve_symbol(type_.base.name):
                self.file.write("addr")
                return
        elif type(type_.base) is Function:
            self.write_type(type_.base)
            return

        self.file.write("*")
        self.write_type(type_.base)

    def write_field_type(self, type_):
        if type(type_) != ArrayType:
            self.write_type(type_)
        else:
            self.file.write("[")
            self.write_type(type_.base)
            self.file.write("; ")
            self.file.write(str(type_.length))
            self.file.write("]")

    def resolve_type(self, type_):
        if type(type_) is not IdentifierType:
            return type_

        symbol = self.bindings.resolve_symbol(type_.name)
        if symbol and type(symbol) is TypeAlias:
            return self.resolve_type(symbol.type_)
        else:
            return type_

    def write_params(self, params):
        for i, param in enumerate(params):
            if type(param.type) == IdentifierType and param.type.name == "void":
                continue

            if param.name is not None and param.name != "":
                self.write_identifier(f"{utils.to_snake_case(param.name)}: ")

            self.write_type(param.type)

            if i != len(params) - 1:
                self.file.write(", ")

    def write_identifier(self, identifier):
        if len(identifier) == 0:
            identifier = "_"

        if identifier[0].isdigit():
            identifier = "_" + identifier

        if identifier in KEYWORDS:
            identifier += "_"

        self.file.write(identifier)
