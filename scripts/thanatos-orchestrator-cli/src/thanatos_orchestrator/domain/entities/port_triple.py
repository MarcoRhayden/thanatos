from __future__ import annotations

from dataclasses import dataclass
from typing import List


@dataclass(frozen=True, slots=True)
class PortTriple:
    """A set of 3 ports used by a Thanatos instance."""

    config_index: int
    login: int
    char: int
    query: int

    def as_list(self) -> List[int]:
        return [int(self.login), int(self.char), int(self.query)]
