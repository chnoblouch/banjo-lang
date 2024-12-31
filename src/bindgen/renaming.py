from bindings import *
from generator import Generator, get_symbol_info


def rename_symbols(bindings: Bindings, generator: Generator):
    type_name_map = {}

    for symbol in bindings.symbols:
        rename_symbol(symbol, type_name_map, generator)

    for symbol in bindings.symbols:
        rename_types(symbol, type_name_map)


def rename_symbol(symbol, type_name_map, generator: Generator):
    is_type = type(symbol) in (Struct, Union, Enum, TypeAlias)

    info = get_symbol_info(symbol)
    if info:
        generator.rename_symbol(info)

    if is_type:
        type_name_map[symbol.name] = info.name

    symbol.name = info.name

    if type(symbol) in (Struct, Union):
        for i in range(len(symbol.fields)):
            symbol.fields[i].name = info.fields[i].name
    elif type(symbol) == Enum:
        for i in range(len(symbol.variants)):
            symbol.variants[i].name = info.variants[i].name

    return True


def rename_types(node, type_name_map):
    if type(node) == Function:
        rename_types(node.result, type_name_map)

        for param in node.params:
            rename_types(param, type_name_map)
    elif type(node) == Param:
        rename_types(node.type, type_name_map)
    elif type(node) == Constant:
        rename_types(node.type, type_name_map)
    elif type(node) in (Struct, Union):
        for field in node.fields:
            rename_types(field, type_name_map)
    elif type(node) == Field:
        rename_types(node.type, type_name_map)
    elif type(node) == IdentifierType:
        node.name = type_name_map.get(node.name, node.name)
    elif type(node) == PtrType:
        rename_types(node.base, type_name_map)
    elif type(node) == ArrayType:
        rename_types(node.base, type_name_map)
