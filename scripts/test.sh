#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$project_dir/build"
gcc -std=c99 -Wall -Wextra -Werror -fsanitize=address,undefined -g \
    -I"$project_dir/modules/camdesk" "$project_dir/tests/interval_setup_test.c" \
    "$project_dir/modules/camdesk/interval_setup.c" -o "$project_dir/build/interval_setup_test"
"$project_dir/build/interval_setup_test"
