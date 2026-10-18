#!/usr/bin/env python3
# Copyright (c) Theophilus Eriata. All Rights Reserved.

"""Validate infrastructure invariants and links in contributor documentation."""

import json
from pathlib import Path
import re
import subprocess
from urllib.parse import unquote, urlsplit

root = Path(__file__).resolve().parents[2]
errors = []
for path in (root / ".github/workflows").glob("*.yml"):
    content = path.read_text(encoding="utf-8")
    for action in re.findall(r"^\s*-?\s*uses:\s*(\S+)", content, re.MULTILINE):
        if not action.startswith("./") and not re.fullmatch(r"[\w.-]+/[\w./-]+@[0-9a-f]{40}", action):
            errors.append(f"{path.name}: action is not pinned to a full commit: {action}")
        if action.startswith("./") and not (root / action).is_file():
            errors.append(f"{path.name}: missing reusable workflow {action}")
    if not re.search(r"^permissions:\n", content, re.MULTILINE):
        errors.append(f"{path.name}: no explicit workflow permissions")
    if "continue-on-error:" in content or "@master" in content:
        errors.append(f"{path.name}: failure masking or mutable action reference")
    if "pull_request_target:" in content and ("actions/checkout@" in content or re.search(r"^\s+run:", content, re.MULTILINE)):
        errors.append(f"{path.name}: privileged PR metadata job must not check out or execute code")

presets = json.loads((root / "CMakePresets.json").read_text(encoding="utf-8"))
names = {preset["name"] for preset in presets["configurePresets"]}
for preset in presets["configurePresets"]:
    parents = preset.get("inherits", [])
    if isinstance(parents, str):
        parents = [parents]
    if any(parent not in names for parent in parents):
        errors.append(f"Unknown parent in preset {preset['name']}")

docs = [root / name for name in ("README.md", "CONTRIBUTING.md", "SECURITY.md", "SUPPORT.md", "Coding-Standards.md")]
docs += [root / "Documentation" / name for name in ("Readme.md", "Building.md", "Infrastructure.md")]
for path in docs:
    if not path.is_file():
        errors.append(f"Missing document: {path}")
        continue
    # Remove code fences so command examples are not treated as Markdown links.
    content = re.sub(r"```.*?```", "", path.read_text(encoding="utf-8"), flags=re.DOTALL)
    for target in re.findall(r"!?\[[^\]]*\]\(([^\s)]+)\)", content):
        parts = urlsplit(target.strip("<>"))
        if parts.scheme or parts.netloc or not parts.path:
            continue
        resolved = path.parent / unquote(parts.path)
        if not resolved.exists():
            errors.append(f"{path.relative_to(root)}: missing link {target}")
if errors:
    raise SystemExit("\n".join(errors))
subprocess.run(["git", "diff", "--check"], cwd=root, check=True)
print("Action pins, workflow boundaries, preset references, local documentation links, and diff whitespace passed.")
