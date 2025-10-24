

from enum import IntEnum

__version__: str

class Status(IntEnum):

    GREEN: int
    YELLOW: int
    RED: int
    UNKNOWN: int

class StatusTree:


    def __init__(self) -> None:

        ...

    def load_config(self, config_file: str) -> None:

        ...

    def set_status(self, node_name: str, status: Status) -> None:

        ...

    def compute(self) -> None:

        ...

    def get_status(self, node_name: str) -> Status | None:

        ...

    def print_statuses(self) -> None:

        ...

def string_to_status(s: str) -> Status:

    ...

def status_to_string(status: Status) -> str:

    ...
