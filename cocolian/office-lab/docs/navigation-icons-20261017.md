# Writer 导航栏图标标签

去掉导航标题，将原有大纲、结果标签改为列表和搜索图标，保留中文悬浮提示及无障碍名称。标签与关闭按钮同一行，搜索框位于下方。保留原有事件、键盘切换、搜索与大纲跳转逻辑；仅作用于 Writer。

验证：独立 WOPI 文档启动，图标及标题隐藏、中文标签、方向键焦点切换、结果标签选中、大纲内容及双击、关闭、编辑和 WOPI 保存通过。见 scripts/verify-navigation-icons.py。构建完成，镜像推送并由 k3s 节点预拉取后，仅更新 office-lab/collabora；capabilities/discovery 均返回 200，UI revision a612f3f22e。DOCX 转 PDF 返回 200。发布前后其他 11 个 Pod UID、镜像、就绪及重启次数不变。

回滚镜像：registry.example.invalid/cocolian/collabora-source@sha256:022c426f8c16e08e4e1478a1558341579bc5e852d4eaa43c059d8c4dcbd5f4a3。保留 Deployment 快照及 rollback-patch.json 于开发机 artifacts/nav-icons-20261017。
