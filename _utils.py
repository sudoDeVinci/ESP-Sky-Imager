from enum import Enum
from functools import lru_cache
from typing_extensions import Self
from typing import (
    Any,
    Optional
)
from re import compile


class HEADERS(Enum):
    """
    Expected Headers to the /api route from esp devices.
    """

    MACADDRESS = "X-MAC-Address"
    TIMESTAMP = "X-Timestamp"
    FIRMWAREVERSION = "X-Firmware-Version"
    CONTENTTYPE = "Content-Type"
    CONTENTLENGTH = "Content-Length"
    USERAGENT = "User-Agent"
    UNKNOWN = "UNKNOWN"

    @classmethod
    @lru_cache(maxsize=10)
    def match(cls: Self, header: Optional[str]) -> "HEADERS":
        """
        Match input string to header.
        """
        header = header.lower()
        for _, headertype in cls.__members__.items():
            if header == headertype.value:
                return headertype
        return cls.UNKNOWN

    @classmethod
    @lru_cache(maxsize=10)
    def __contains__(cls: Self, header: Optional[str]) -> bool:
        """
        Check if a header is supported.
        """
        return HEADERS.match(header) != cls.UNKNOWN

    @classmethod
    @lru_cache(maxsize=10)
    def _missing_(cls: Self, value: Any):
        """
        Handle missing headers.
        """
        return cls.UNKNOWN

    @classmethod
    @lru_cache(maxsize=1)
    def members(cls: Self) -> tuple[str]:
        """
        Return an iterable of headers.
        """
        return tuple(ctype for _, ctype in cls.__members__.items())

    @classmethod
    @lru_cache(maxsize=1)
    def names(cls: Self) -> tuple[str]:
        """
        Return a tuple of header names.
        """
        return tuple(names for names, _ in cls.__members__.items())
    

