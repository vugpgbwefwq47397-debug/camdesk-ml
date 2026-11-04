#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname -- "${BASH_SOURCE[0]}")/.."
gcc -std=c99 -Wall -Wextra -Werror -Wno-misleading-indentation -g -fsanitize=address,undefined \
    -DCJK_HOST_TEST -DZH_HOST_TEST -Itests -Iui -Iupstream/ml/src \
    tests/ui_test.c ui/nordic_draw.c ui/face.c localization/cjk_font.c upstream/ml/src/zh_translation.c -o build/ui_test
./build/ui_test
gcc -std=c99 -Wall -Wextra -Werror -g -fsanitize=address,undefined -Iui \
    tests/ui_snapshot_test.c -o build/ui_snapshot_test
./build/ui_snapshot_test
gcc -std=c99 -Wall -Wextra -Werror -Wno-misleading-indentation -g -fsanitize=address,undefined \
    -DCJK_HOST_TEST -Itests -Iui -Iupstream/ml/src \
    tests/font_io_failure_test.c ui/face.c localization/cjk_font.c -o build/font_io_failure_test
CD_TEST_IO_EOF=1 ./build/font_io_failure_test
CD_TEST_IO_ERROR=1 ./build/font_io_failure_test
CD_TEST_IO_ALLOC=1 ./build/font_io_failure_test
