#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source_dir="$project_dir/upstream/ml"
expected="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["commit"])' "$project_dir/upstream.lock.json")"
[[ "$(git -C "$source_dir" rev-parse HEAD)" == "$expected" ]] || { echo 'Upstream version mismatch' >&2; exit 1; }
# Build a physical-camera core and its matching module symbols, without installer changes.
make -C "$source_dir/platform/50D.109" CONFIG_QEMU=n FATAL_WARNINGS=y \
    -j4 build/autoexec.bin build/50D_109.sym
