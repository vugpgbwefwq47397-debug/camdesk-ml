# v0.4.2-hui — CamDesk for Magic Lantern

## 中文

面向Magic Lantern的参数界面与拍摄工具，目前仅以EOS 50D开展开发和实机测试。**本次二进制成品适用于EOS 50D／佳能固件1.0.9（50D.109）；其他机型需移植和重新编译。**

- 恢复哈苏参考风格的分格、白平衡／对焦标牌、主体与存储图标、AE-L、曝光刻度和EV。
- 接入拍照／实时取景白平衡、色温、曝光补偿、驱动和测光属性。
- 保留四项大字取景栏及光圈不可读时的M.AP显示。
- 修复字库整块读取触发FIO_ReadFile断言、调试文字覆盖界面的问题；改用8KB DMA分块读取并处理短读和错误。
- 不可用功能以灰色图标保留，不模拟功能启用状态或电量百分比。

ARM核心以FATAL_WARNINGS=y编译；配套模块12个接口匹配。界面、字库损坏、DMA／短读／EOF／错误／分配失败、读数、菜单导航、中文及间隔拍摄配置回滚测试通过。0.4.2已写卡并读回核验；**最新实机回归尚待完成**。

## English

A Magic Lantern interface and shooting toolkit, developed and camera-tested only on EOS 50D so far. **These binaries target EOS 50D with Canon firmware 1.0.9 (50D.109). Other cameras require porting and rebuilding.**

- Restores reference-style cells, WB/focus badges, subject/storage icons, AE-L, exposure scale and EV.
- Reads photo/LiveView WB, color temperature, compensation, drive and metering properties.
- Retains four large LiveView exposure fields and the M.AP fallback.
- Fixes the oversized cached font read that triggered an FIO_ReadFile assertion and console overlays; uses 8KB DMA chunks with short-read/error handling.
- Keeps unavailable functions as gray visual placeholders, without simulated activation or battery percentages.

The ARM build passes FATAL_WARNINGS=y and all 12 module interfaces resolve. Host checks cover UI/font validation, DMA and I/O failures, readings, navigation, Chinese localization and interval setup/rollback. Card write/readback passed. **Latest camera regression remains pending.**

## Attachments / 附件

Attach the `card.zip`, matching `source.zip`, optional `github.zip` and `SHA256SUMS.txt` together. The source archive contains the prepared 50D source tree and build instructions; the GitHub repository archive contains the clean project files for an initial upload. Existing Magic Lantern installation is required for the card package. See INSTALL.zh.md / INSTALL.en.md.
