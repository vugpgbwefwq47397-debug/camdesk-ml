# Build / 构建

目标为50D.109；其他机型不能直接复用这些成品。Target: 50D.109. Adapt and rebuild for other cameras.

## Environment / 环境

Linux or WSL; the release was built with Ubuntu 24.04. Install `git`, `make`, `gcc`, `binutils-arm-none-eabi`, `gcc-arm-none-eabi`, `python3`, `python3-pil`, `python3-docutils`, and `fonts-wqy-microhei`. Upstream also uses ordinary shell/coreutils and its bundled libraries. Internet access is needed for a fresh upstream clone; the source ZIP includes the prepared source subset used by this target.

## Repository checkout / 从仓库构建

Run at the project root:

```sh
mkdir -p upstream build
git clone --no-checkout https://github.com/reticulatedpines/magiclantern_simplified.git upstream/ml
git -C upstream/ml checkout --detach 142472e95054d73640f50789ac0fd66325499d82
python3 scripts/fetch-localization.py
python3 scripts/prepare-zh.py
bash scripts/test-ui.sh
bash scripts/test-zh.sh
bash scripts/test.sh
bash scripts/build-core.sh
bash scripts/build-module.sh
python3 scripts/render-ui-build.py
python3 scripts/stage-zh.py
```

The packaged bitmap fonts can be used as supplied. To regenerate them after preparing translations: run `python3 scripts/build-cjk-fonts.py`, `python3 scripts/build-ui-font.py` and `python3 scripts/build-ui-face.py`, then repeat tests and build. Inter sources are bundled; WenQuanYi is loaded from the installed system font package.

## Prepared source ZIP / 已应用修改的源码包

The `source.zip` includes `upstream/ml` with generated translation/UI source already applied, without Git history or build artifacts. Its `SOURCE-MANIFEST.json` records every included source file. Do **not** run the Git-checkout preparation scripts on this snapshot. Build directly:

```sh
mkdir -p build
bash scripts/test-ui.sh
bash scripts/test-zh.sh
bash scripts/test.sh
make -C upstream/ml/platform/50D.109 CONFIG_QEMU=n FATAL_WARNINGS=y -j4 build/autoexec.bin build/50D_109.sym
make -C upstream/ml/modules/camdesk TOP_DIR="$(pwd)/upstream/ml" FATAL_WARNINGS=y
python3 scripts/render-ui-build.py
python3 scripts/stage-zh.py
```

This archive is the 50D build subset present during the release, not a complete checkout of every upstream camera/module. Upstream may label snapshot builds “unofficial: not a git repo.” Timestamp/hostname/compiler changes mean rebuilt binaries need not be byte-identical to the release. The release manifest identifies the distributed binaries.

源码包已包含本次目标所用上游代码及生成文件，直接make即可；重新生成源码覆盖层请走固定提交的Git仓库流程。不要使用开发电脑专用的写卡脚本。本发布包不提供一键格式化或首次安装功能。
