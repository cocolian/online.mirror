# 字体与字体列表

安装从 jzb joyspace_pwa 提取的 FangSong_GB2312、KaiTi_GB2312、SimHei。按 SHA256 去重补充开发 VM 已安装的 Ubuntu 字体，包括文泉驿、Liberation、URW 与 DejaVu；新增文件及来源校验值见 font-additions.json。保留已有思源与 Noto 字体。

字体列表去掉引擎生成的语言样例预览图，改为统一 13px 名称、紧凑行距；仿宋、楷体、黑体、简体中文常用字体优先，其余按名称排序。每次引擎更新都重排，保留 sourcePos 回传原索引，避免排序影响字体应用。

构建上下文 fonts/ 在开发机 artifacts/fonts-ui-20261018，打包前复制到 build/source-19b4efae7fb8/fonts，并在 .dockerignore 中加入 !fonts/ 和 !fonts/**。Runtime.Dockerfile 在生成 systemplate 之前复制字体；转换服务使用 Fonts.Dockerfile 叠加到原 200MiB 转换镜像。商业字体二进制保留在私有开发和运行环境，不提交到 Git。Ubuntu 字体包版权说明随镜像字体目录保留。

验证：紧凑与标签栏字体列表、排序、三种字体选中及引擎状态、编辑保存通过；中文 DOCX 转 PDF 确认嵌入三个原字体；新镜像先推私有仓库并节点预拉，再仅更新 office-lab/collabora 与 libreoffice。对照发布前后其余10个Pod的UID、镜像、就绪和重启状态。完整快照、回滚JSON、字体清单与测试截图保留于 artifacts/fonts-ui-20261018。

免费字体来源：Google Fonts https://fonts.google.com/ ，Noto https://github.com/notofonts/noto-cjk ，思源 https://github.com/adobe-fonts/source-han-sans 与 https://github.com/adobe-fonts/source-han-serif ，霞鹜文楷 https://github.com/lxgw/LxgwWenKai 。
