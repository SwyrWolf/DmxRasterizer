#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
import subprocess
from pathlib import Path


PROGRAM_EXE = "dmxrasterizer.exe"
BUILD_DIR = Path("build")
EXE_PATH = BUILD_DIR / PROGRAM_EXE


def run(cmd: list[str], *, cwd: Path | None = None) -> None:
	"""Run a command and fail fast if it errors."""
	print("+", " ".join(cmd))
	subprocess.run(cmd, cwd=str(cwd) if cwd else None, check=True)


def ensure_in_path(tool: str) -> None:
	if shutil.which(tool) is None:
		raise SystemExit(f"Required tool not found on PATH: {tool}")


def cmake_configure(build_type: str, *, use_clang: bool = False) -> None:
	ensure_in_path("cmake")
	ensure_in_path("ninja")

	cmd = [
		"cmake",
		"-G", "Ninja",
		"-B", str(BUILD_DIR),
		f"-DCMAKE_BUILD_TYPE={build_type}",
	]

	if use_clang:
		cmd.extend([
			"-DCMAKE_C_COMPILER=clang",
			"-DCMAKE_CXX_COMPILER=clang++",
		])

	run(cmd)


def ninja_build() -> None:
	ensure_in_path("ninja")
	run(["ninja", "-C", str(BUILD_DIR)])


def run_program() -> None:
	if not EXE_PATH.exists():
		raise SystemExit(f"Executable not found: {EXE_PATH} (build first)")

	# Equivalent to: & $exePath
	run([str(EXE_PATH)])


def run_lldb() -> None:
	ensure_in_path("lldb")

	if not EXE_PATH.exists():
		raise SystemExit(f"Executable not found: {EXE_PATH} (build first)")

	# Equivalent to: lldb $exePath
	run(["lldb", str(EXE_PATH)])


def parse_args() -> argparse.Namespace:
	p = argparse.ArgumentParser(description="Build/run helper for dmxrasterizer")
	g = p.add_mutually_exclusive_group(required=True)

	g.add_argument("--release", action="store_true", help="Configure + build Release")
	g.add_argument("--debug", action="store_true", help="Configure + build Debug")
	g.add_argument("--rundb", action="store_true", help="Run under LLDB")
	g.add_argument("--run", action="store_true", help="Run the executable")
	g.add_argument("--setup", action="store_true", help="Configure Debug using clang/clang++ (no build)")

	return p.parse_args()


def main() -> int:
	args = parse_args()

	try:
		if args.release:
			print("Generating Release Build!")
			cmake_configure("Release")
			ninja_build()
			return 0

		if args.debug:
			print("Generating Debug Build!")
			cmake_configure("Debug")
			ninja_build()
			return 0

		if args.rundb:
			print("Running Program (LLDB)")
			run_lldb()
			return 0

		if args.run:
			print("Running Program")
			run_program()
			return 0

		if args.setup:
			print("Generating Debug Build with clang Setup!")
			cmake_configure("Debug", use_clang=True)
			return 0

	except subprocess.CalledProcessError as e:
		return e.returncode
	except KeyboardInterrupt:
		return 130

	return 0


if __name__ == "__main__":
	raise SystemExit(main())