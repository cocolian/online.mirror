# 字体中文名称和分类

引擎提供真实字体族名，用独立显示层呈现中文名：FangSong_GB2312 → 仿宋_GB2312，KaiTi_GB2312 → 楷体_GB2312，SimHei → 黑体，Source Han Serif/Sans SC → 思源宋体/黑体。字体文件和文档中的实际名称不变。

按仿宋、楷体、宋体/东亚衬线、黑体/东亚无衬线、西文衬线、西文无衬线、等宽、书法装饰、符号分类。未知字体保留于其他分类。分类标题不参与选择；过滤时隐藏空分类。保留源位置索引，显示名输入和自动补全会解析回引擎真实名称。搜索支持中文名称、英文名称及原编号。

C059 是 Century Schoolbook 风格衬线字体，P052 是 Palatino 风格衬线字体，Z003 是 Chancery 西文书法风格字体。可参考 URW Base35 字体项目 fontconfig 映射：https://github.com/ArtifexSoftware/urw-base35-fonts/tree/master/fontconfig 。

验证：标签栏、紧凑工具栏；9个分类且同组连续；中文和英文搜索、C059说明和编号搜索；无结果时隐藏分类；方向键与回车跳过标题；中文显示名输入后实际引擎状态仍为 SimHei/C059；原字体选中、导航、编辑和WOPI保存通过。上线能力接口返回新revision，DOCX转PDF回归通过；其他11个Pod不变。

仅更新 office-lab/collabora，不更新字体文件、转换镜像或文档卷。原镜像、Deployment模板、回滚JSON、测试截图和结果保存在开发机 artifacts/font-groups-20261018。
