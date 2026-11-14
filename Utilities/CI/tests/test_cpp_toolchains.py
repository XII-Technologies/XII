# Copyright (c) Theophilus Eriata. All Rights Reserved.

"""Check the external SDK compiler contract without requiring every host toolchain."""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[3]
MODULE = ROOT / "Source/BuildSystem/CMake/CMakeUtils/xiiUtilsExternal.cmake"


@unittest.skipUnless(shutil.which("cmake"), "CMake is required")
class CppToolchainTests(unittest.TestCase):
    def validate(self, family="Clang", version="23.1.5", **overrides):
        values = {}
        for language in ("C", "CXX"):
            values[f"EXPINP_{language}_COMPILER_ID"] = family
            values[f"EXPINP_{language}_COMPILER_VERSION"] = version
            values[f"EXPINP_{language}_COMPILER_SIMULATE_ID"] = ""
            values[f"CMAKE_{language}_COMPILER_ID"] = family
            values[f"CMAKE_{language}_COMPILER_VERSION"] = version
            values[f"CMAKE_{language}_SIMULATE_ID"] = ""
        values.update(overrides)
        with tempfile.TemporaryDirectory(prefix="XII toolchain ") as directory:
            script = Path(directory) / "validate.cmake"
            script.write_text(
                f'include("{MODULE.as_posix()}")\n'
                + "".join(f"set({key} [==[{value}]==])\n" for key, value in values.items())
                + "xii_validate_sdk_toolchain()\n",
                encoding="utf-8",
            )
            return subprocess.run(
                ["cmake", "-P", str(script)], capture_output=True, text=True, timeout=10
            )

    def test_supported_families_and_compatible_patch_versions(self):
        for family, version in (("Clang", "23.1.5"), ("GNU", "16.2.1"), ("MSVC", "19.50.1")):
            with self.subTest(family=family):
                result = self.validate(family, version, CMAKE_CXX_COMPILER_VERSION=version + ".1")
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_wrong_family_or_mixed_c_and_cpp_is_rejected(self):
        for language in ("C", "CXX"):
            with self.subTest(language=language):
                result = self.validate(**{f"CMAKE_{language}_COMPILER_ID": "GNU"})
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("SDK requires Clang", result.stderr)

    def test_wrong_major_and_missing_version_are_rejected(self):
        for version in ("22.9.1", ""):
            with self.subTest(version=version):
                result = self.validate(CMAKE_CXX_COMPILER_VERSION=version)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("compiler major version 23", result.stderr)

    def test_clang_abi_target_must_match_the_sdk(self):
        result = self.validate(EXPINP_CXX_COMPILER_SIMULATE_ID="MSVC")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("ABI target differs", result.stderr)

    def test_msvc_toolset_must_match_the_sdk(self):
        matching = self.validate("MSVC", "19.50.1", EXPINP_MSVC_VERSION="1950", MSVC_VERSION="1950")
        self.assertEqual(matching.returncode, 0, matching.stderr)
        mismatched = self.validate("MSVC", "19.50.1", EXPINP_MSVC_VERSION="1950", MSVC_VERSION="1944")
        self.assertNotEqual(mismatched.returncode, 0)
        self.assertIn("MSVC toolset version 1950", mismatched.stderr)


if __name__ == "__main__":
    unittest.main()
