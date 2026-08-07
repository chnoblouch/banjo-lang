from bindings import *
from generator import Generator


def filter_symbols(bindings: Bindings, generator: Generator):
    def filter_func(symbol): return filter_symbol(symbol, generator)
    bindings.symbols = list(filter(filter_func, bindings.symbols))


def filter_symbol(symbol, generator: Generator):
    if not symbol.name:
        return False

    return generator.filter_symbol(symbol)
