# 中文开源字体

> 地址和目录均为通用示例，实际部署值应由自己的私有配置提供。


添加 Adobe 官方思源宋体、思源黑体简体中文常规和粗体，保留 Noto CJK。4 个 OTF 总计约 80 MiB，字体不放入 Git；来源提交、SHA256、大小见 fonts-lock.json，保留 OFL 许可证。

开发机字体及构建上下文：<workspace>/cocolian-office/artifacts/fonts-20261017/fonts。下载脚本 scripts/download-source-han-fonts.py 可使用该目录现有 fonts-lock.json 锁定下载；复现时先将本目录 fonts-lock.json 复制到该构建目录。

Fonts.Dockerfile 用于转换服务，CollaboraFonts.Dockerfile 同时将字体复制到 systemplate。基础镜像为本次发布前已验证镜像；精确 Deployment 镜像见开发机 artifacts/fonts-20261017/deployments-before.json。字体缓存刷新后以 1001 运行，不在 Pod 内临时安装。

验证：候选编辑器启动、中文字体显示、WOPI 保存通过；DOCX 转 PDF 返回 200，pdffonts 确认 4 个字体均嵌入；上线后校验两服务字体 SHA256、能力接口、转换 PDF。仅替换 office-lab/collabora 和 libreoffice 镜像，其余 10 个 Pod 不变。rollback JSON 与完整 Deployment 快照位于上述 artifacts 目录。

此版本没有新增方正、微软商业字体，没有将仿宋或楷体强制映射到思源字体，也没有修改默认字体和既有文档。
