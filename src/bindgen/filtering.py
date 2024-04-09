from bindings import *
from generator import *


def filter_symbols(bindings: Bindings, generator: Generator):
    def filter_func(symbol): return filter_symbol(symbol, generator)
    bindings.symbols = list(filter(filter_func, bindings.symbols))


def filter_symbol(symbol, generator: Generator):
    if not symbol.name:
        return False

    if type(symbol) == Function:
        return generator.filter_symbol(FuncInfo(symbol))
    elif type(symbol) == Constant:
        return generator.filter_symbol(ConstInfo(symbol))
    elif type(symbol) == Structure:
        return generator.filter_symbol(StructInfo(symbol))
    elif type(symbol) == Enumeration:
        return generator.filter_symbol(EnumInfo(symbol))
    elif type(symbol) == TypeAlias:
        return generator.filter_symbol(TypeAliasInfo(symbol))

    return True
