# GitHub发布步骤 / Publishing to GitHub

1. 新建仓库，建议名 `camdesk-ml`。解压 `github.zip` 到空目录，将其中的项目文件作为仓库根目录上传；不要上传整个开发工作目录。
2. README.md为中文首页，README.en.md为英文介绍。建议仓库描述：`A photography-focused UI and shooting toolkit for Magic Lantern. Developed and tested on EOS 50D; other cameras require porting.`
3. 创建Tag／Release：`v0.4.2-hui`。标题：`CamDesk v0.4.2-hui · Magic Lantern UI · EOS 50D build`。
4. 将 `docs/release/RELEASE.md` 的中英文正文粘贴为Release介绍。当前建议标记 **Pre-release**，因为最新版本实机回归尚未完成。
5. 同时上传 `card.zip`、对应 `source.zip` 和 `SHA256SUMS.txt`；可附 `github.zip`。不要只上传二进制而漏掉匹配源码。
6. 预览图是实际C代码输出的示例读数；请保留该说明。其他机型不要标为“已兼容”，可以邀请移植与测试。

Create a repository (suggested name: `camdesk-ml`), extract the GitHub ZIP into an empty directory and upload those files as its root. Create the `v0.4.2-hui` release using RELEASE.md, mark it as a pre-release while camera regression is pending, and attach the card ZIP, matching source ZIP and checksums. This preparation does not itself publish anything to GitHub.
