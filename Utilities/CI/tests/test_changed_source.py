# Copyright (c) Theophilus Eriata. All Rights Reserved.

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

CI = Path(__file__).resolve().parents[1]


@unittest.skipIf(os.name == "nt", "Uses a POSIX executable formatter fixture")
class ChangedSourceTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.repo = Path(self.temporary.name)
        self.git("init", "-q")
        self.git("config", "user.name", "Infrastructure test")
        self.git("config", "user.email", "test@example.invalid")
        (self.repo / "Utilities").mkdir()
        shutil.copy(CI.parent / "ClangFormatDiff.py", self.repo / "Utilities")
        self.source = self.repo / "source with spaces.inl"
        self.source.write_text("/// Copyright (c) Theophilus Eriata. All Rights Reserved.\n\nint x = 1;\n")
        self.git("add", ".")
        self.git("commit", "-qm", "Base source")
        self.base = self.git("rev-parse", "HEAD")
        self.source.write_text(self.source.read_text().replace("1", "2"))
        self.git("add", ".")
        self.git("commit", "-qm", "Change source")
        self.formatter = self.repo / "formatter"
        self.formatter.write_text(
            "#!/usr/bin/env python3\nimport pathlib, sys\nprint(pathlib.Path(sys.argv[-1]).read_text(), end='')\n"
        )
        self.formatter.chmod(0o755)

    def git(self, *arguments):
        return subprocess.check_output(["git", *arguments], cwd=self.repo, text=True).strip()

    def check(self):
        return subprocess.run(
            [sys.executable, str(CI / "check_changed_source.py"), "--base", self.base,
             "--formatter", str(self.formatter)], cwd=self.repo, capture_output=True, text=True,
        )

    def test_clean_changed_lines_pass_with_spaces_and_inl(self):
        result = self.check()
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_suggestion_fails_without_modifying_source(self):
        original = self.source.read_bytes()
        self.formatter.write_text(self.formatter.read_text().replace("end=''", "end='// suggestion\\n'"))
        result = self.check()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.source.read_bytes(), original)
        self.assertIn("suggestion", (self.repo / "Workspace/Quality/clang-format.patch").read_text())

    def test_formatter_error_is_not_success(self):
        self.formatter.write_text("#!/usr/bin/env python3\nraise SystemExit(7)\n")
        self.assertEqual(self.check().returncode, 7)


if __name__ == "__main__":
    unittest.main()
