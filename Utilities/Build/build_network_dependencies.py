#!/usr/bin/env python3
# Copyright (c) Theophilus Eriata. All Rights Reserved.

"""Build the hash-pinned private OpenSSL/MsQuic providers and stage SDK runtime files."""
from __future__ import annotations

import argparse
import contextlib
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import time
import urllib.request
import zipfile

PACKAGES = {
    "openssl": ("openssl-3.5.9.tar.gz", "https://github.com/openssl/openssl/releases/download/openssl-3.5.9/openssl-3.5.9.tar.gz", "603f5602e2eef00d77fbd429d34dcd5822bb301757a1bc9cdb24c670f1eb859a"),
    "msquic": ("msquic-2.6.4.tar.gz", "https://codeload.github.com/microsoft/msquic/tar.gz/4504c53564948c8075d6854a08c77e574c5cd3f9", "83a76b8d3c552bfe3cc84780b2ac6ac322ab1d18a26ca87871b61ac531068c67"),
    "nasm": ("nasm-2.16.03-win64.zip", "https://www.nasm.us/pub/nasm/releasebuilds/2.16.03/win64/nasm-2.16.03-win64.zip", "3ee4782247bcb874378d02f7eab4e294a84d3d15f3f6ee2de2f47a46aa7226e6"),
}


