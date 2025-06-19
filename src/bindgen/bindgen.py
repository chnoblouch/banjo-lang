from ast_clang import parse, ASTConverter
import filtering
import renaming
from writer import Writer
from generator import Generator

import importlib.util
import sys
from argparse import ArgumentParser


def generate(filename, generator, include_paths):
    print(f"Generating bindings for {filename}...")
    
    print("  Parsing file...")
    ast = parse(filename, include_paths)

    print("  Generating bindings...")
    bindings = ASTConverter(ast, generator).generate()

    print("  Filtering symbols...")
    filtering.filter_symbols(bindings, generator)

    print("  Renaming symbols...")
    renaming.rename_symbols(bindings, generator)

    print("  Writing bindings...")
    Writer(bindings, "bindings.bnj").write()


def run(source_file_path, generator_path, include_paths):
    symbols = {}

    if generator_path:
        dont_write_bytecode = sys.dont_write_bytecode
        sys.dont_write_bytecode = True

        spec = importlib.util.spec_from_file_location("bindgen_generator", generator_path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        symbols = module.__dict__
        
        sys.dont_write_bytecode = dont_write_bytecode

    generator = Generator()
    generator.filter_file_path = symbols.get("filter_file_path", lambda path: True)
    generator.filter_symbol = symbols.get("filter_symbol", lambda symbol: True)
    generator.rename_symbol = symbols.get("rename_symbol", lambda symbol: symbol.name)

    generate(source_file_path, generator, include_paths)


if __name__ == "__main__":
    parser = ArgumentParser("banjo-bindgen")
    parser.add_argument("--generator")
    parser.add_argument("-I", action="append", default=[], dest="include_paths")
    parser.add_argument("file")
    args = parser.parse_args()

    run(args.file, args.generator, args.include_paths)
