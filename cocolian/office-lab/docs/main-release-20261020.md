# main 合并版本发布（2026-10-20）

引擎源码提交：053c8491238（main）。在 codex-dev 中使用既有 Debian 13 构建容器和匹配的编译缓存，增量执行 `make -j2 sw oox`，随后执行 `CppunitTest_sw_layoutwriter5` 的 `testWordImportTypography` 和 `testWordTableGridProportional`。构建容器限制 2 CPU、10 GiB 内存。测试使用已有 Liberation、Calibri 和中文字体副本；字体二进制不提交 Git。

编译及测试完成后复制 libmergedlo.so、libswlo.so、libsw_writerfilterlo.so，检查 ELF 头和文件大小，再打包。不能在链接任务运行时复制引擎库。构建和部署入口见 scripts/releases。

本次以发布前各服务镜像为基线更新引擎，保留已有浏览器资源、字体、字体配置和转换接口：

- 普通编辑器基线：collabora-source@sha256:ebea1969bd81b0ccae5d3de9a3d29cd692b4437247e357a80b8251422518f025。
- Word Mac 编辑器基线：collabora-wordmac@sha256:bb99082a04c79f432f1d8f9046356a24b35252daa820c872ffce4854a9f09f12。
- 转换服务基线：libreoffice-source@sha256:3826f010d4e25156ae19904a74a5ccbb25d61eec395450c24b6a0fd43cfceffb。

上述基线不代表浏览器及运行时从本次 main 全量重建。门户未重建，保留线上版本及文档卷。

候选验证：两项布局回归测试通过；Alice/Bob 双会话启动及各自输入后的 WOPI 保存通过；普通样例、word-import-typography.docx、word-table-grid-proportional.docx 转 PDF 均返回 200；Word Mac 编辑器发现接口就绪。以上不等同于全部文档与 Word 像素级一致性验收。

镜像推送 res.cocolian.cn 后校验摘要，在宿主机预拉取，按不可变摘要串行更新 office-lab。Deployment 原配置及镜像保存在 codex-dev 的 artifacts/main-053c849-20261020；失败恢复原模板，不删除 PVC。其余 Namespace 只读。

集群内 Word Mac 发现接口为 `/word-mac/hosting/discovery`，不能使用普通实例的根路径；检查根路径会返回 400。首次发布因检查路径不匹配触发自动回滚，修正验证路径后重试。