def digest(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def source(package: str, cache: Path, root: Path) -> Path:
    name, url, expected = PACKAGES[package]
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / name
    if not archive.exists():
        temporary = cache / (name + f".{os.getpid()}.tmp")
        try:
            with urllib.request.urlopen(url, timeout=120) as response, temporary.open("wb") as output:
                shutil.copyfileobj(response, output)
            if digest(temporary) != expected:
                raise RuntimeError(f"Checksum mismatch for {name}")
            os.replace(temporary, archive)
        finally:
            temporary.unlink(missing_ok=True)
    if digest(archive) != expected:
        raise RuntimeError(f"Checksum mismatch for cached {name}; remove it before retrying")
    destination = root / (package + "-" + expected[:16])
    if not destination.is_dir():
        staging = root / (package + f"-extract-{os.getpid()}")
        staging.mkdir(parents=True, exist_ok=False)
        try:
            if archive.suffix == ".zip":
                with zipfile.ZipFile(archive) as stream:
                    for member in stream.infolist():
                        if not (staging / member.filename).resolve().is_relative_to(staging.resolve()):
                            raise RuntimeError("Unsafe archive path")
                    stream.extractall(staging)
            else:
                with tarfile.open(archive) as stream:
                    stream.extractall(staging, filter="data")
            entries = list(staging.iterdir())
            if len(entries) != 1 or not entries[0].is_dir():
                raise RuntimeError(f"Unexpected source layout for {name}")
            os.replace(entries[0], destination)
        finally:
            shutil.rmtree(staging, ignore_errors=True)
    return destination


@contextlib.contextmanager
def build_lock(path: Path):
    with path.open("a+b") as stream:
        stream.seek(0)
        stream.write(b"\0")
        stream.flush()
        if os.name == "nt":
            import msvcrt
            deadline = time.monotonic() + 1800
            while True:
                try:
                    stream.seek(0)
                    msvcrt.locking(stream.fileno(), msvcrt.LK_NBLCK, 1)
                    break
                except OSError:
                    if time.monotonic() >= deadline:
                        raise RuntimeError("Timed out waiting for the network dependency build")
                    time.sleep(0.1)
        else:
            import fcntl
            fcntl.flock(stream.fileno(), fcntl.LOCK_EX)
        try:
            yield
        finally:
            if os.name == "nt":
                stream.seek(0)
                msvcrt.locking(stream.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(stream.fileno(), fcntl.LOCK_UN)


def run(arguments: list[str], directory: Path, environment: dict[str, str]) -> None:
    subprocess.run(arguments, cwd=directory, env=environment, check=True)


def stage_file(source_path: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or digest(source_path) != digest(destination):
        shutil.copy2(source_path, destination)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("root", "cache", "bin", "lib", "include"):
        parser.add_argument("--" + name, required=True, type=Path)
    parser.add_argument("--arch", choices=("x64", "arm64"), required=True)
    parser.add_argument("--cc", required=True)
    parser.add_argument("--cxx", required=True)
    parser.add_argument("--cmake", required=True)
    parser.add_argument("--perl", required=True)
    parser.add_argument("--vs", default="")
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    if not 1 <= args.jobs <= 64:
        parser.error("--jobs must be in [1,64]")
    args.root.mkdir(parents=True, exist_ok=True)
    with build_lock(args.root / "build.lock"):
        environment = os.environ.copy()
        if os.name == "nt":
            vcvars = Path(args.vs) / "VC/Auxiliary/Build/vcvarsall.bat"
            if not vcvars.is_file():
                raise RuntimeError("The selected Visual Studio installation must supply vcvarsall.bat")
            target = "amd64" if args.arch == "x64" else "amd64_arm64"
            result = subprocess.run(["cmd.exe", "/d", "/s", "/c", f'call "{vcvars}" {target} >nul && set'], check=True, capture_output=True, text=True)
            for line in result.stdout.splitlines():
                if "=" in line and not line.startswith("="):
                    key, value = line.split("=", 1)
                    environment[key.upper()] = value
            nasm = source("nasm", args.cache, args.root)
            environment["PATH"] = str(nasm) + os.pathsep + environment.get("PATH", "")
        else:
            environment["CC"] = args.cc
            environment["CXX"] = args.cxx
        openssl = source("openssl", args.cache, args.root)
        msquic = source("msquic", args.cache, args.root)
        prefix = args.root / "install"
        ssl_build = args.root / "openssl-build"
        ssl_build.mkdir(exist_ok=True)
        # Providers use their release C ABI in every engine configuration. Their allocators
        # never free application-owned storage. No C++ standard-library ABI crosses this boundary.
        if os.name == "nt":
            ssl_target = "VC-WIN64A" if args.arch == "x64" else "VC-WIN64-ARM"
            make = ["nmake", "/NOLOGO"]
            ssl_library = prefix / "lib/libssl.lib"
        elif sys.platform == "darwin":
            ssl_target = "darwin64-x86_64-cc" if args.arch == "x64" else "darwin64-arm64-cc"
            make = ["make", "-j" + str(args.jobs)]
            ssl_library = prefix / "lib/libssl.3.dylib"
        else:
            ssl_target = "linux-x86_64" if args.arch == "x64" else "linux-aarch64"
            make = ["make", "-j" + str(args.jobs)]
            ssl_library = prefix / "lib/libssl.so.3"
        ssl_stamp = prefix / "xii-openssl-3.5.9.ready"
        if not ssl_library.exists() or not ssl_stamp.exists():
            # A failed recursive make can leave truncated generated assembler. Rebuild only
            # this private provider tree, under the process lock, rather than reuse partial output.
            shutil.rmtree(ssl_build)
            ssl_build.mkdir()
            command = [args.perl, str(openssl / "Configure"), ssl_target, "shared", "no-tests", "no-ssl3", "no-comp", "--libdir=lib", "--prefix=" + str(prefix)]
            if sys.platform.startswith("linux"):
                system_directory = subprocess.run(["openssl", "version", "-d"], capture_output=True, text=True)
                if system_directory.returncode == 0 and '"' in system_directory.stdout:
                    command.append("--openssldir=" + system_directory.stdout.split('"')[1])
            run(command, ssl_build, environment)
            run(make, ssl_build, environment)
            run([make[0]] + (["/NOLOGO"] if os.name == "nt" else []) + ["install_sw"], ssl_build, environment)
            ssl_stamp.write_text("verified-source-openssl-3.5.9\n", encoding="utf-8")
        quic_build = args.root / "msquic-build"
        quic_output = quic_build / "provider"
        quic_library = quic_output / ("msquic.dll" if os.name == "nt" else "libmsquic.2.dylib" if sys.platform == "darwin" else "libmsquic.so.2")
        provider_stamp = args.root / "provider-config-v2.ready"
        if not quic_library.exists() or not provider_stamp.exists():
            quic_build.mkdir(exist_ok=True)
            command = [args.cmake, "-S", str(msquic), "-B", str(quic_build), "-DCMAKE_BUILD_TYPE=Release", "-DQUIC_TLS_LIB=openssl", "-DQUIC_USE_EXTERNAL_OPENSSL=ON", "-DQUIC_OPENSSL_INCLUDE_DIR=" + str(prefix / "include"), "-DQUIC_OPENSSL_LIB_DIR=" + str(prefix / "lib"), "-DQUIC_BUILD_SHARED=ON", "-DQUIC_BUILD_TOOLS=OFF", "-DQUIC_BUILD_TEST=OFF", "-DQUIC_BUILD_PERF=OFF", "-DQUIC_ENABLE_LOGGING=OFF", "-DQUIC_OUTPUT_DIR=" + str(quic_output), "-DCMAKE_INSTALL_RPATH=" + ("@loader_path" if sys.platform == "darwin" else "$ORIGIN"), "-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON", "-DQUIC_STATIC_LINK_CRT=OFF", "-DQUIC_STATIC_LINK_PARTIAL_CRT=OFF"]
            if os.name == "nt":
                command += ["-G", "NMake Makefiles"]
            else:
                command += ["-G", "Ninja", "-DCMAKE_C_COMPILER=" + args.cc, "-DCMAKE_CXX_COMPILER=" + args.cxx]
            if sys.platform == "darwin":
                command += ["-DCMAKE_OSX_ARCHITECTURES=" + ("arm64" if args.arch == "arm64" else "x86_64")]
            run(command, args.root, environment)
            run([args.cmake, "--build", str(quic_build), "--config", "Release", "--parallel", str(args.jobs), "--target", "msquic"], args.root, environment)
            provider_stamp.write_text("relocatable-rpath-v2\n", encoding="utf-8")
        args.include.mkdir(parents=True, exist_ok=True)
        shutil.copytree(prefix / "include", args.include, dirs_exist_ok=True)
        for header in (msquic / "src/inc").glob("*.h"):
            stage_file(header, args.include / header.name)
        if os.name == "nt":
            for name in ("libssl", "libcrypto"):
                stage_file(prefix / ("lib/" + name + ".lib"), args.lib / (name + ".lib"))
                for path in (prefix / "bin").glob(name + "-3*.dll"):
                    stage_file(path, args.bin / path.name)
            stage_file(quic_output / "msquic.dll", args.bin / "msquic.dll")
            stage_file(quic_output / "msquic.lib", args.lib / "msquic.lib")
        else:
            names = ("libssl.3.dylib", "libcrypto.3.dylib") if sys.platform == "darwin" else ("libssl.so.3", "libcrypto.so.3")
            for name in names:
                stage_file(prefix / "lib" / name, args.bin / name)
            stage_file(quic_library, args.bin / quic_library.name)
            if sys.platform == "darwin":
                for name in names:
                    path = args.bin / name
                    run(["install_name_tool", "-id", "@rpath/" + name, str(path)], args.root, environment)
                    for dependency in names:
                        run(["install_name_tool", "-change", str(prefix / "lib" / dependency), "@rpath/" + dependency, str(path)], args.root, environment)
        # The marker is written only after every expected artifact is available.
        (args.bin / "xii-network-dependencies.ready").write_text("OpenSSL 3.5.9\nMsQuic 2.6.4\n", encoding="utf-8")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Network dependency build failed: {error}", file=sys.stderr)
        sys.exit(1)
