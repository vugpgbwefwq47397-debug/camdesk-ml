# Credits and licenses / 来源与授权

- **Magic Lantern**: [reticulatedpines/magiclantern_simplified](https://github.com/reticulatedpines/magiclantern_simplified), pinned commit `142472e95054d73640f50789ac0fd66325499d82`. Core, build system, headers and native shooting engine retain upstream notices and GPL licensing. Full GPL v2 text is in `LICENSE`; prepared source is included in the source release. Existing upstream per-file and library licenses remain applicable.
- **Chinese translation and CRBF integration**: [ChaseChanCN/magiclantern](https://github.com/ChaseChanCN/magiclantern), commit `a41f406a6c50b998089fa65dc1d70164d2da40b8`. Vendored files, GPL text and blob hashes are retained under `localization/vendor` and `localization/vendor-manifest.json`. CamDesk revises translations and adds validation/loaders.
- **Inter Display Bold 4.1**: [Inter](https://github.com/rsms/inter), SIL Open Font License 1.1. Original font, license and checksums are under `ui/fonts`. The derived bitmap face is named **CamDesk Neutral Bold**. Its license accompanies the card fonts.
- **WenQuanYi Micro Hei**: generated Chinese glyphs and the fallback 40px font use the open font, with the full supplied copyright/license record in `localization/fonts/FONT-LICENSE.txt`. Font regeneration uses the system `fonts-wqy-microhei` package. Do not treat these glyphs as proprietary Hasselblad fonts.
- **Visual reference**: Hasselblad-inspired layout; CamDesk draws its own geometry and icons. No Hasselblad logo, proprietary font or user reference photograph is distributed. Canon, Hasselblad and Magic Lantern names identify platforms or inspiration, not endorsement.

CamDesk新增源码采用GPL-2.0-or-later；上游代码、词库及字体保留其原有授权与署名。发布成品时请同时提供对应源码包和本声明。源码包包含本次50D构建所用的已应用修改的上游源码子集、生成代码及构建脚本；完整上游仓库和固定提交在上方列明。
