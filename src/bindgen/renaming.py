from bindings import *
from generator import Generator, SymbolInfo


def rename_symbols(bindings: Bindings, generator: Generator):
    type_name_map = {}

    for symbol in bindings.symbols:
        rename_in_node(symbol, type_name_map, generator)

    for symbol in bindings.symbols:
        update_identifiers(symbol, type_name_map)


def rename_in_node(node, type_name_map, generator: Generator):
    if type(node) in (Function, Param, Constant, Struct, Union, Field, Enum, EnumVariant, TypeAlias):
        if type(node) == Function:
            for param in node.params:
                rename_in_node(param, type_name_map, generator)
        elif type(node) == Param:
            rename_in_node(node.type, type_name_map, generator)
        elif type(node) in (Struct, Union):
            for field in node.fields:
                rename_in_node(field, type_name_map, generator)
        elif type(node) == Enum:
            enum_common_prefix_len = common_prefix_len(node.variants, generator)

            for variant in node.variants:
                rename_symbol(variant, type_name_map, generator, node.name, enum_common_prefix_len)
        elif type(node) == Field:
            rename_in_node(node.type, type_name_map, generator)

        if node.name is not None:
            rename_symbol(node, type_name_map, generator)
    elif type(node) == PtrType:
        rename_in_node(node.base, type_name_map, generator)
    elif type(node) == ArrayType:
        rename_in_node(node.base, type_name_map, generator)


def rename_symbol(
    symbol,
    type_name_map,
    generator: Generator,
    parent_enum_name=None,
    enum_common_prefix_len=None,
):
    is_type = type(symbol) in (Struct, Union, Enum, TypeAlias)
    
    info = SymbolInfo(symbol.name, symbol.kind)
    info.parent_enum_name = parent_enum_name
    info.enum_common_prefix_len = enum_common_prefix_len

    new_name = generator.rename_symbol(info)

    if new_name is not None:
        if is_type:
            type_name_map[symbol.name] = new_name

        symbol.name = new_name

    return True


def rename_params(params, type_name_map, generator: Generator):
    for param in params:
        rename_symbol(param, type_name_map, generator)


def update_identifiers(node, type_name_map):
    if type(node) == IdentifierType:
        node.name = type_name_map.get(node.name, node.name)
    elif type(node) == Function:
        update_identifiers(node.result, type_name_map)

        for param in node.params:
            update_identifiers(param, type_name_map)
    elif type(node) == Param:
        update_identifiers(node.type, type_name_map)
    elif type(node) == Constant:
        update_identifiers(node.type, type_name_map)
    elif type(node) in (Struct, Union):
        for field in node.fields:
            update_identifiers(field, type_name_map)
    elif type(node) == Field:
        update_identifiers(node.type, type_name_map)
    elif type(node) == PtrType:
        update_identifiers(node.base, type_name_map)
    elif type(node) == ArrayType:
        update_identifiers(node.base, type_name_map)


def common_prefix_len(variants, generator: Generator):
    strings = generator.enum_variants_with_common_prefix([v.name for v in variants])

    if not strings:
        return 0

    for i in range(min([len(s) for s in strings])):
        c = strings[0][i]
        for s in strings:
            if s[i] != c:
                return i
    return i
