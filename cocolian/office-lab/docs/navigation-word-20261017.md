# Writer 导航栏图标标签

去掉导航标题，将原有大纲、结果标签改为列表和搜索图标，保留中文悬浮提示及无障碍名称。标签与关闭按钮同一行，搜索框仅在搜索结果标签显示，大纲标签隐藏；打开大纲时不再聚焦隐藏的搜索框。图标按参考图调整为单色圆点列表与细线放大镜。保留原有事件、键盘切换、搜索与大纲跳转逻辑；仅作用于 Writer。

验证：独立 WOPI 文档启动，图标及标题隐藏、中文标签、方向键焦点切换、结果标签选中、大纲内容及双击、关闭、编辑和 WOPI 保存通过。见 scripts/verify-navigation-icons.py。构建完成，镜像推送并由 k3s 节点预拉取后，仅更新 office-lab/collabora；capabilities/discovery 均返回 200，UI revision 5f84842d99。DOCX 转 PDF 返回 200。发布前后其他 11 个 Pod UID、镜像、就绪及重启次数不变。

回滚镜像：res.cocolian.cn/cocolian/collabora-source@sha256:6d6856cc2a22b6e41b71fee8d9c5f0d0412da86da5d391a2237d0d6a9d3e7ddd。保留 Deployment 快照及 rollback-patch.json 于开发机 artifacts/nav-word-20261017。
