from bindings import *
from typing import Callable, List, Optional


@dataclass
class SymbolInfo:
    name: str
    kind: str
    parent_enum_name: Optional[str] = None
    enum_common_prefix_len: Optional[int] = None


class Generator:

    def __init__(self):
        self.filter_file_path: Callable[[str], bool] = lambda _: True
        self.filter_symbol: Callable[[SymbolInfo], bool] = lambda _: True
        self.rename_symbol: Callable[[SymbolInfo], str] = lambda _: None
        self.enum_variants_with_common_prefix: Callable[List[str], List[str]] = lambda variants: variants

