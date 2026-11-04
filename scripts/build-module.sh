#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source_dir="$project_dir/upstream/ml"

for tool in git make python3 arm-none-eabi-gcc arm-none-eabi-ld arm-none-eabi-strip readelf; do
    command -v "$tool" >/dev/null || { echo "Missing tool: $tool" >&2; exit 1; }
done
python3 -c 'import docutils' || { echo 'Missing Python dependency: docutils' >&2; exit 1; }
expected_commit="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["commit"])' "$project_dir/upstream.lock.json")"
actual_commit="$(git -C "$source_dir" rev-parse HEAD)"
if [[ "$actual_commit" != "$expected_commit" ]]; then
    echo "Upstream differs from upstream.lock.json. Review the version first." >&2
    exit 1
fi
for file in Makefile.globals modules/Makefile modules/readme2modulestrings.py modules/module_strings_wrapper.h data/fonts/argnor23.rbf src/module.h src/menu.h; do
    [[ -f "$source_dir/$file" ]] || { echo "Missing upstream file: $file" >&2; exit 1; }
done
module_dir="$source_dir/modules/camdesk"
if [[ -e "$module_dir" && ! -f "$module_dir/.camdesk-managed" ]]; then
    echo "Refusing to overwrite an unmanaged directory: $module_dir" >&2
    exit 1
fi
mkdir -p -- "$module_dir"
touch "$module_dir/.camdesk-managed"
for file in Makefile README.rst camdesk.c interval_setup.c interval_setup.h; do
    cp -- "$project_dir/modules/camdesk/$file" "$module_dir/$file"
done
make -C "$module_dir" TOP_DIR="$source_dir" FATAL_WARNINGS=y "$@"
echo "Module: $module_dir/build/camdesk.mo"
echo 'Development output only. Compilation does not establish camera compatibility.'
