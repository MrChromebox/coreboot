#!/usr/bin/env bash
#
# SPDX-License-Identifier: GPL-2.0-only

# On some systems, `parted` and `debugfs` are located in /sbin.
export PATH="$PATH:/sbin"

# external URLs
_url_primary=https://dl.google.com/dl/edgedl/chromeos/recovery/recovery2.json
_url_enterprise=https://dl.google.com/dl/edgedl/chromeos/recovery/workspaceHardware_recovery2.json
_url_legacy=https://dl.google.com/dl/edgedl/chromeos/recovery/recovery.conf

exit_if_dependencies_are_missing() {
	local missing_deps=()
	local -A deps_map=(
		["uudecode"]="sharutils"
		["debugfs"]="e2fsprogs"
		["parted"]="parted"
		["curl"]="curl"
		["unzip"]="unzip"
		["7z"]="p7zip"
		["python3"]="python3"
	)

	# Check all dependencies at once
	for cmd_name in "${!deps_map[@]}"; do
		if ! type "$cmd_name" >/dev/null 2>&1; then
			missing_deps+=("$cmd_name:${deps_map[$cmd_name]}")
		fi
	done

	# Exit if any dependencies are missing
	if [ ${#missing_deps[@]} -gt 0 ]; then
		printf 'The following required commands were not found:\n' >&2
		for dep_info in "${missing_deps[@]}"; do
			cmd_name="${dep_info%%:*}"
			deb_pkg_name="${dep_info##*:}"
			printf '  - `%s`\n' "$cmd_name" >&2
			printf '    On Debian-based systems, install with: `apt install %s`\n' "$deb_pkg_name" >&2
		done
		exit 1
	fi
}

get_inventory_legacy() {
	_conf=$1

	echo "Downloading legacy recovery image inventory..."
	curl -s "$_url_legacy" >"$_conf"
}

get_inventory_json() {
	_json_primary=$1
	_json_secondary=$2

	echo "Downloading recovery image inventory..."
	curl -s "$_url_primary" >"$_json_primary"
	curl -s "$_url_enterprise" >"$_json_secondary"
}

get_inventory_enterprise_json() {
	_json_enterprise=$1
	_url_enterprise=https://dl.google.com/dl/edgedl/chromeos/recovery/workspaceHardware_recovery2.json

	echo "Downloading enterprise recovery image inventory..."
	curl -s "$_url_enterprise" >"$_json_enterprise"
}

lookup_recovery_from_json() {
	_board=$1
	_json_primary=$2
	_json_secondary=$3

	python3 - "$_board" "$_json_primary" "$_json_secondary" <<'PY'
import json
import re
import sys

board = (sys.argv[1] or "").lower()
primary_path = sys.argv[2]
secondary_path = sys.argv[3]

if not board:
    sys.exit(2)

preferred_channels = ["STABLE", "LTS", "LTC"]

def load(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)

def normalize(s):
    return str(s or "").strip()

def match_entry(e):
    """
    We treat the CLI argument as a HWID/board name (platform prefix), not the
    recovery image codename embedded in the URL.
    """
    hwidmatch = normalize(e.get("hwidmatch"))
    hwids = e.get("hwids") or []

    # 1) Exact hwids list (when present).
    if isinstance(hwids, list):
        for h in hwids:
            h = normalize(h).lower()
            if not h:
                continue
            # Accept either full HWID (GENESIS-FOO) or platform prefix (GENESIS).
            if h == board or h.startswith(board + "-"):
                return True

    # 2) hwidmatch regex string typically anchors the platform prefix.
    if hwidmatch:
        # Common patterns: ^GENESIS-.* , ^ATLAS .* , ^MORPHIUS[-| ].* , ...
        # Require a word boundary after the board so we accept bracket/suffix
        # forms without falsely matching a shorter prefix inside another name.
        if re.match(rf"^\^?{re.escape(board)}\b", hwidmatch.strip(), re.IGNORECASE):
            return True

    return False

def pick(entries):
    matches = [e for e in entries if isinstance(e, dict) and match_entry(e)]
    if not matches:
        return None
    for ch in preferred_channels:
        for e in matches:
            if str(e.get("channel", "")).upper() == ch:
                url = e.get("url")
                file = e.get("file")
                if url and file:
                    return (url, file)
    for e in matches:
        url = e.get("url")
        file = e.get("file")
        if url and file:
            return (url, file)
    return None

primary = load(primary_path)
secondary = load(secondary_path)

res = pick(primary) or pick(secondary)
if not res:
    sys.exit(1)

url, file = res
print(f"url={url}")
print(f"file={file}")
PY
}

download_image() {
	_url=$1
	_file=$2

	if [ -z "$_url" ] || [ -z "$_file" ]; then
		echo "Missing recovery URL or filename (url='$_url' file='$_file')" >&2
		exit 1
	fi

	echo "Downloading recovery image"
	curl "$_url" >"$_file.zip"
	echo "Decompressing recovery image"
	unzip -q "$_file.zip"
	rm "$_file.zip"
}

extract_partition() {
	NAME=$1
	FILE=$2
	ROOTFS=$3
	_bs=1024

	echo "Extracting ROOT-A partition"
	ROOTP=$(printf "unit\nB\nprint\nquit\n" |
		parted $FILE 2>/dev/null | grep $NAME)

	if [ "$ROOTP" == "" ]; then
		# Automatic extraction failed, likely due to parted detecting
		# overlapping partitions. Fall back to using fdisk and assume
		# ROOT-A is partition #3
		echo "(Extracting via parted failed; falling back to fdisk)"
		_ssize=$(printf "p q" | fdisk $FILE | grep "Sector size" |
			cut -f2 -d: | cut -f2 -d ' ')
		_start=$(printf "p q" | fdisk $FILE | grep "bin3" | tr -s ' ' |
			cut -f2 -d ' ')
		_nsec=$(printf "p q" | fdisk $FILE | grep "bin3" | tr -s ' ' |
			cut -f4 -d ' ')
		START=$(($_ssize * $_start))
		SIZE=$(($_ssize * $_nsec))
	else
		START=$(($(echo $ROOTP | cut -f2 -d\  | tr -d "B")))
		SIZE=$(($(echo $ROOTP | cut -f4 -d\  | tr -d "B")))
	fi

	dd if=$FILE of=$ROOTFS bs=$_bs skip=$(($START / $_bs)) \
		count=$(($SIZE / $_bs)) >/dev/null 2>&1
}

extract_shellball() {
	ROOTFS=$1
	SHELLBALL=$2

	echo "Extracting chromeos-firmwareupdate"
	printf "cd /usr/sbin\ndump chromeos-firmwareupdate $SHELLBALL\nquit" |
		debugfs $ROOTFS >/dev/null 2>&1
}

extract_coreboot() {
	_shellball=$1
	_unpacked=$(mktemp -d)

	echo "Extracting coreboot image"
	if ! sh $_shellball --unpack $_unpacked >/dev/null 2>&1; then
		if ! sh $_shellball --sb_extract $_unpacked >$_unpacked/sb_extract.log 2>&1; then
			echo "Failed to extract shellball image"
			cat $_unpacked/sb_extract.log
			exit 1
		fi
	fi

	if [ -d $_unpacked/models/ ]; then
		_version=$(cat $_unpacked/VERSION | grep -m 1 -e Model.*$_board -A5 |
			grep "BIOS (RW) version:" | cut -f2 -d: | tr -d \ )
		if [ "$_version" == "" ]; then
			_version=$(cat $_unpacked/VERSION | grep -m 1 -e Model.*$_board -A5 |
				grep "BIOS version:" | cut -f2 -d: | tr -d \ )
		fi
		if [ -f $_unpacked/models/$_board/setvars.sh ]; then
			_bios_image=$(grep "IMAGE_MAIN" $_unpacked/models/$_board/setvars.sh |
				cut -f2 -d'"')
		else
			# special case for REEF, others?
			_version=$(grep -m1 "host" "$_unpacked/manifest.json" | cut -f12 -d'"')
			_bios_image=$(grep -m1 "image" "$_unpacked/manifest.json" | cut -f4 -d'"')
		fi
	elif [ -f "$_unpacked/manifest.json" ]; then
		_version=$(grep -m1 -A4 "$BOARD\":" "$_unpacked/manifest.json" | grep -m1 "rw" |
				sed 's/.*\(rw.*\)/\1/' | sed 's/.*\("Google.*\)/\1/' | cut -f2 -d'"')
		_bios_image=$(grep -m1 -A10 "$BOARD\":" "$_unpacked/manifest.json" |
				grep -m1 "image" | sed 's/.*"image": //' | cut -f2 -d'"')
	else
		_version=$(cat $_unpacked/VERSION | grep BIOS\ version: |
			cut -f2 -d: | tr -d \ )
		_bios_image=bios.bin
	fi
	if cp $_unpacked/$_bios_image coreboot-$_version.bin; then
		echo "Extracted coreboot-$_version.bin"
	fi
	rm -rf "$_unpacked"
	rm $_shellball
}

do_one_board() {
	_board=$1
	_url=$2
	_file=$3

	download_image $_url $_file

	extract_partition ROOT-A $_file root-a.ext2
	extract_shellball root-a.ext2 chromeos-firmwareupdate-$_board
	rm $_file root-a.ext2

	extract_coreboot chromeos-firmwareupdate-$_board
}

#
# Main
#

LEGACY=0
ENTERPRISE=0

usage() {
	echo "Usage: $0 [--legacy] [--enterprise] <boardname>"
	echo
	echo "Options:"
	echo "  --legacy      Use legacy recovery image inventory (recovery.conf)"
	echo "  --enterprise  Use enterprise recovery image inventory (workspaceHardware_recovery2.json) only"
	echo
	echo "Arguments:"
	echo "  boardname  - Name of the board for which to extract the shellball firmware"
	echo
	echo "Examples:"
	echo "  $0 --legacy panther"
	echo "  $0 --enterprise genesis"
	echo "  $0 banshee"
}

while [ $# -gt 0 ]; do
	case "$1" in
		--legacy)
			LEGACY=1
			shift
			;;
		--enterprise)
			ENTERPRISE=1
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		--)
			shift
			break
			;;
		-*)
			echo "Unknown option: $1" >&2
			usage >&2
			exit 1
			;;
		*)
			break
			;;
	esac
