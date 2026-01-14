#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 Juan Medina
# SPDX-License-Identifier: MIT

import json
import sys
import subprocess
from pathlib import Path


def main():
	if len(sys.argv) != 2:
		print("Usage: release.py project path")
		sys.exit(1)

	path = Path(sys.argv[1])
	json_path = path / "resources/version/version.json"
	if not json_path.exists():
		print(f"Version file not found: {json_path}")
		sys.exit(1)

	data = json.loads(json_path.read_text())

	version = data.get("version", {})

	version_str = f"v{version.get('major', 0)}.{version.get('minor', 0)}.{version.get('patch', 0)}.{version.get('build', 0)}"
	print(f"version: {version_str}")

	release_notes_path = path / "release-notes.md"

	if not release_notes_path.exists():
		print(f"release notes not found: {release_notes_path}")
		sys.exit(1)

	print(f"release notes: {release_notes_path}")

	print(f"Creating release: {version_str}...")

	# Create git tag
	result = subprocess.run(
		["git", "tag", "-a", version_str, "-m", f"Release {version_str}"],
		capture_output=True, text=True
	)
	if result.returncode != 0:
		print(f"Failed to create git tag. {result.stderr.strip()}")
		sys.exit(1)

	# Push tags
	result = subprocess.run(
		["git", "push", "--tags"],
		capture_output=True, text=True
	)
	if result.returncode != 0:
		print(f"Failed to push tags. {result.stderr.strip()}")
		sys.exit(1)

	# Create GitHub release
	result = subprocess.run(
		["gh", "release", "create", version_str, "-F", str(release_notes_path), "-t", version_str],
		capture_output=True, text=True
	)
	if result.returncode != 0:
		print(f"Failed to create release. {result.stderr.strip()}")
		sys.exit(1)

if __name__ == "__main__":
	main()
