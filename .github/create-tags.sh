#!/usr/bin/env bash
# Compatibility entry point; preview by default, pass --apply to update GitHub.
set -euo pipefail
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
exec python3 "$script_dir/../Utilities/CI/manage_labels.py" sync --repo XII-Technologies/XII "$@"