done

BOARD=${1,,}

exit_if_dependencies_are_missing

if [ "$LEGACY" -eq 1 ] && [ "$ENTERPRISE" -eq 1 ]; then
	echo "Options --legacy and --enterprise are mutually exclusive." >&2
	usage >&2
	exit 1
fi

if [ "$BOARD" != "" ]; then
	if [ "$LEGACY" -eq 1 ]; then
		CONF=$(mktemp)
		get_inventory_legacy "$CONF"

		echo "Processing board $BOARD"
		# shellcheck disable=SC2154
		eval "$(grep -i -w "$BOARD" -A8 "$CONF" | grep '\(url=\|file=\)')"
		# shellcheck disable=SC2154
		do_one_board "$BOARD" "$url" "$file"

		rm "$CONF"
	else
		JSON_SECONDARY=$(mktemp)
		JSON_PRIMARY=$(mktemp)

		if [ "$ENTERPRISE" -eq 1 ]; then
			# Only use the enterprise inventory; provide an empty primary list.
			printf '[]\n' >"$JSON_PRIMARY"
			get_inventory_enterprise_json "$JSON_SECONDARY"
		else
			get_inventory_json "$JSON_PRIMARY" "$JSON_SECONDARY"
		fi

		echo Processing board $BOARD
		if ! eval "$(lookup_recovery_from_json "$BOARD" "$JSON_PRIMARY" "$JSON_SECONDARY")"; then
			echo "Failed to find recovery image for board: $BOARD" >&2
			rm "$JSON_PRIMARY" "$JSON_SECONDARY"
			exit 1
		fi
		do_one_board "$BOARD" "$url" "$file"

		rm "$JSON_PRIMARY" "$JSON_SECONDARY"
	fi
else
	usage
	exit 1
fi
