#!/usr/bin/env python3
"""TCP peer for xemu's -serialtcp in test_xemu.sh's serial test.

Listens on 127.0.0.1 (a free port, written to PORTFILE), takes one
connection and sends back each byte it receives XOR $55, so the program
can tell the echo from its own bytes. Ends when the peer closes, or after
TIMEOUT seconds.
"""
import socket
import sys

def main():
    portfile = sys.argv[1]
    timeout = float(sys.argv[2]) if len(sys.argv) > 2 else 30
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.bind(("127.0.0.1", 0))
    srv.listen(1)
    srv.settimeout(timeout)
    with open(portfile + ".tmp", "w") as f:
        f.write(str(srv.getsockname()[1]))
    import os
    os.replace(portfile + ".tmp", portfile)
    try:
        conn, _ = srv.accept()
    except socket.timeout:
        return
    conn.settimeout(timeout)
    try:
        while True:
            data = conn.recv(256)
            if not data:
                break
            conn.sendall(bytes(b ^ 0x55 for b in data))
    except (socket.timeout, OSError):
        pass
    conn.close()

if __name__ == "__main__":
    main()
