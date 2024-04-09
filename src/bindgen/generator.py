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
    def __init__(self, struct: Structure):
        self.kind = "struct"
        self.name = struct.name


class FieldInfo:
    def __init__(self, field: Field, struct: Structure):
        self.kind = "field"
        self.name = field.name
        self.struct = StructInfo(struct)


class EnumInfo:
    def __init__(self, enum: Enumeration):
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
