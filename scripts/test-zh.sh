#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"
gcc -std=c99 -Wall -Wextra -Werror -Wno-misleading-indentation -g -fsanitize=address,undefined \
    -DCJK_HOST_TEST -DZH_HOST_TEST -Itests -Iupstream/ml/src \
    tests/zh_test.c localization/cjk_font.c upstream/ml/src/zh_translation.c -o build/zh_test
./build/zh_test
