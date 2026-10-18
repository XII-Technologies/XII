# Copyright (c) Theophilus Eriata. All Rights Reserved.

"""Exercise snapshot and push races against real local Git remotes."""

import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch


spec = importlib.util.spec_from_file_location(
    "publish_api_docs", Path(__file__).resolve().parents[1] / "publish_api_docs.py")
publisher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(publisher)
SOURCE_SHA = "a" * 40


class PublicationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.remote = self.root / "remote.git"
        subprocess.run(["git", "init", "--bare", "--initial-branch=Development", str(self.remote)],
                       check=True, capture_output=True)
        self.writer = self.clone("writer")
        (self.writer / "docs").mkdir()
        (self.writer / "docs/index.html").write_text("Old API", encoding="utf-8")
        (self.writer / "docs/obsolete.html").write_text("Remove me", encoding="utf-8")
        (self.writer / "README.md").write_text("Keep me", encoding="utf-8")
        publisher.git(self.writer, "add", ".")
        publisher.git(self.writer, "commit", "-m", "Initial documentation")
        publisher.git(self.writer, "push", "origin", "Development")
        self.checkout = self.clone("publisher")
        self.generated = self.root / "generated"
        self.generated.mkdir()
        (self.generated / "index.html").write_text("Generated API", encoding="utf-8")

    def clone(self, name):
        destination = self.root / name
        subprocess.run(["git", "clone", str(self.remote), str(destination)], check=True, capture_output=True)
        publisher.git(destination, "config", "user.name", "Infrastructure test")
        publisher.git(destination, "config", "user.email", "test@example.invalid")
        return destination

    def advance(self, count=1, same_docs=False):
        (self.writer / "README.md").write_text(f"Concurrent update {count}", encoding="utf-8")
        if same_docs:
            (self.writer / "docs/index.html").write_text("Generated API", encoding="utf-8")
            (self.writer / "docs/obsolete.html").unlink()
        else:
            (self.writer / "docs/index.html").write_text(f"Concurrent docs {count}", encoding="utf-8")
        publisher.git(self.writer, "add", ".")
        publisher.git(self.writer, "commit", "-m", f"Concurrent change {count}")
        publisher.git(self.writer, "push", "origin", "Development")
        return publisher.git(self.writer, "rev-parse", "HEAD").stdout.strip()

    def remote_file(self, filename):
        return publisher.git(self.writer, "show", f"origin/Development:{filename}").stdout

    def test_stale_checkout_preserves_latest_changes_and_deletes_old_docs(self):
        latest = self.advance()
        publisher.publish(self.checkout, self.generated, SOURCE_SHA)
        publisher.git(self.writer, "fetch", "origin")
        self.assertEqual(self.remote_file("README.md"), "Concurrent update 1")
        self.assertEqual(self.remote_file("docs/index.html"), "Generated API")
        self.assertNotEqual(publisher.git(self.writer, "cat-file", "-e",
                                         "origin/Development:docs/obsolete.html", check=False).returncode, 0)
        publisher.git(self.writer, "merge-base", "--is-ancestor", latest, "origin/Development")

    def test_competing_push_recreates_generated_commit(self):
        real_git = publisher.git
        pushes = []

        def race(checkout, *args, **kwargs):
            if Path(checkout) == self.checkout and args[0] == "push":
                pushes.append(args)
                if len(pushes) == 1:
                    self.advance()
            return real_git(checkout, *args, **kwargs)

        with patch.object(publisher, "git", side_effect=race):
            publisher.publish(self.checkout, self.generated, SOURCE_SHA)
        self.assertEqual(len(pushes), 2)
        self.assertTrue(all("--force" not in args for args in pushes))
        publisher.git(self.writer, "fetch", "origin")
        self.assertEqual(self.remote_file("README.md"), "Concurrent update 1")
        self.assertEqual(self.remote_file("docs/index.html"), "Generated API")

    def test_competing_identical_publication_is_a_noop(self):
        real_git = publisher.git

        def race(checkout, *args, **kwargs):
            if Path(checkout) == self.checkout and args[0] == "push":
                self.advance(same_docs=True)
            return real_git(checkout, *args, **kwargs)

        with patch.object(publisher, "git", side_effect=race):
            publisher.publish(self.checkout, self.generated, SOURCE_SHA)
        self.assertEqual(publisher.git(self.checkout, "rev-parse", "HEAD").stdout,
                         publisher.git(self.writer, "rev-parse", "HEAD").stdout)

    def test_push_failure_without_branch_advance_is_not_hidden(self):
        real_git = publisher.git

        def reject(checkout, *args, **kwargs):
            if args[0] == "push":
                return subprocess.CompletedProcess(args, 1, "", "Permission denied")
            return real_git(checkout, *args, **kwargs)

        with patch.object(publisher, "git", side_effect=reject):
            with self.assertRaisesRegex(RuntimeError, "Permission denied"):
                publisher.publish(self.checkout, self.generated, SOURCE_SHA)

    def test_retries_are_bounded(self):
        real_git = publisher.git
        pushes = []

        def race(checkout, *args, **kwargs):
            if Path(checkout) == self.checkout and args[0] == "push":
                pushes.append(args)
                self.advance(len(pushes))
            return real_git(checkout, *args, **kwargs)

        with patch.object(publisher, "git", side_effect=race):
            with self.assertRaisesRegex(RuntimeError, "2 publication attempts"):
                publisher.publish(self.checkout, self.generated, SOURCE_SHA, attempts=2)
        self.assertEqual(len(pushes), 2)

    def test_dirty_checkout_is_rejected(self):
        (self.checkout / "README.md").write_text("Local work", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "must be clean"):
            publisher.publish(self.checkout, self.generated, SOURCE_SHA)
        self.assertEqual((self.checkout / "README.md").read_text(), "Local work")


if __name__ == "__main__":
    unittest.main()
