# Cocolian Office Lab

这里保存 Cocolian Office 的集成、构建和部署实现。项目目标是构建国产化的 LibreOffice 和云文档中台，持续提升中文文档在在线编辑、预览、PDF 转换中与 Microsoft Office Word 的渲染一致性。

项目开源，可按各组件许可证免费使用和接入。欢迎贡献代码、测试及系统集成；欢迎捐款支持，可通过 [GitHub Issue](https://github.com/cocolian/online.mirror/issues) 联系项目维护者。

## 当前源码入口

- [apps/office-portal](apps/office-portal)：文档门户与 WOPI Host。
- [deploy/source-build](deploy/source-build)：构建依赖、引擎编译与运行镜像配方。
- [scripts/devops](scripts/devops/README.md)：main 分支变更检测、镜像构建和自动部署流程。
- [项目介绍](docs/community/introduction-zh.md)：项目目标、接入和参与方式。

复杂文档兼容性仍在持续改进。第三方字体由部署方按授权提供；真实业务样本、生成的 PDF、凭据和构建缓存不应提交到仓库。
