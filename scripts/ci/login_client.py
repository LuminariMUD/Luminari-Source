#!/usr/bin/env python3
"""Drive a running server through account creation and login over telnet.

Used by scripts/ci/test_server_startup.sh against the isolated test runtime.
The first connection creates the account, the second logs in to it after one
wrong password, so the account must have been saved to and loaded from the
database. Each step waits for the prompt the server sends before answering, and
any missing prompt fails with the text received so far.

Usage: login_client.py --port 4100 --account NAME --password TEXT
"""

import argparse
import socket
import sys
import time

IAC, SB, SE = 255, 250, 240
WILL, WONT, DO, DONT = 251, 252, 253, 254
ESC = 27


class Session:
    def __init__(self, port, timeout):
        self.timeout = timeout
        self.sock = socket.create_connection(("127.0.0.1", port), timeout=timeout)
        self.plain = bytearray()
        self.state = "text"
        self.seen = 0

    def feed(self, data):
        """Keep the text, dropping telnet negotiation and ANSI escape sequences.

        The state survives between reads, so a sequence split across two reads
        is still removed whole and the text already searched never shifts.
        """
        for byte in data:
            if self.state == "text":
                if byte == IAC:
                    self.state = "iac"
                elif byte == ESC:
                    self.state = "escape"
                else:
                    self.plain.append(byte)
            elif self.state == "iac":
                self.state = {WILL: "option", WONT: "option", DO: "option", DONT: "option"}.get(
                    byte, "subnegotiation" if byte == SB else "text"
                )
            elif self.state == "option":
                self.state = "text"
            elif self.state == "subnegotiation":
                self.state = "subnegotiation-iac" if byte == IAC else "subnegotiation"
            elif self.state == "subnegotiation-iac":
                self.state = "text" if byte == SE else "subnegotiation"
            elif self.state == "escape":
                self.state = "csi" if byte == ord("[") else "text"
            elif self.state == "csi" and (0x40 <= byte <= 0x7E):
                self.state = "text"

    def expect(self, marker):
        deadline = time.monotonic() + self.timeout
        while True:
            text = self.plain.decode("ascii", "replace")
            found = text.find(marker, self.seen)
            if found >= 0:
                self.seen = found + len(marker)
                return
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise SystemExit(f"timed out waiting for {marker!r}; received:\n{text}")
            self.sock.settimeout(remaining)
            try:
                chunk = self.sock.recv(4096)
            except socket.timeout:
                continue
            except OSError as error:
                raise SystemExit(f"connection reset before {marker!r}: {error}; received:\n{text}")
            if not chunk:
                raise SystemExit(f"connection closed before {marker!r}; received:\n{text}")
            self.feed(chunk)

    def send(self, line):
        self.sock.sendall(line.encode("ascii") + b"\r\n")

    def expect_close(self):
        deadline = time.monotonic() + self.timeout
        while time.monotonic() < deadline:
            self.sock.settimeout(max(deadline - time.monotonic(), 0.01))
            try:
                if not self.sock.recv(4096):
                    self.sock.close()
                    return
            except socket.timeout:
                continue
            except OSError:
                # A reset closes the connection as surely as an orderly shutdown.
                self.sock.close()
                return
        raise SystemExit("the server did not close the connection after Quit")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--account", required=True)
    parser.add_argument("--password", required=True)
    parser.add_argument("--timeout", type=float, default=20.0)
    args = parser.parse_args()

    # The protocol report precedes the greeting once negotiation finishes.
    create = Session(args.port, args.timeout)
    create.expect("GMCP")
    create.send(args.account)
    create.expect(f"Did I get that right, {args.account} (Y/N)?")
    create.send("Y")
    create.expect("New Account.")
    create.expect("Give me a Password:")
    create.send(args.password)
    create.expect("Please retype password:")
    create.send(args.password)
    create.expect("Your choice :")
    create.send("Q")
    create.expect("Quitting.")
    create.expect_close()
    print(f"created account {args.account}")

    login = Session(args.port, args.timeout)
    login.expect("GMCP")
    login.send(args.account)
    login.expect("Password:")
    login.send(args.password + "-wrong")
    login.expect("Wrong password.")
    login.expect("Password:")
    login.send(args.password)
    login.expect("Your choice :")
    login.send("Q")
    login.expect("Quitting.")
    login.expect_close()
    print(f"logged in to account {args.account} after rejecting a wrong password")
    return 0


if __name__ == "__main__":
    sys.exit(main())
