# Copyright (c) Theophilus Eriata. All Rights Reserved.

"""Exercise release identity, archive contents, and checksum integrity in a fresh repo."""

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "package_source.py"


class SourcePackageTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.repo = Path(self.temporary.name)
        self.git("init", "-q")
        self.git("config", "user.name", "Infrastructure test")
        self.git("config", "user.email", "test@example.invalid")
        (self.repo / "version.txt").write_text("1.2.3\n", encoding="utf-8")
        (self.repo / "LICENSE.md").write_text("Preserved attribution\n", encoding="utf-8")
        self.git("add", "version.txt", "LICENSE.md")
        self.git("commit", "-qm", "Test source snapshot")
        self.git("tag", "v1.2.3")
        (self.repo / "untracked-secret.txt").write_text("excluded", encoding="utf-8")

    def git(self, *arguments):
        return subprocess.check_output(["git", *arguments], cwd=self.repo, text=True).strip()

    def package(self, tag):
        return subprocess.run(
            [sys.executable, str(SCRIPT), "--tag", tag, "--output", "dist"],
            cwd=self.repo, capture_output=True, text=True,
        )

    def test_exact_snapshot_and_checksums(self):
        result = self.package("v1.2.3")
        self.assertEqual(result.returncode, 0, result.stderr)
        output = self.repo / "dist"
        manifest = json.loads((output / "XII-1.2.3-source-manifest.json").read_text())
        self.assertEqual(manifest["commit"], self.git("rev-parse", "HEAD"))
        with tarfile.open(output / "XII-1.2.3-source.tar.gz") as archive:
            self.assertIn("XII-1.2.3/LICENSE.md", archive.getnames())
            self.assertNotIn("XII-1.2.3/untracked-secret.txt", archive.getnames())
        for line in (output / "SHA256SUMS").read_text().splitlines():
            digest, name = line.split("  ", 1)
            self.assertEqual(digest, hashlib.sha256((output / name).read_bytes()).hexdigest())

    def test_version_mismatch_fails_without_artifacts(self):
        self.assertNotEqual(self.package("v9.9.9").returncode, 0)
        self.assertFalse((self.repo / "dist").exists())

    def test_tag_must_identify_head(self):
        (self.repo / "new.txt").write_text("new commit", encoding="utf-8")
        self.git("add", "new.txt")
        self.git("commit", "-qm", "Move HEAD beyond tag")
        self.assertNotEqual(self.package("v1.2.3").returncode, 0)
        self.assertFalse((self.repo / "dist").exists())


if __name__ == "__main__":
    unittest.main()
