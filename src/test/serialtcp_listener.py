#!/usr/bin/env python3
"""
MEGA65 Test Listener via serialtcp

Acts as TCP server that xemu connects to.
Captures serial output and validates test results.

Usage:
    ./serialtcp_listener.py --port 27513 --timeout 30 --test array_init

Environment:
    xemu-xmega65 runs with: -serialtcp 127.0.0.1:27513
    This script starts first, then xemu connects to it.
"""

import socket
import sys
import time
import argparse

class SerialTcpListener:
    def __init__(self, host='127.0.0.1', port=27513, timeout=30.0):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.server_sock = None
        self.client_sock = None
        self.buffer = ""
        self.lines = []

    def listen(self):
        """Create TCP server and wait for xemu connection"""
        try:
            self.server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_sock.bind((self.host, self.port))
            self.server_sock.listen(1)
            print(f"✓ Listening for xemu on [{self.host}]:{self.port}")

            # Wait for xemu to connect
            self.server_sock.settimeout(self.timeout + 5)
            try:
                self.client_sock, addr = self.server_sock.accept()
                self.client_sock.settimeout(self.timeout)
                print(f"✓ Accepted connection from {addr}")
                return True
            except socket.timeout:
                print(f"✗ Timeout waiting for xemu connection after {self.timeout+5}s")
                return False

        except Exception as e:
            print(f"✗ Failed to setup listener: {e}")
            import traceback
            traceback.print_exc()
            return False

    def read_until_completion(self):
        """Read output until test completes"""
        start_time = time.time()

        try:
            while True:
                try:
                    data = self.client_sock.recv(4096)
                    if not data:
                        print("Connection closed by xemu")
                        break

                    text = data.decode('utf-8', errors='replace')
                    self.buffer += text

                    # Extract complete lines
                    while '\n' in self.buffer:
                        line, self.buffer = self.buffer.split('\n', 1)
                        line = line.rstrip('\r')
                        self.lines.append(line)

                        # Check for test completion
                        if 'RESULT:' in line:
                            elapsed = time.time() - start_time
                            print(f"✓ Test completed in {elapsed:.2f}s")
                            return True

                except socket.timeout:
                    elapsed = time.time() - start_time
                    if elapsed > self.timeout:
                        print(f"✗ Timeout after {self.timeout}s")
                        return False

        except Exception as e:
            print(f"✗ Read error: {e}")
            return False

        return False

    def get_output(self):
        """Get all collected output"""
        return '\n'.join(self.lines)

    def validate_array_init_test(self):
        """Validate array initialization test results"""
        output = self.get_output()
        failures = []

        expected = {
            'char_init': '[10 20 30 40 50]',
            'char_partial': '[AA BB 00 00 00 00]',
            'int_init': '[03E8 07D0 0BB8 0FA0]',
            'int_zero': '[0000 0000 0000]',
        }

        print("\nValidating array initialization test:")
        print("-" * 50)

        for name, pattern in expected.items():
            if pattern in output:
                print(f"  ✓ {name:20s} {pattern}")
            else:
                print(f"  ✗ {name:20s} NOT FOUND")
                failures.append(name)

        passes = output.count('[PASS]')
        fails = output.count('[FAIL]')

        print(f"\n  Test assertions: {passes} passed, {fails} failed")

        if fails > 0:
            failures.append(f"assertions_failed: {fails}")

        if 'ALL TESTS PASSED' in output:
            print("  ✓ Final result: ALL TESTS PASSED")
        else:
            print("  ✗ Final result: NOT PASSED")
            failures.append("final_result")

        return len(failures) == 0, failures

    def close(self):
        """Close connections"""
        if self.client_sock:
            self.client_sock.close()
        if self.server_sock:
            self.server_sock.close()

    def print_output(self):
        """Print captured output"""
        print("\n" + "=" * 60)
        print("CAPTURED OUTPUT:")
        print("=" * 60)
        output = self.get_output()
        if output:
            print(output)
        else:
            print("(no output captured)")
        print("=" * 60 + "\n")


def run_test_with_listener(port, timeout, test_type):
    """Run test and validate via serialtcp"""
    listener = SerialTcpListener(port=port, timeout=timeout)

    if not listener.listen():
        return False

    print(f"Waiting for test output (timeout: {timeout}s)...")

    if not listener.read_until_completion():
        listener.print_output()
        listener.close()
        return False

    # Validate based on test type
    if test_type == 'array_init':
        success, failures = listener.validate_array_init_test()
    else:
        print(f"Unknown test type: {test_type}")
        success = False
        failures = [test_type]

    listener.print_output()
    listener.close()

    if success:
        print(f"✅ {test_type} test PASSED\n")
    else:
        print(f"❌ {test_type} test FAILED: {', '.join(failures)}\n")

    return success


def main():
    parser = argparse.ArgumentParser(
        description='MEGA65 Test Listener via serialtcp'
    )
    parser.add_argument('--host', default='127.0.0.1',
                        help='Host to listen on (default: 127.0.0.1)')
    parser.add_argument('--port', type=int, default=27513,
                        help='Port to listen on (default: 27513)')
    parser.add_argument('--timeout', type=float, default=30,
                        help='Timeout in seconds (default: 30)')
    parser.add_argument('--test', default='array_init',
                        help='Test type (array_init, etc.)')

    args = parser.parse_args()

    success = run_test_with_listener(args.port, args.timeout, args.test)
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
