# Copyright (c) Theophilus Eriata. All Rights Reserved.

import copy
import importlib.util
import json
from pathlib import Path
import re
import unittest
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location("manage_labels", ROOT / "Utilities/CI/manage_labels.py")
labels = importlib.util.module_from_spec(spec)
spec.loader.exec_module(labels)


class LabelTests(unittest.TestCase):
    def setUp(self):
        self.catalog = json.loads((ROOT / labels.CATALOG).read_text(encoding="utf-8"))

    def test_catalog_and_generated_rules(self):
        labels.validate(self.catalog, ROOT)
        self.assertEqual((ROOT / labels.RULES).read_text(encoding="utf-8"), labels.render(self.catalog))

    def test_new_module_requires_coverage(self):
        modules = labels.discover_modules(ROOT) | {"Source/Engine/NewModule"}
        with patch.object(labels, "discover_modules", return_value=modules):
            with self.assertRaisesRegex(ValueError, "NewModule"):
                labels.validate(self.catalog, ROOT)

    def test_obsolete_module_path_rejected(self):
        self.catalog[0]["paths"].append("Source/Engine/DeletedModule/**")
        with self.assertRaisesRegex(ValueError, "Missing directory"):
            labels.validate(self.catalog, ROOT)

    def test_duplicate_label_rejected(self):
        duplicate = copy.deepcopy(self.catalog[0])
        duplicate["name"] = duplicate["name"].upper()
        self.catalog.append(duplicate)
        with self.assertRaisesRegex(ValueError, "duplicate label"):
            labels.validate(self.catalog, ROOT)

    def test_manual_labels_have_no_path_rules(self):
        manual = next(label for label in self.catalog if label["name"].startswith("Status: "))
        manual["paths"] = ["Source/**"]
        with self.assertRaisesRegex(ValueError, "Manual label"):
            labels.validate(self.catalog, ROOT)

    def test_issue_form_area_options_match_catalog(self):
        areas = {label["name"] for label in self.catalog if label["name"].startswith("Area: ")} | {"GitHub"}
        for filename in ("bug_report.yml", "feature_request.yml"):
            content = (ROOT / ".github/ISSUE_TEMPLATE" / filename).read_text(encoding="utf-8")
            options = set(re.findall(r'^        - "((?:Area: [^"]+)|GitHub)"$', content, re.MULTILINE))
            self.assertEqual(areas, options, filename)

    def test_unmanaged_labels_preserved(self):
        existing = self.catalog + [dict(name="Review: Custom Assessment", color="ffffff", description="Manual")]
        self.assertEqual(labels.pending_labels(self.catalog, existing), [])
        existing[0] = dict(existing[0], description="Outdated")
        self.assertEqual(labels.pending_labels(self.catalog, existing), [self.catalog[0]])

    def test_dry_run_does_not_mutate(self):
        with patch.object(labels, "get_labels", return_value=[]), patch.object(labels, "gh") as gh:
            labels.synchronize(self.catalog[:1], "owner/repository", False)
            gh.assert_not_called()

    def test_sync_upserts_and_verifies(self):
        with patch.object(labels, "get_labels", side_effect=[[], self.catalog]), patch.object(labels, "gh") as gh:
            labels.synchronize(self.catalog[:1], "owner/repository", True)
            gh.assert_called_once_with("label", "create", "GitHub", "--repo", "owner/repository",
                                       "--color", "6a737d", "--description", self.catalog[0]["description"], "--force")

    def test_sync_readback_failure_propagates(self):
        with patch.object(labels, "get_labels", return_value=[]), patch.object(labels, "gh"):
            with self.assertRaisesRegex(RuntimeError, "after synchronization"):
                labels.synchronize(self.catalog[:1], "owner/repository", True)


if __name__ == "__main__":
    unittest.main()
