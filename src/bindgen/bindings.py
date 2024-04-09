from dataclasses import dataclass
import typing

Type = typing.Union[
    "PrimitiveType",
    "IdentifierType",
    "PtrType",
    "ArrayType",
    "Structure",
    "Enumeration"
]


@dataclass
class PrimitiveType:
    name: str


@dataclass
class IdentifierType:
    name: str


@dataclass
class PtrType:
    base: Type


@dataclass
class ArrayType:
    base: Type
    length: int


@dataclass
class Param:
    name: typing.Optional[str]
    type: Type


@dataclass
class Function:
    kind = "func"
    name: str
    original_name: str
    params: typing.List[Param]
    result: Type


@dataclass
class Constant:
    kind = "const"
    name: str
    type: Type
    value: typing.Any


@dataclass
class Field:
    kind = "field"
    name: str
    type: Type


@dataclass
class Structure:
    kind = "struct"
    name: str
    fields: list[Field]


@dataclass
class EnumVariant:
    kind = "enum_variant"
    name: str
    value: typing.Optional[int]


@dataclass
class Enumeration:
    kind = "enum"
    name: str
    variants: list[EnumVariant]


@dataclass
class Union:
    kind = "union"
    name: str


@dataclass
class TypeAlias:
    kind = "type_alias"
    name: str
    type_: Type


@dataclass
class Bindings:
    symbols: typing.List[
        typing.Union[
            Function,
            Structure,
            Enumeration,
            TypeAlias
        ]
    ]

    def resolve_symbol(self, name):
        for symbol in self.symbols:
            if symbol and symbol.name == name:
                return symbol

        return None
