# Cocolian Office Lab

这里保存 Cocolian Office 的集成、构建和部署实现。项目目标是构建国产化的 LibreOffice 和云文档中台，持续提升中文文档在在线编辑、预览、PDF 转换中与 Microsoft Office Word 的渲染一致性。

项目开源，可按各组件许可证免费使用和接入。欢迎贡献代码、测试及系统集成；欢迎捐款支持，可通过 [GitHub Issue](https://github.com/cocolian/online.mirror/issues) 联系项目维护者。

## 当前源码入口

- [apps/office-portal](apps/office-portal)：文档门户与 WOPI Host。
- [deploy/source-build](deploy/source-build)：构建依赖、引擎编译与运行镜像配方。
- [scripts/devops](scripts/devops/README.md)：main 分支变更检测、镜像构建和自动部署流程。
- [项目介绍](docs/community/introduction-zh.md)：项目目标、接入和参与方式。

复杂文档兼容性仍在持续改进。第三方字体由部署方按授权提供；真实业务样本、生成的 PDF、凭据和构建缓存不应提交到仓库。

## 早期开发与发布记录

以下记录保留早期版本的背景；当前分支与构建行为以代码和上述自动部署说明为准。

版本 office-ui-v2026.10.17 记录中文补全、Word 风格标签栏和编辑页固定顶部修复。

- 仓库根目录的 browser/ 已直接包含 Collabora 修改。
- 此目录 apps/office-portal/ 是门户与 WOPI Host；运行密钥从 LAB_KEY 环境变量读取。
- scripts/、deploy/source-build/ 保存构建工具与配方。
- docs/ 保存验证结果和历史发布摘要；release.json 记录本次已部署版本。
- patches/collabora/zh-cn-native-commands.patch 是额外翻译仓库的中文补丁；先获取固定翻译提交，再应用该补丁。浏览器的两个补丁仅用于追溯，当前分支已经应用，不应重复应用。

开发机原工作区为 <workspace>/cocolian-office，Collabora 位于 src/online.mirror；门户构建工作区为 <workspace>/cocolian。构建脚本保留这些默认位置，可参考脚本中的环境变量调整。此处不包含源码编译缓存、镜像归档、文档数据卷、访问密码、认证文件或 SSH 密钥。

该分支为既有在线部署的源码版本记录；没有更改 GitHub 默认分支，也不触发再次部署。后续修改仍先提交源码，再构建新镜像并验证发布。

最新版本 office-ui-v2026.10.17-network 增加标题栏连接状态、编辑 WebSocket 往返探测和独立保存状态，见 docs/connection-health-20261017.md。

最新界面：连接状态已移到底部语言选项旁，详情向上展开。参见 docs/connection-statusbar-20261017.md。

样式控件外观：顶部独立预览卡片、底部名称栏；侧栏统一紧凑预览列表。仅修改 CSS，详见 docs/style-gallery-20261017.md。

打开文件：保存左侧入口支持本地上传与共享知识库，目录为持久卷 /data/knowledge-base。参见 docs/file-open-20261017.md。
