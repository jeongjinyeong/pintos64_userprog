#!/usr/bin/env python3
"""Start a paused Pintos VM, capture its output, and check it after debugging."""

import argparse
import os
from pathlib import Path
import signal
import socket
import subprocess
import sys
import time


def stop_requested(signum, frame):
    raise SystemExit(0)


def check_result(root, build, test):
    test_path = Path("tests/threads") / test
    output = build / test_path.with_suffix(".output")
    result = build / test_path.with_suffix(".result")
    result.unlink(missing_ok=True)
    if not output.exists():
        print(f"[NOT RUN] {test}: no output from this debug session.", flush=True)
        return 0

    content = output.read_text(errors="replace")
    if "Powering off" not in content and not any(
        marker in content for marker in ("PANIC", "FAIL", "TIMEOUT")
    ):
        print(f"[STOPPED] {test}: debugging ended before the test finished.", flush=True)
        print(f"Output: {output}", flush=True)
        return 0

    completed = subprocess.run(
        ["perl", f"-I{root / 'pintos'}", str(root / 'pintos' / test_path.with_suffix('.ck')),
         str(test_path), str(test_path.with_suffix('.result'))],
        cwd=build,
        env={**os.environ, "PATH": f"{root / 'pintos/utils'}:{os.environ.get('PATH', '')}"},
    )
    if completed.returncode or not result.exists():
        print(f"[ERROR] {test}: the Pintos checker could not produce a result.", flush=True)
        return 1
    verdict = result.read_text().splitlines()[0]
    print(f"\n[{verdict}] {test}", flush=True)
    print(f"Result: {result}", flush=True)
    return 0 if verdict == "PASS" else 1


def start_debug(root, build, output_dir, test):
    with socket.socket() as probe:
        probe.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            probe.bind(("127.0.0.1", 1234))
        except OSError:
            sys.exit("GDB port 1234 is already in use. Stop the previous debug session.")

    signal.signal(signal.SIGTERM, stop_requested)
    signal.signal(signal.SIGINT, stop_requested)
    test_path = output_dir / "tests/threads" / test
    test_path.parent.mkdir(parents=True, exist_ok=True)
    # The standard panic checker looks for kernel.o in its working directory.
    kernel = output_dir / "kernel.o"
    if not kernel.exists() and not kernel.is_symlink():
        kernel.symlink_to("../kernel.o")
    test_path.with_suffix(".result").unlink(missing_ok=True)
    # QEMU writes straight to disk so the post-debug task can check the output
    # even if the debug adapter terminates this launcher as soon as QEMU exits.
    with test_path.with_suffix(".output").open("w") as output, \
            test_path.with_suffix(".errors").open("w") as errors:
        process = subprocess.Popen(
            [sys.executable, str(root / "pintos/utils/pintos"),
             "--gdb", "-m", "20", "--", "-q", "run", test],
            cwd=build,
            stdin=subprocess.DEVNULL,
            stdout=output,
            stderr=errors,
            start_new_session=True,
        )
    log = test_path.with_suffix(".output").open()

    def show_output():
        text = log.read()
        if text:
            print(text, end="", flush=True)

    try:
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            if process.poll() is not None:
                sys.exit("Pintos exited before its GDB server became ready.")
            try:
                with socket.create_connection(("127.0.0.1", 1234), timeout=0.2):
                    break
            except OSError:
                time.sleep(0.05)
        else:
            sys.exit("Timed out waiting for the Pintos GDB server.")
        print("Pintos GDB server ready", flush=True)
        while process.poll() is None:
            show_output()
            time.sleep(0.05)
        return process.returncode
    finally:
        if process.poll() is None:
            try:
                os.killpg(process.pid, signal.SIGINT)
            except ProcessLookupError:
                pass
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
        show_output()
        log.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Check the captured debug output")
    parser.add_argument("test", nargs="?", default="alarm-single")
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    build = root / "pintos/threads/build"
    output_dir = build / "debug"
    # Restrict the argument to a known threads test before creating output files.
    if "/" in args.test or "\\" in args.test or not (
        root / "pintos/tests/threads" / f"{args.test}.ck"
    ).is_file():
        parser.error(f"Unknown threads test: {args.test}")
    if args.check:
        return check_result(root, output_dir, args.test)
    return start_debug(root, build, output_dir, args.test)


if __name__ == "__main__":
    sys.exit(main())
