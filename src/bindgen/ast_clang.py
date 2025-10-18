from clang import cindex
from bindings import *


def parse(filename, include_paths):
    index = cindex.Index.create()
    args = [f"-I{path}" for path in include_paths]
    tu = index.parse(filename, args, options=cindex.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD)
    return tu


class ASTConverter:
    PRIMITIVE_TYPES = {
        cindex.TypeKind.VOID: "void",
        cindex.TypeKind.BOOL: "u8",
        cindex.TypeKind.CHAR_U: "u8",
        cindex.TypeKind.UCHAR: "u8",
        cindex.TypeKind.CHAR16: "u16",
        cindex.TypeKind.CHAR32: "u32",
        cindex.TypeKind.USHORT: "u16",
        cindex.TypeKind.UINT: "u32",
        cindex.TypeKind.ULONG: "u32", # TODO: depends on OS
        cindex.TypeKind.ULONGLONG: "u64",
        cindex.TypeKind.CHAR_S: "u8",
        cindex.TypeKind.SHORT: "i16",
        cindex.TypeKind.INT: "i32",
        cindex.TypeKind.LONG: "i32", # TODO: depends on OS
        cindex.TypeKind.LONGLONG: "i64",
        cindex.TypeKind.FLOAT: "f32",
        cindex.TypeKind.DOUBLE: "f64",
    }

    def __init__(self, tu, generator):
        self.tu = tu
        self.generator = generator
        # self.print_rec(unit.cursor, 0)

    def print_rec(self, cursor, indent):
        print("  " * indent + cursor.kind.name)

        for child in cursor.get_children():
            self.print_rec(child, indent + 1)

    def generate(self) -> Bindings:
        symbols = {}

        for child in self.tu.cursor.get_children():
            file = child.location.file

            if file and not self.generator.filter_file_path(file.name):
                continue

            if child.kind == cindex.CursorKind.FUNCTION_DECL:
                symbol = self.gen_func(child)
                symbols[symbol.name] = symbol
            elif child.kind == cindex.CursorKind.VAR_DECL:
                symbol = self.gen_const(child)
                symbols[symbol.name] = symbol
            elif child.kind == cindex.CursorKind.STRUCT_DECL:
                symbol = self.gen_struct(child)
                symbols[symbol.name] = symbol
            elif child.kind == cindex.CursorKind.UNION_DECL:
                symbol = self.gen_union(child)
                symbols[symbol.name] = symbol
            elif child.kind == cindex.CursorKind.ENUM_DECL:
                symbol = self.gen_enum(child)
                symbols[symbol.name] = symbol
            elif child.kind == cindex.CursorKind.TYPEDEF_DECL:
                if child.underlying_typedef_type.kind == cindex.TypeKind.ELABORATED:
                    decl = child.underlying_typedef_type.get_declaration()

                    if decl.kind == cindex.CursorKind.ENUM_DECL:
                        symbol = self.gen_enum(decl)
                        symbols[symbol.name] = symbol

                # symbol = self.gen_type_alias(child)
                # if symbol:
                #     symbols[symbol.name] = symbol
            elif child.kind == cindex.CursorKind.MACRO_DEFINITION:
                symbol = self.gen_macro(child)
                if symbol:
                    symbols[symbol.name] = symbol

        return Bindings(symbols.values())

    def gen_func(self, cursor):
        param_names = self.collect_param_names(cursor)

        name = cursor.spelling
        result = self.gen_type(cursor.result_type, param_names)

        params = []
        for child in cursor.get_arguments():
            param = self.gen_param(child.type, param_names)
            params.append(param)

        return Function(name, name, params, result)

    def gen_const(self, cursor):
        param_names = self.collect_param_names(cursor)
        
        name = cursor.spelling
        type_ = self.gen_type(cursor.type, param_names)

        value = ""

        in_value = False
        for token in cursor.get_tokens():
            if in_value:
                value = token.spelling
            elif token.spelling == "=":
                in_value = True

        value = value.lower().replace("u", "").replace("l", "")

        return Constant(name, type_, value)

    def gen_macro(self, cursor):
        tokens = list(cursor.get_tokens())
        if len(tokens) < 2:
            return None

        name = tokens[0].spelling

        if tokens[1].kind == cindex.TokenKind.LITERAL:
            value = tokens[1].spelling
            value = value.lower().replace("u", "").replace("l", "")

            type_ = PrimitiveType("i64")
            if value[0] == "\"":
                type_ = PtrType(PrimitiveType("u8"))

            return Constant(name, type_, value)

        return None

    def gen_struct(self, cursor):
        name = cursor.spelling
        fields = []

        for i, field in enumerate(cursor.type.get_fields()):
            param_names = self.collect_param_names(field)
            field_name = field.spelling

            # FIXME: Thie needs a better solution.
            if "anonymous " in field_name:
                field_name = f"field{i}"

            field_type = self.gen_type(field.type, param_names)
            fields.append(Field(field_name, field_type))

        return Struct(name, fields)

    def gen_union(self, cursor):
        name = cursor.spelling
        fields = []

        for i, field in enumerate(cursor.type.get_fields()):
            param_names = self.collect_param_names(field)
            field_name = field.spelling

            # FIXME: Thie needs a better solution.
            if "anonymous " in field_name:
                field_name = f"field{i}"

            field_type = self.gen_type(field.type, param_names)
            fields.append(Field(field_name, field_type))

        return Union(name, fields)

    def gen_enum(self, cursor):
        name = cursor.spelling
        variants = []

        for child in cursor.get_children():
            if child.kind != cindex.CursorKind.ENUM_CONSTANT_DECL:
                continue
            
            variant_name = child.spelling
            variant_value = child.enum_value
            variants.append(EnumVariant(variant_name, variant_value))

        return Enum(name, variants)

    def gen_type_alias(self, cursor):
        first_child = next(cursor.get_children(), None)

        if not first_child or first_child.kind != cindex.CursorKind.TYPE_REF:
            return None

        name = cursor.spelling

        param_names = self.collect_param_names(cursor)
        type_ = self.gen_type(cursor.underlying_typedef_type, param_names)
        
        return TypeAlias(name, type_)

    def collect_param_names(self, cursor):
        names = []

        for child in cursor.get_children():
            if child.kind == cindex.CursorKind.PARM_DECL:
                names.append(child.spelling)
                names.extend(self.collect_param_names(child))

        return names

    def gen_param(self, cursor_type, param_names):
        if param_names:
            name = param_names[0]
            param_names.pop(0)
        else:
            name = None

        type_ = self.gen_type(cursor_type, param_names)

        return Param(name, type_)

    def gen_type(self, type_, param_names):
        type_ = type_.get_canonical()

        if type_.kind in (cindex.TypeKind.RECORD, cindex.TypeKind.ENUM, cindex.TypeKind.TYPEDEF):
            return IdentifierType(type_.get_declaration().spelling)
        elif type_.kind == cindex.TypeKind.POINTER:
            pointee_clang = type_.get_pointee()

            if pointee_clang.kind == cindex.TypeKind.FUNCTIONPROTO:
                pointee = self.gen_func_type(pointee_clang, param_names)
            else:
                pointee = self.gen_type(pointee_clang, param_names)

            return PtrType(pointee)
        elif type_.kind == cindex.TypeKind.CONSTANTARRAY:
            base = self.gen_type(type_.element_type, param_names)
            length = type_.element_count
            return ArrayType(base, length)
        elif type_.kind in ASTConverter.PRIMITIVE_TYPES:
            return PrimitiveType(ASTConverter.PRIMITIVE_TYPES[type_.kind])

        return IdentifierType("i64")

    def gen_func_type(self, type_, param_names):
        result = self.gen_type(type_.get_result(), param_names)

        params = []
        for arg in type_.argument_types():
            params.append(self.gen_param(arg, param_names))

        return Function(None, None, params, result)
