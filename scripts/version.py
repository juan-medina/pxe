#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 Juan Medina
# SPDX-License-Identifier: MIT

import json
import sys
from pathlib import Path

def load_json(p: Path) -> dict:
	return json.loads(p.read_text(encoding="utf-8"))

def save_json(p: Path, data: dict) -> None:
	p.write_text(json.dumps(data, indent=4), encoding="utf-8")

def bump_build(version_json: Path) -> int:
	data = load_json(version_json)
	version = data.get("version", {})
	version["build"] = int(version.get("build", 0)) + 1
	data["version"] = version
	save_json(version_json, data)
	return version["build"]

def get_version_parts(data: dict):
	v = data.get("version", {})
	major = int(v.get("major", 0))
	minor = int(v.get("minor", 0))
	patch = int(v.get("patch", 0))
	build = int(v.get("build", 0))
	return major, minor, patch, build

def emit_rc(version_json: Path, rc_out: Path, icon_ico: Path) -> None:
	data = load_json(version_json)
	major, minor, patch, build = get_version_parts(data)

	filever_commas = f"{major},{minor},{patch},{build}"
	filever_dots = f"{major}.{minor}.{patch}.{build}"

	rc = f"""// Auto-generated. Do not edit.
#include <winver.h>

IDI_ICON1 ICON "{icon_ico.as_posix()}"

VS_VERSION_INFO VERSIONINFO
 FILEVERSION {filever_commas}
 PRODUCTVERSION {filever_commas}
 FILEFLAGSMASK 0x3fL
 FILEFLAGS 0x0L
 FILEOS 0x40004L
 FILETYPE 0x1L
 FILESUBTYPE 0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904b0"
        BEGIN
            VALUE "CompanyName", "Juan Medina"
            VALUE "FileDescription", "Energy Swap"
            VALUE "FileVersion", "{filever_dots}"
            VALUE "InternalName", "energy-swap"
            VALUE "LegalCopyright", "Copyright (c) 2024 Juan Medina"
            VALUE "OriginalFilename", "energy-swap.exe"
            VALUE "ProductName", "Energy Swap"
            VALUE "ProductVersion", "{filever_dots}"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x0409, 1200
    END
END
"""
	rc_out.write_text(rc, encoding="utf-8")

def main():
	# Mode 1: bump only (default)
	if len(sys.argv) == 2:
		version_json = Path(sys.argv[1])
		if not version_json.exists():
			print(f"Version file not found: {version_json}")
			return 1
		new_build = bump_build(version_json)
		print(f"Updated build number to {new_build}")
		return 0

	# Mode 2: emit rc
	if len(sys.argv) == 5 and sys.argv[1] == "--emit-rc":
		version_json = Path(sys.argv[2])
		rc_out = Path(sys.argv[3])
		icon_ico = Path(sys.argv[4])

		if not version_json.exists():
			print(f"Version file not found: {version_json}")
			return 1
		if not icon_ico.exists():
			print(f"Icon file not found: {icon_ico}")
			return 1

		rc_out.parent.mkdir(parents=True, exist_ok=True)
		emit_rc(version_json, rc_out, icon_ico)
		return 0

	print("Usage:")
	print("  version.py <path-to-version-json>")
	print("  version.py --emit-rc <version-json> <rc-out> <icon-ico>")
	return 1

if __name__ == "__main__":
	raise SystemExit(main())
