# 样式控件显示优化

> 地址和目录均为通用示例，实际部署值应由自己的私有配置提供。


参考 Word 样式库，顶部预览采用等宽卡片，保留实际文档样式预览，名称放在独立底栏，增加细边框与选中状态。侧栏采用紧凑列表，限制预览图片高度，保留原有的层级缩进和展开控件。

代码变更仅涉及 browser/css/notebookbar.css 与 browser/css/jssidebar.css，不修改样式命令、条目数据、事件、筛选逻辑、预览生成或文档格式。移动端顶部控件沿用既有布局。

发布仅更新 office-lab/Collabora，保留原 Deployment 配置、原镜像摘要和文档卷。源代码基于 19b4efae7fb8e9eda4a6d88baa959c838d3eb8c3，中文翻译版本不变。

验证完成：顶部应用标题 1、侧栏展开并应用标题 2、已使用样式筛选、预览开关；1440/1024 宽度截图复查；隔离文档 Alice/Bob 编辑器启动与 WOPI 保存；线上 DOCX 转 PDF 返回 200。线上 capabilities/discovery 返回 200，资源版本 9546bd8ff2。其余 11 个 Pod 的 UID、镜像、就绪状态和重启次数保持一致。

镜像：`registry.example.invalid/cocolian/collabora-source@sha256:ef8795215fd0c37a4078c65a2e424b615cbace488d458bdd8b556aabcffe12d4`。
