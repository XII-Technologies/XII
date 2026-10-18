#!/usr/bin/env python3
# Copyright (c) Theophilus Eriata. All Rights Reserved.

"""Publish generated API docs from a clean, disposable destination checkout.

Recreate only the generated docs commit on the latest remote branch after a push
race. Do not rebase generated-file conflicts or force push over other writers.
"""

import argparse
from pathlib import Path
import re
import shutil
import subprocess


def git(checkout, *args, check=True):
    return subprocess.run(["git", *args], cwd=checkout, check=check,
                          text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def publish(checkout, generated, source_sha, attempts=5):
    checkout, generated = Path(checkout).resolve(), Path(generated).resolve()
    if not (checkout / ".git").is_dir():
        raise ValueError("Destination must be a standalone disposable Git checkout")
    if generated == checkout or generated.is_relative_to(checkout) or checkout.is_relative_to(generated):
        raise ValueError("Generated documentation must be outside the destination checkout")
    if not (generated / "index.html").is_file():
        raise ValueError("Generated documentation is missing index.html")
    if any(path.is_symlink() for path in generated.rglob("*")):
        raise ValueError("Generated documentation must not contain symbolic links")
    if not re.fullmatch(r"[0-9a-f]{40}", source_sha):
        raise ValueError("Source revision must be a full commit SHA")
    if attempts < 1:
        raise ValueError("At least one publication attempt is required")
    if git(checkout, "status", "--porcelain", "--untracked-files=all").stdout:
        raise ValueError("Destination checkout must be clean before publication")

    remote_ref = "refs/remotes/origin/Development"

    def fetch():
        git(checkout, "fetch", "--no-tags", "origin",
            f"+refs/heads/Development:{remote_ref}")
        return git(checkout, "rev-parse", remote_ref).stdout.strip()

    base = fetch()
    for attempt in range(1, attempts + 1):
        # This checkout is disposable and was verified clean. A retry discards
        # only our unpublished generated commit, then reapplies docs to new HEAD.
        git(checkout, "checkout", "--detach", base)
        target = checkout / "docs"
        if target.is_symlink() or (target.exists() and not target.is_dir()):
            raise ValueError("Destination docs must be a regular directory")
        if target.exists():
            shutil.rmtree(target)
        shutil.copytree(generated, target)
        git(checkout, "add", "--all", "--", "docs")
        difference = git(checkout, "diff", "--cached", "--quiet", check=False)
        if difference.returncode == 0:
            print("API documentation already matches the latest destination branch.")
            return
        if difference.returncode != 1:
            difference.check_returncode()
        git(checkout, "commit", "-m", f"Documentation: Update API reference for {source_sha}.")
        result = git(checkout, "push", "origin", "HEAD:refs/heads/Development", check=False)
        if result.returncode == 0:
            print(f"Published API documentation on attempt {attempt}.")
            return
        latest = fetch()
        if latest == base:
            # Authentication, policy and transport failures are not push races.
            raise RuntimeError(f"Documentation push failed without branch advancement:\n{result.stderr}")
        print(f"Destination advanced during attempt {attempt}; refreshing generated commit.")
        base = latest
    raise RuntimeError(f"Destination kept advancing across {attempts} publication attempts; rerun publication.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--checkout", type=Path, required=True)
    parser.add_argument("--generated", type=Path, required=True)
    parser.add_argument("--source-sha", required=True)
    args = parser.parse_args()
    publish(args.checkout, args.generated, args.source_sha)


if __name__ == "__main__":
    main()
