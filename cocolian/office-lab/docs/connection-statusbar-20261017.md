# 连接状态移到底部状态栏

连接质量与往返延迟显示在底部状态栏的语言选项旁；点击向上展开详情，保留近期稳定性、保存状态及网络提示。顶部标题栏不再显示连接状态。

沿用编辑 WebSocket 的探测和保存反馈。状态栏创建后挂载控件，兼容连接先于状态栏初始化的情况。仅更新 office-lab 的 Collabora 镜像，保留门户、转换服务与文档卷。

已验证：标签栏与经典视图的底部位置、详情向上展开、真实 WebSocket 探测；隔离文档 Alice/Bob 两个编辑会话启动与 WOPI 保存；线上 capabilities/discovery 返回 200 且资源版本为 383b396742，DOCX 转 PDF 返回 200。其余 11 个 Pod 的 UID、镜像、就绪和重启次数无变化。

镜像：`res.cocolian.cn/cocolian/collabora-source@sha256:4b2aab7235bd212951bdd3a13959d5dbbb25165db71b0e91b42b8300ad760a4d`。
