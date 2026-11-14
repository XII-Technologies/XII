# Copyright (c) Theophilus Eriata. All Rights Reserved.

from contextlib import chdir
from pathlib import Path
import shutil
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

    def format(self, formatter, *selection):
        real_run = subprocess.run

        def invoke(command, *args, **kwargs):
            if command[0] == "clang-format":
                return formatter(command)
            return real_run(command, *args, **kwargs)

        with patch.object(sys, "argv", ["format_changed_source.py", *(selection or ("--base", self.base))]), \
             patch.object(style.subprocess, "run", side_effect=invoke):
            style.main()

    def test_repairs_and_formats_whole_files_and_emits_reusable_patch(self):
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
                "clang-format", "--style=file", "--fallback-style=none", "-i", "--", str(self.source)
            ])
            self.assertTrue(self.source.read_bytes().startswith(b"/// " + style.COPYRIGHT))
            self.source.write_bytes(self.source.read_bytes().replace(b"a=2", b"a = 2").replace(b"b=3", b"b = 3"))

        self.format(formatter)
        formatted = self.source.read_bytes()
        self.assertIn(b"int b = 3;", formatted)
        self.assertTrue(cmake.read_bytes().startswith(style.BOM + b"# " + style.COPYRIGHT + b"\r\n\r\n"))
        self.assertEqual(third_party.read_bytes(), b"int library=0;\n")
        self.git("restore", ".")
        self.assertEqual(self.source.read_bytes(), original)
        self.git("apply", "--index", str(self.artifact))
        self.assertEqual(self.source.read_bytes(), formatted)

    def test_rename_formats_entire_file(self):
        renamed = Path("renamed with spaces.inl")
        self.git("mv", str(self.source), str(renamed))
        renamed.write_bytes(renamed.read_bytes().replace(b"a = 1", b"a = 2"))
        self.commit("Rename source")
        commands = []
        self.format(commands.append)
        self.assertEqual(commands, [
            ["clang-format", "--style=file", "--fallback-style=none", "-i", "--", str(renamed)]
        ])
        self.assertFalse(self.artifact.exists())

    def test_all_includes_unchanged_sources_and_excludes_unowned_files(self):
        cmake = Path("CMakeLists.txt")
        cmake.write_bytes(b"project(Test)\n")
        excluded = []
        for directory in (*style.EXCLUDED, "Source/EnginePlugins/RenderDocPlugin/ThirdParty/"):
            path = Path(directory) / "library.cpp"
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b"int library=0;\n")
            excluded.append(path)
        link = Path("linked.cpp")
        try:
            link.symlink_to(self.source)
        except OSError:
            link = None
        self.commit("Add repository sources")
        untracked = Path("untracked.cpp")
        untracked.write_bytes(b"int untracked=0;\n")
        commands = []
        self.format(commands.append, "--all")
        self.assertEqual(commands, [[
            "clang-format", "--style=file", "--fallback-style=none", "-i", "--", str(self.source)
        ]])
        self.assertTrue(cmake.read_bytes().startswith(b"# " + style.COPYRIGHT + b"\n\n"))
        for path in excluded:
            self.assertEqual(path.read_bytes(), b"int library=0;\n")
        self.assertEqual(untracked.read_bytes(), b"int untracked=0;\n")
        if link is not None:
            self.assertTrue(link.is_symlink())

    def test_all_from_subdirectory_uses_repository_root(self):
        directory = Path("nested")
        directory.mkdir()
        commands = []
        with chdir(directory):
            self.format(commands.append, "--all")
        self.assertEqual(commands[0][-1], str(self.source))

    def test_dirty_checkout_is_rejected_without_changes(self):
        original = b"int local=7;\n"
        self.source.write_bytes(original)
        with self.assertRaises(SystemExit), patch("sys.stderr"):
            self.format(self.fail, "--all")
        self.assertEqual(self.source.read_bytes(), original)
        self.assertFalse(self.artifact.exists())

    @unittest.skipUnless(shutil.which("clang-format"), "clang-format is not installed")
    def test_real_formatter_formats_unchanged_lines_and_is_idempotent(self):
        Path(".clang-format").write_text(
            "BasedOnStyle: LLVM\nIndentWidth: 2\nBreakBeforeBraces: Allman\n"
        )
        self.source.write_bytes(b"int first(){return 1;}\nint second( ){return 2;}\n")
        self.commit("Add unformatted functions")
        self.base = self.git("rev-parse", "HEAD").decode().strip()
        self.source.write_bytes(self.source.read_bytes().replace(b"return 1", b"return 3"))
        self.commit("Change only the first function")
        self.format(lambda command: subprocess.check_call(command))
        expected = subprocess.check_output([
            "clang-format", "--style=file", "--", str(self.source)
        ])
        self.assertEqual(self.source.read_bytes(), expected)
        self.assertIn(b"int second()", expected)
        self.assertNotIn(b"second( )", expected)
        self.assertTrue(self.artifact.exists())
        self.artifact.unlink()
        self.commit("Record formatting")
        self.format(lambda command: subprocess.check_call(command), "--all")
        self.assertFalse(self.artifact.exists())

    def test_deletion_only_change_still_formats_file(self):
        self.source.write_bytes(self.source.read_bytes().replace(b"int a = 1;\n", b""))
        self.commit("Delete a declaration")
        commands = []
        self.format(commands.append)
        self.assertEqual(len(commands), 1)
        self.assertEqual(commands[0][-1], str(self.source))

    def test_empty_source_notice_has_no_trailing_blank_line(self):
        self.source.write_bytes(b"")
        style.fix_notice(self.source, b"///")
        expected = b"/// " + style.COPYRIGHT + b"\n"
        self.assertEqual(self.source.read_bytes(), expected)
        style.fix_notice(self.source, b"///")
        self.assertEqual(self.source.read_bytes(), expected)

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
