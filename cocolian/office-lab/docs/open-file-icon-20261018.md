# 打开文件图标

将门户 `file-open.svg` 替换为现有 Collabora 官方图标 `upstream/collabora-engine/browser/images/lc_open.svg`，保留原 SVG 内容。上游版权及许可遵循 Collabora 源码中的相应声明。打开按钮、上传及知识库逻辑未修改。

发布版本：office-ui-v2026.10.18-open-icon。
门户镜像：res.cocolian.cn/cocolian/office-portal@sha256:c4e90a07ac09ae88061a44bda5db4f25348498fdd6678f5fbab903ab0df51792。

验证：隔离浏览器环境中按钮位于保存左侧，点击弹窗、本地上传、知识库打开、文档编辑保存和目录持久化通过；线上健康检查通过，图标内容与源码一致；其他 11 个 Pod 的 UID、镜像、就绪及重启状态未变化。仅更新 office-lab/wopi 的 wopi 镜像，gateway 和文档卷保持原配置。
