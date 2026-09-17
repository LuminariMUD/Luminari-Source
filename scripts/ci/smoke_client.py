#!/usr/bin/env python3
"""Drive a pre-authentication client conversation against a running server.

The exchange stays in front of the account password prompt, so it needs no
credentials and creates nothing: a new account name is offered and declined,
an invalid name is rejected, an over-long line is truncated, Telnet
negotiation is absorbed, and an empty name closes the connection. The server
must survive every step; the caller checks its health afterwards.

usage: smoke_client.py [--host HOST] [--port PORT] [--timeout SECONDS]
"""

from __future__ import annotations

import argparse
import socket
import sys
import time

IAC, WILL, WONT, DO, DONT = 255, 251, 252, 253, 254
TTYPE, NAWS, MSDP = 24, 31, 69


class Conversation:
    def __init__(self, host: str, port: int, timeout: float) -> None:
        self.sock = socket.create_connection((host, port), timeout=timeout)
        self.timeout = timeout
        self.transcript = bytearray()

    def expect(self, needle: bytes, description: str) -> None:
        """Read until needle appears in the newly received bytes."""
        deadline = time.monotonic() + self.timeout
        received = bytearray()
        while time.monotonic() < deadline:
            self.sock.settimeout(max(0.1, deadline - time.monotonic()))
            try:
                chunk = self.sock.recv(4096)
            except socket.timeout:
                continue
            if not chunk:
                break
            received += chunk
            self.transcript += chunk
            if needle.lower() in bytes(received).lower():
                return
        raise AssertionError(
            f"{description}: did not receive {needle!r}\n--- received ---\n"
            f"{bytes(received).decode('latin-1')}"
        )

    def expect_any(self, description: str) -> None:
        """Read until the server has sent something at all."""
        deadline = time.monotonic() + self.timeout
        while time.monotonic() < deadline:
            self.sock.settimeout(max(0.1, deadline - time.monotonic()))
            try:
                chunk = self.sock.recv(4096)
            except socket.timeout:
                continue
            if not chunk:
                break
            self.transcript += chunk
            return
        raise AssertionError(f"{description}: the server sent nothing")

    def expect_close(self, description: str) -> None:
        deadline = time.monotonic() + self.timeout
        while time.monotonic() < deadline:
            self.sock.settimeout(max(0.1, deadline - time.monotonic()))
            try:
                chunk = self.sock.recv(4096)
            except socket.timeout:
                continue
            except ConnectionError:
                return
            if not chunk:
                return
            self.transcript += chunk
        raise AssertionError(f"{description}: the server kept the connection open")

    def send(self, data: bytes) -> None:
        self.sock.sendall(data)


def run(host: str, port: int, timeout: float) -> int:
    conversation = Conversation(host, port, timeout)
    try:
        conversation.expect_any("greeting")
        # Telnet negotiation replies that the server offers for its protocols.
        conversation.send(bytes([IAC, WONT, TTYPE, IAC, DONT, MSDP, IAC, WILL, NAWS]))
        conversation.send(b"Sanitysmoke\r\n")
        conversation.expect(b"Did I get that right", "new account name")
        conversation.send(b"N\r\n")
        conversation.expect(b"what IS it", "declined name asks again")
        conversation.send(b"1!!\r\n")
        conversation.expect(b"Invalid account name", "invalid characters in a name")
        conversation.send(b"A" * 600 + b"\r\n")
        conversation.expect(b"Line too long", "over-long input line is truncated")
        conversation.expect(b"name", "over-long name is rejected")
        conversation.send(b"\r\n")
        conversation.expect_close("empty name closes the connection")
    except (AssertionError, OSError) as error:
        sys.stderr.write(f"smoke client: {error}\n")
        sys.stderr.write("--- transcript ---\n")
        sys.stderr.write(bytes(conversation.transcript).decode("latin-1"))
        sys.stderr.write("\n")
        return 1
    finally:
        conversation.sock.close()
    print("Pre-authentication client interaction PASSED")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n", 1)[0])
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=4100)
    parser.add_argument("--timeout", type=float, default=20.0)
    args = parser.parse_args()
    return run(args.host, args.port, args.timeout)


if __name__ == "__main__":
    sys.exit(main())
