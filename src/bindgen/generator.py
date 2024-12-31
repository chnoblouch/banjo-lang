from bindings import *


class FuncInfo:
    def __init__(self, func: Function):
        self.kind = "func"
        self.name = func.name


class ConstInfo:
    def __init__(self, const: Constant):
        self.kind = "const"
        self.name = const.name


class StructInfo:
    def __init__(self, struct: Struct):
        self.kind = "struct"
        self.name = struct.name
        self.fields = [FieldInfo(field) for field in struct.fields]


class UnionInfo:
    def __init__(self, union: Union):
        self.kind = "union"
        self.name = union.name
        self.fields = [FieldInfo(field) for field in union.fields]


class FieldInfo:
    def __init__(self, field: Field):
        self.kind = "field"
        self.name = field.name


class EnumInfo:
    def __init__(self, enum: Enum):
        self.kind = "enum"
        self.name = enum.name
        self.variants = [EnumVariantInfo(variant) for variant in enum.variants]

        self.new_name = self.name


class EnumVariantInfo:
    def __init__(self, enum_variant: EnumVariant):
        self.kind = "enum_variant"
        self.name = enum_variant.name

        self.new_name = self.name


class TypeAliasInfo:
    def __init__(self, type_alias: TypeAlias):
        self.kind = "type_alias"
        self.name = type_alias.name


class Generator:
    def __init__(self):
        self.filter_file_path = None
        self.filter_symbol = None
        self.rename_symbol = None


def get_symbol_info(symbol):
    if type(symbol) == Function:
        return FuncInfo(symbol)
    elif type(symbol) == Constant:
        return ConstInfo(symbol)
    elif type(symbol) == Struct:
        return StructInfo(symbol)
    elif type(symbol) == Union:
        return UnionInfo(symbol)
    elif type(symbol) == Enum:
        return EnumInfo(symbol)
    elif type(symbol) == TypeAlias:
        return TypeAliasInfo(symbol)
    else:
        return None