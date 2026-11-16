# Copyright (c) Theophilus Eriata. All Rights Reserved.

"""Exercise native CMake NuGet acquisition, corruption recovery, and relocatable SDK targets."""

from concurrent.futures import ThreadPoolExecutor
import hashlib
import http.server
from pathlib import Path
import platform
import shutil
import socket
import ssl
import struct
import subprocess
import tempfile
import threading
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[3]
MODULE = ROOT / "Source/BuildSystem/CMake/CMakeUtils/xiiUtilsNuGet.cmake"


@unittest.skipUnless(shutil.which("cmake"), "CMake is required")
class NuGetTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="XII NuGet ")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.feed = self.root / "feed"
        self.feed.mkdir()
        self.cache = self.root / "Workspace/Nuget"
        self.counter = 0

    def package(self, name="Fixture.Native", version="1.2.3", *, files=None, metadata=None):
        directory = self.feed / name.lower() / version
        directory.mkdir(parents=True, exist_ok=True)
        archive = directory / f"{name.lower()}.{version}.nupkg"
        contents = {
            f"{name}.nuspec": metadata if metadata is not None else (
                f'<?xml version="1.0"?><package><metadata><id>{name}</id>'
                f'<version>{version}</version></metadata></package>'
            ),
            "[Content_Types].xml": '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"/>',
            "_rels/.rels": '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"/>',
            "include/fixture.h": "int fixture(void);\n",
            "build/native/ignored.targets": '<Project><Exec Command="must-never-run"/></Project>',
        }
        contents.update(files or {})
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as output:
            for path, value in contents.items():
                if value is not None:
                    output.writestr(path, value)
        return archive, hashlib.sha256(archive.read_bytes()).hexdigest()

    def preamble(self):
        return (
            'cmake_minimum_required(VERSION 3.20)\n'
            f'set(XII_ROOT [==[{self.root.as_posix()}]==])\n'
            f'include([==[{MODULE.as_posix()}]==])\n'
        )

    def fetch(self, digest, name="Fixture.Native", version="1.2.3", extra=""):
        return (
            f'xii_nuget_fetch_package(PACKAGE {name} VERSION {version} SHA256 {digest}\n'
            f'  SOURCE [==[{self.feed.as_uri()}]==] OUT_PACKAGE_DIR PACKAGE_DIR {extra})\n'
        )

    def run_script(self, body, *, success=True):
        self.counter += 1
        script = self.root / f"test-{self.counter}.cmake"
        script.write_text(self.preamble() + body, encoding="utf-8")
        result = subprocess.run(["cmake", "-P", str(script)], capture_output=True, text=True, timeout=30)
        if success:
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        else:
            self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        return result

    def test_versioned_cache_offline_and_cross_build_configuration_reuse(self):
        _, digest = self.package()
        self.run_script(self.fetch(digest, "FIXTURE.Native"))
        directory = self.cache / "fixture.native/1.2.3"
        archive = directory / "fixture.native.1.2.3.nupkg"
        marker = directory / ".extracted"
        timestamps = (archive.stat().st_mtime_ns, marker.stat().st_mtime_ns)
        shutil.rmtree(self.feed)
        project = self.root / "project"
        project.mkdir()
        (project / "CMakeLists.txt").write_text(
            self.preamble() + 'project(Cache NONE)\nset(XII_NUGET_OFFLINE ON)\n'
            + self.fetch(digest) + 'file(WRITE "${CMAKE_BINARY_DIR}/package.txt" "${PACKAGE_DIR}")\n',
            encoding="utf-8",
        )
        for config in ("Debug", "Dev", "Shipping"):
            build = self.root / f"build-{config}"
            result = subprocess.run(["cmake", "-S", str(project), "-B", str(build), f"-DCMAKE_BUILD_TYPE={config}"], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual((build / "package.txt").read_text(), (directory / "Extracted").as_posix())
            shutil.rmtree(build)
        self.assertEqual(timestamps, (archive.stat().st_mtime_ns, marker.stat().st_mtime_ns))
        self.run_script(f'xii_nuget_find_package(PACKAGE Fixture.Native VERSION 1.2.3 SHA256 {digest} OUT_PACKAGE_DIR FOUND)\nif(NOT FOUND)\nmessage(FATAL_ERROR "cache miss")\nendif()\n')

    def test_bad_hash_and_partial_download_are_not_published(self):
        _, digest = self.package()
        result = self.run_script(self.fetch("0" * 64), success=False)
        self.assertIn("SHA256 mismatch", result.stderr)
        self.assertFalse(list(self.cache.glob("*/*/*.nupkg")))
        directory = self.cache / "fixture.native/1.2.3"
        (directory / "fixture.native.1.2.3.nupkg.download").write_bytes(b"interrupted")
        self.run_script(self.fetch(digest))
        archive = directory / "fixture.native.1.2.3.nupkg"
        archive.write_bytes(b"corrupted cached archive")
        result = self.run_script("set(XII_NUGET_OFFLINE ON)\n" + self.fetch(digest), success=False)
        self.assertIn("SHA256 mismatch for cached", result.stderr)

    def test_interrupted_and_modified_extractions_recover_without_network(self):
        _, digest = self.package()
        self.run_script(self.fetch(digest))
        directory = self.cache / "fixture.native/1.2.3"
        (directory / ".extracting").mkdir()
        (directory / ".extracting/partial").write_text("partial")
        (directory / "Extracted/include/fixture.h").unlink()
        shutil.rmtree(self.feed)
        self.run_script("set(XII_NUGET_OFFLINE ON)\n" + self.fetch(digest))
        self.assertEqual((directory / "Extracted/include/fixture.h").read_text(), "int fixture(void);\n")
        (directory / "Extracted/include/fixture.h").write_text("corrupted")
        self.run_script("set(XII_NUGET_OFFLINE ON)\n" + self.fetch(digest))
        (directory / ".extracted").write_text("forged marker")
        self.run_script("set(XII_NUGET_OFFLINE ON)\n" + self.fetch(digest))
        self.assertTrue((directory / ".extracted").read_text().startswith("XII-NUGET-1\nfixture.native@1.2.3\n"))

    def test_concurrent_acquisition_has_one_consistent_result(self):
        _, digest = self.package(files={"large.bin": b"x" * 2_000_000})
        scripts = []
        for i in range(6):
            script = self.root / f"concurrent-{i}.cmake"
            script.write_text(self.preamble() + self.fetch(digest))
            scripts.append(script)
        with ThreadPoolExecutor(max_workers=6) as pool:
            results = list(pool.map(lambda path: subprocess.run(["cmake", "-P", str(path)], capture_output=True, text=True, timeout=30), scripts))
        for result in results:
            self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(sum("Downloading NuGet" in result.stdout for result in results), 1)
        self.assertEqual(len(list(self.cache.glob("*/*/.extracted"))), 1)

    def test_rejects_unsafe_paths_links_and_case_collisions(self):
        for path in ("../escape", "/absolute", "C:/escape", "folder\\escape", "name:stream", "CON.txt", "trailing./value"):
            with self.subTest(path=path):
                shutil.rmtree(self.cache, ignore_errors=True)
                _, digest = self.package(files={path: "unsafe"})
                result = self.run_script(self.fetch(digest), success=False)
                self.assertIn("Unsafe", result.stderr)
                shutil.rmtree(self.cache)
        archive, _ = self.package(files={"Include/Fixture.h": "case collision"})
        digest = hashlib.sha256(archive.read_bytes()).hexdigest()
        result = self.run_script(self.fetch(digest), success=False)
        self.assertIn("case-colliding", result.stderr)
        shutil.rmtree(self.cache)
        archive, _ = self.package()
        with zipfile.ZipFile(archive, "a") as output:
            link = zipfile.ZipInfo("linked")
            link.create_system = 3
            link.external_attr = 0o120777 << 16
            output.writestr(link, "../escape")
        result = self.run_script(self.fetch(hashlib.sha256(archive.read_bytes()).hexdigest()), success=False)
        self.assertIn("links and special files", result.stderr)

    def test_metadata_identity_missing_files_and_version_errors(self):
        cases = [
            dict(metadata="not XML"),
            dict(metadata='<package><metadata><id>Wrong</id><version>1.2.3</version></metadata></package>'),
            dict(files={"[Content_Types].xml": None}),
            dict(metadata='<package><metadata><id>Fixture.Native</id><version>9.0.0</version></metadata></package>'),
        ]
        for case in cases:
            with self.subTest(case=case):
                _, digest = self.package(**case)
                self.run_script(self.fetch(digest), success=False)
                shutil.rmtree(self.cache)
        _, digest = self.package()
        self.run_script(self.fetch(digest, version="latest"), success=False)
        self.run_script(self.fetch(digest, name="../escape"), success=False)

    def test_explicit_dependencies_are_required_and_validated(self):
        _, dependency_hash = self.package("Fixture.Dependency", "2.0.0")
        metadata = '<package><metadata><id>Fixture.Native</id><version>1.2.3</version><dependencies><dependency id="Fixture.Dependency" version="[2.0.0]" /></dependencies></metadata></package>'
        _, digest = self.package(metadata=metadata)
        result = self.run_script(self.fetch(digest), success=False)
        self.assertIn("explicitly pinned dependency", result.stderr)
        self.run_script(self.fetch(dependency_hash, "Fixture.Dependency", "2.0.0") + self.fetch(digest, extra="DEPENDENCIES fixture.dependency@2.0.0"))
        shutil.rmtree(self.cache)
        metadata = metadata.replace('[2.0.0]', '[2.0.0,3.0.0)')
        _, digest = self.package(metadata=metadata)
        self.run_script(self.fetch(digest), success=False)

    def test_balanced_xml_and_package_opc_metadata(self):
        for metadata in (
            '<package><metadata><id>Fixture.Native</id><version>1.2.3</version></wrong></package>',
            '<package><metadata><id>Fixture.Native</id><version>1.2.3</version></metadata>',
            '<!DOCTYPE package [<!ENTITY remote SYSTEM "https://invalid.example/">]><package/>',
        ):
            shutil.rmtree(self.cache, ignore_errors=True)
            _, digest = self.package(metadata=metadata)
            self.run_script(self.fetch(digest), success=False)
        shutil.rmtree(self.cache, ignore_errors=True)
        _, digest = self.package(files={"_rels/.rels": "invalid OPC XML"})
        self.run_script(self.fetch(digest), success=False)
        shutil.rmtree(self.cache, ignore_errors=True)
        _, digest = self.package(metadata='<package><metadata><id>Fixture.Native</id><version>1.2.3+build.4</version><description><![CDATA[<id>NotMetadata</id>]]></description></metadata></package>')
        self.run_script(self.fetch(digest))

    def test_validation_does_not_repair_and_version_metadata_is_normalized(self):
        _, digest = self.package()
        self.run_script(self.fetch(digest, name="FIXTURE.NATIVE", version="01.02.003+build.7"))
        marker = self.cache / "fixture.native/1.2.3/.extracted"
        marker.write_text("corrupt")
        self.run_script(f'xii_nuget_validate_package(PACKAGE Fixture.Native VERSION 1.2.3 SHA256 {digest} OUT_VALID VALID)\nif(VALID)\nmessage(FATAL_ERROR "corrupt cache accepted")\nendif()\n')
        self.assertEqual(marker.read_text(), "corrupt")

    @unittest.skipUnless(shutil.which("cc") and shutil.which("ar") and platform.system() == "Linux", "Linux native archive fixture")
    def test_static_archive_and_exported_private_runtime_metadata(self):
        source = self.root / "native.c"
        source.write_text("int fixture(void) { return 42; }\n")
        obj = self.root / "native.o"
        archive = self.root / "libfixture.a"
        subprocess.run(["cc", "-c", str(source), "-o", str(obj)], check=True, capture_output=True)
        subprocess.run(["ar", "rcs", str(archive), str(obj)], check=True, capture_output=True)
        _, digest = self.package(files={"lib/libfixture.a": archive.read_bytes()})
        project = self.root / "static-project"
        project.mkdir()
        (project / "wrapper.c").write_text("int wrapper(void) { return 0; }\n")
        (project / "CMakeLists.txt").write_text(self.preamble() + 'project(StaticArchive C)\n' + self.fetch(digest) + r'''
xii_nuget_import_native_library(TARGET Fixture::Static PACKAGE_DIR "${PACKAGE_DIR}"
  TYPE STATIC PLATFORM Linux ARCHITECTURE x64 ABI C CRT NONE LOCATION lib/libfixture.a INCLUDE_DIRS include)
add_library(Wrapper SHARED wrapper.c)
target_link_libraries(Wrapper PRIVATE Fixture::Static)
xii_nuget_stage_runtime(TARGET Wrapper LIBRARIES Wrapper)
export(TARGETS Wrapper FILE "${CMAKE_BINARY_DIR}/wrapper-targets.cmake")
xii_nuget_export_targets(FILE "${CMAKE_BINARY_DIR}/sdk/xiiNuGetTargets.cmake" TARGETS Fixture::Static)
''')
        build = self.root / "static-build"
        result = subprocess.run(["cmake", "-S", str(project), "-B", str(build), "-DCMAKE_BUILD_TYPE=Debug"], capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr)
        subprocess.run(["cmake", "--build", str(build)], check=True, capture_output=True, timeout=30)
        exported = (build / "wrapper-targets.cmake").read_text()
        self.assertIn('XII_NUGET_RUNTIME_TARGETS "Fixture::Static"', exported)
        self.assertIn('add_library(Fixture::Static STATIC IMPORTED GLOBAL)', (build / "sdk/xiiNuGetTargets.cmake").read_text())

    def test_offline_miss_and_incomplete_archive_fail(self):
        archive, digest = self.package()
        result = self.run_script("set(XII_NUGET_OFFLINE ON)\n" + self.fetch(digest), success=False)
        self.assertIn("offline cache miss", result.stderr)
        archive.write_bytes(b"not a ZIP")
        digest = hashlib.sha256(archive.read_bytes()).hexdigest()
        result = self.run_script(self.fetch(digest), success=False)
        self.assertIn("Invalid NuGet archive", result.stderr)

    @unittest.skipUnless(shutil.which("openssl"), "OpenSSL CLI needed for local HTTPS fixture")
    def test_actual_interrupted_transfer_does_not_publish_archive(self):
        class Interrupted(http.server.BaseHTTPRequestHandler):
            def do_GET(self):
                self.send_response(200)
                self.send_header("Content-Length", "10000")
                self.end_headers()
                self.wfile.write(b"partial")
                self.wfile.flush()
                self.connection.shutdown(socket.SHUT_RDWR)

            def log_message(self, *args):
                pass

        certificate = self.root / "certificate.pem"
        key = self.root / "key.pem"
        subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-subj", "/CN=localhost", "-addext", "subjectAltName=DNS:localhost", "-days", "1", "-keyout", str(key), "-out", str(certificate)], check=True, capture_output=True)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(certificate, key)
        with http.server.ThreadingHTTPServer(("localhost", 0), Interrupted) as server:
            server.socket = context.wrap_socket(server.socket, server_side=True)
            worker = threading.Thread(target=server.serve_forever, daemon=True)
            worker.start()
            try:
                body = f'set(CMAKE_TLS_CAINFO [==[{certificate.as_posix()}]==])\n'
                body += f'xii_nuget_fetch_package(PACKAGE Fixture.Native VERSION 1.2.3 SHA256 {"0" * 64} SOURCE https://localhost:{server.server_port} OUT_PACKAGE_DIR RESULT)\n'
                result = self.run_script(body, success=False)
                self.assertIn("download failed", result.stderr)
                self.assertFalse(list(self.cache.glob("*/*/*.nupkg")))
                self.assertFalse(list(self.cache.glob("*/*/*.download")))
            finally:
                server.shutdown()
                worker.join(timeout=5)

    @unittest.skipUnless(shutil.which("cc") and platform.system() == "Linux", "Linux C compiler needed for executable SDK fixture")
    def test_native_targets_runtime_installation_and_sdk_relocation(self):
        source = self.root / "native.c"
        source.write_text("int fixture(void) { return 42; }\n")
        library = self.root / "libfixture.so"
        subprocess.run(["cc", "-shared", "-fPIC", str(source), "-o", str(library)], check=True, capture_output=True)
        _, digest = self.package(files={"lib/libfixture.so": library.read_bytes(), "lib/fixture.debug": "symbols"})
        project = self.root / "producer"
        project.mkdir()
        native = '''xii_nuget_import_native_library(TARGET Fixture::Native PACKAGE_DIR "${PACKAGE_DIR}"
TYPE SHARED PLATFORM Linux ARCHITECTURE x64 ABI C CRT NONE LOCATION lib/libfixture.so
INCLUDE_DIRS include SYMBOLS lib/fixture.debug)
'''
        (project / "main.c").write_text('#include <fixture.h>\nint main(void) { return fixture() == 42 ? 0 : 1; }\n')
        (project / "CMakeLists.txt").write_text(self.preamble() + 'project(Producer C)\n' + self.fetch(digest) + native + '''
add_executable(app main.c)
target_link_libraries(app PRIVATE Fixture::Native)
xii_nuget_stage_runtime(TARGET app LIBRARIES Fixture::Native)
xii_nuget_export_targets(FILE "${CMAKE_BINARY_DIR}/sdk/xiiNuGetTargets.cmake" TARGETS Fixture::Native INSTALL_DESTINATION native-sdk)
''')
        build = self.root / "build"
        result = subprocess.run(["cmake", "-S", str(project), "-B", str(build), "-DCMAKE_BUILD_TYPE=Debug"], capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        result = subprocess.run(["cmake", "--build", str(build)], capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue((build / "libfixture.so").is_file())
        self.assertTrue((build / "fixture.debug").is_file())
        prefix = self.root / "installed"
        subprocess.run(["cmake", "--install", str(build), "--prefix", str(prefix)], check=True, capture_output=True)
        relocated = self.root / "relocated"
        prefix.rename(relocated)
        shutil.rmtree(build)
        shutil.rmtree(self.cache)
        consumer = self.root / "consumer"
        consumer.mkdir()
        shutil.copyfile(project / "main.c", consumer / "main.c")
        (consumer / "CMakeLists.txt").write_text(self.preamble() + 'project(Consumer C)\n' + f'include([==[{(relocated / "native-sdk/xiiNuGetTargets.cmake").as_posix()}]==])\n' + '''
add_executable(app main.c)
target_link_libraries(app PRIVATE Fixture::Native)
xii_nuget_stage_runtime(TARGET app LIBRARIES Fixture::Native)
''')
        for config in ("Debug", "Dev", "Shipping"):
            directory = self.root / f"consumer-{config}"
            result = subprocess.run(["cmake", "-S", str(consumer), "-B", str(directory), f"-DCMAKE_BUILD_TYPE={config}"], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stderr)
            subprocess.run(["cmake", "--build", str(directory)], check=True, capture_output=True, timeout=30)
            subprocess.run([str(directory / "app")], check=True, timeout=5)
            self.assertTrue((directory / "libfixture.so").is_file())
            self.assertTrue((directory / "fixture.debug").is_file())

    def test_explicit_asset_architecture_and_pe_headers(self):
        pe = bytearray(256)
        pe[:2] = b"MZ"
        pe[60:64] = struct.pack("<I", 128)
        pe[128:134] = b"PE\0\0" + struct.pack("<H", 0xAA64)
        _, digest = self.package(files={"native.dll": pe, "native.lib": pe})
        project = self.root / "architecture"
        project.mkdir()
        (project / "CMakeLists.txt").write_text(self.preamble() + 'project(Arch NONE)\nset(CMAKE_SYSTEM_NAME Windows)\nset(CMAKE_SYSTEM_PROCESSOR AMD64)\nset(CMAKE_SIZEOF_VOID_P 8)\n' + self.fetch(digest) + '''
xii_nuget_import_native_library(TARGET Fixture::Native PACKAGE_DIR "${PACKAGE_DIR}"
TYPE SHARED PLATFORM Windows ARCHITECTURE x64 ABI C CRT DYNAMIC LOCATION native.dll IMPLIB native.lib)
''')
        result = subprocess.run(["cmake", "-S", str(project), "-B", str(self.root / "arch-build")], capture_output=True, text=True, timeout=30)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("architecture mismatch", result.stderr)


if __name__ == "__main__":
    unittest.main()
