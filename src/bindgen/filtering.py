from bindings import *
from generator import Generator, get_symbol_info


def filter_symbols(bindings: Bindings, generator: Generator):
    def filter_func(symbol): return filter_symbol(symbol, generator)
    bindings.symbols = list(filter(filter_func, bindings.symbols))


def filter_symbol(symbol, generator: Generator):
    if not symbol.name:
        return False

    info = get_symbol_info(symbol)
    return generator.filter_symbol(info) if info else True
