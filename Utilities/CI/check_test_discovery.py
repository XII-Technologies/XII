#!/usr/bin/env python3
# Copyright (c) Theophilus Eriata. All Rights Reserved.

"""Reject empty or partial hosted test discovery before invoking CTest."""

import json
import subprocess
import sys

result = subprocess.run(
    ["ctest", "--test-dir", sys.argv[1], "-C", sys.argv[2], "-L", "^headless$", "--show-only=json-v1"],
    check=True, capture_output=True, text=True,
)
discovered = {test["name"] for test in json.loads(result.stdout)["tests"]}
expected = {"FoundationTest", "CoreTest", "ToolsFoundationTest"}
if discovered != expected:
    raise SystemExit(f"Expected {sorted(expected)}, discovered {sorted(discovered)}")
print(f"Discovered all {len(expected)} hosted suites: {', '.join(sorted(discovered))}")
