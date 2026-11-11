# Copyright (c) Theophilus Eriata. All Rights Reserved.

from contextlib import chdir
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import format_changed_source as style


class ChangedSourceTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.enterContext(chdir(temporary.name))
        self.git("init", "-q")
        self.git("config", "user.name", "Infrastructure test")
        self.git("config", "user.email", "test@example.invalid")
        self.git("config", "core.autocrlf", "false")
        self.source = Path("source with spaces.inl")
        self.source.write_bytes(b"/// " + style.COPYRIGHT + b"\n\nint a = 1;\n\nint b=3;\n")
        self.commit("Base source")
        self.base = self.git("rev-parse", "HEAD").decode().strip()
        self.artifact = Path("Workspace/Quality/source-style.patch")

    def git(self, *arguments):
        return subprocess.check_output(["git", *arguments])

    def commit(self, message):
        self.git("add", ".")
        self.git("commit", "-qm", message)

    def format(self, formatter):
        real_run = subprocess.run

        def invoke(command, *args, **kwargs):
            if command[0] == "clang-format":
                return formatter(command)
            return real_run(command, *args, **kwargs)

        with patch.object(sys, "argv", ["format_changed_source.py", "--base", self.base]), \
             patch.object(style.subprocess, "run", side_effect=invoke):
            style.main()

    def test_repairs_and_formats_changed_lines_and_emits_reusable_patch(self):
        original = self.source.read_bytes().replace(
            b"/// " + style.COPYRIGHT, b"// Copyright 2025 Theophilus Eriata"
        ).replace(b"a = 1", b"a=2")
        self.source.write_bytes(original)
        cmake = Path("CMakeLists.txt")
        cmake.write_bytes(style.BOM + b"cmake_minimum_required(VERSION 3.30)\r\n")
        third_party = Path("Source/ThirdParty/library.cpp")
        third_party.parent.mkdir(parents=True)
        third_party.write_bytes(b"int library=0;\n")
        self.commit("Change sources")

        def formatter(command):
            self.assertEqual(command, [
                "clang-format", "--style=file", "-i", "--lines=3:3", "--", str(self.source)
            ])
            self.assertTrue(self.source.read_bytes().startswith(b"/// " + style.COPYRIGHT))
            self.source.write_bytes(self.source.read_bytes().replace(b"a=2", b"a = 2"))

        self.format(formatter)
        formatted = self.source.read_bytes()
        self.assertIn(b"int b=3;", formatted)
        self.assertTrue(cmake.read_bytes().startswith(style.BOM + b"# " + style.COPYRIGHT + b"\r\n\r\n"))
        self.assertEqual(third_party.read_bytes(), b"int library=0;\n")
        self.git("restore", ".")
        self.assertEqual(self.source.read_bytes(), original)
        self.git("apply", "--index", str(self.artifact))
        self.assertEqual(self.source.read_bytes(), formatted)

    def test_rename_preserves_changed_line_scope(self):
        renamed = Path("renamed with spaces.inl")
        self.git("mv", str(self.source), str(renamed))
        renamed.write_bytes(renamed.read_bytes().replace(b"a = 1", b"a = 2"))
        self.commit("Rename source")
        commands = []
        self.format(commands.append)
        self.assertEqual(commands, [
            ["clang-format", "--style=file", "-i", "--lines=3:3", "--", str(renamed)]
        ])
        self.assertFalse(self.artifact.exists())

    def test_failed_formatter_does_not_emit_patch(self):
        self.source.write_bytes(self.source.read_bytes().replace(b"a = 1", b"a=2"))
        self.commit("Change source")

        def formatter(command):
            raise subprocess.CalledProcessError(7, command)

        with self.assertRaises(subprocess.CalledProcessError):
            self.format(formatter)
        self.assertFalse(self.artifact.exists())


if __name__ == "__main__":
    unittest.main()
