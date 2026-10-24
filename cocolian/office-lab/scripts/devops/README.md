# build-host 自动构建与部署

按项目独立触发与部署：

| main 分支发生更新的项目 | 重新构建、推送、部署的模块 |
| --- | --- |
| cocolian | wopi（门户） |
| cocolian-office | collabora、collabora-wordmac、libreoffice |
| online.mirror | collabora、collabora-wordmac、libreoffice |

任意代码提交都会触发对应模块，不依赖外层项目更新子模块固定提交号；同时变更取模块并集。未选中的模块不构建、不更新、不重启，并核对其镜像、模板和 Pod 保持不变。手动 --force 且提交未变时可重建全部模块。

监听 GitHub 的 `cocolian/cocolian`、`cocolian/cocolian-office`、`cocolian/online.mirror` 三个库的 `main`。服务器主动每分钟拉取远程提交号，稳定 45 秒后触发；无需公网 Webhook、GitHub Actions Runner 或 GitHub 写入 Token。首次启用会构建当前版本；之后只有新提交或手动强制运行才构建。

源码归属：门户使用 cocolian/main；渲染引擎、coolwsd 和 browser 使用 online.mirror/main；字体配置、编辑辅助标记和转换适配器使用 cocolian-office/main。这里明确采用引擎 main，不使用外层旧的 submodule 提交号；每次运行的 commits.json 锁定三个实际提交。其他 SDK/office-core 子模块不参与现有服务构建。

流程：独立自动化工作区拉取固定提交 → 按源文件变更复用 Debian 13 构建缓存增量编译（3 CPU / 12 GiB，执行 30 个严格字体布局测试和 26 个私有字体测试）→ 构建本次选中的运行时镜像 → 经加密 SSH 隧道推送 registry.example.invalid → 在现有 k3s 节点串行启动无 Service 标签的候选 Pod 并转换合成 DOCX → 按不可变镜像摘要串行更新 office-lab → 检查登录、浏览器入口、普通/WordMac 编辑器及转换器导出、WOPI 锁定保存读取解锁 → 比较其他命名空间 Pod → 记录成功。未变化的引擎或浏览器复用已验证的编译结果，仍重新打包和发布镜像。发布不向 GitHub 回写，避免循环触发。

只修改 office-lab 中本次选定模块的镜像，保留 gateway 配置、Secret、文档 PVC、字体及既有运行参数。不会自动应用仓库中的任意 Kubernetes YAML 或修改命名空间/入口配置。新建另一套服务需要单独维护部署模板。失败不更新 last_success；部署后失败自动恢复本次已修改的 Deployment 模板；存在并发修改时停止覆盖并记录待人工处理原因。候选 Pod 为本流程临时资源，验收后删除；仅删除本次创建的 UUID 命名测试文件，不修改业务文档。不会删除历史镜像/PVC，导出中间 tar 在推送后删除。

服务器安装位置：

- systemd: `/etc/systemd/system/cocolian-devops.service`、`.timer`，以 构建用户执行，开机自动启动。
- 监听器：`~/.local/lib/cocolian-devops/watch.py`；仅在成功发布后采用仓库中的监听器更新。
- 状态和日志：`~/.local/share/cocolian-devops/state.json`、`runs/<时间-提交摘要>/build.log`、`commits.json`、`images.json`、`deployments-before.json`、`checks.json`、`release.json`。失败还有 `rollback.json`。
- 私有配置：`~/.config/cocolian-devops/config.json`（初始引擎基线 053c8491238）；registry-auth.json 为 0600，复用现有仓库凭据，永不提交 Git。集群操作使用现有 office-lab 受限 kubeconfig；不新增集群管理员凭据。
- 编译缓存：`~/projects/cocolian-office/build/source-19b4efae7fb8`。自动化不 reset 人工工作树；人工编译同一缓存前先停 timer 并确认 service 已结束。不能在 make 链接过程中复制运行库。

```sh
sudo systemctl status cocolian-devops.timer cocolian-devops.service
journalctl -u cocolian-devops.service -n 80 --no-pager
cat ~/.local/share/cocolian-devops/state.json
# 检查当前 main（仍有防抖、无变化不重复发布）
sudo systemctl start cocolian-devops.service
# 强制重试当前提交；与定时任务共用互斥锁
python3 ~/.local/lib/cocolian-devops/watch.py --force
# 停止未来的触发，不中断已运行的发布
sudo systemctl stop cocolian-devops.timer
```

失败后同一组提交 30 分钟后重试；新提交会重新防抖。GitHub 无新提交时保持安静。此流程不是 GitHub Actions，因此 GitHub Actions 页面不会出现运行记录，应查看上述服务器状态和日志。

安装（仅 build-host）：将 watch.py 复制到上述用户目录，将 `cocolian-devops-system.service` 安装为系统 `cocolian-devops.service`、`cocolian-devops-system.timer` 安装为 `.timer`，准备私有配置后执行 `sudo systemctl daemon-reload && sudo systemctl enable --now cocolian-devops.timer`。不要重复安装草稿用户级服务。

镜像传输：公网反向代理下载大层速度较慢，宿主机安装 `cocolian-registry-tunnel.service`，使用现有宿主机 SSH 身份连接 VM，将 VM 的 `127.0.0.1:15000` 反向转发到宿主机回环流式代理，再访问现有 registry Service。仅本机回环监听，公网不开放端口，传输受 SSH 加密；该回环端点使用 HTTP，公共 registry.example.invalid 的 TLS 校验保持启用。两端重启后 systemd 自动重连。私有 config.json 设置 registry_endpoint=127.0.0.1:15000；业务镜像始终记录公共域名与不可变摘要。

宿主机的 `cocolian-registry-proxy.service` 仅监听 127.0.0.1:15001，将仓库返回的绝对上传 Location 地址改回 VM 回环端口，避免上传绕回公网；请求和响应以有界分块转发，支持 Content-Length 与 chunked 上传。不修改 applications 命名空间中的仓库配置。代理与隧道均为宿主机 systemd 开机服务。

### Mac Word rendering profile

Renderer releases install the private font pack pinned by
`office-fonts-manifest.json` from
`~/.local/share/cocolian-devops/fonts/office`. Font binaries are never stored in
Git. A missing file or a mismatched SHA256 stops the build before image creation.
All three renderers receive the same fonts and fontconfig profile. Candidate
pods must select native FangSong_GB2312, KaiTi_GB2312, SimSun, Microsoft YaHei,
FangSong, STZhongsong and Times New Roman before deployment.

`COCOLIAN_WORD_MAC_METRICS=1` enables the Mac Word profile. It excludes the 127%
Windows CJK enlargement when choosing grid multiples, uses measured native-font
non-grid metrics where enabled, and uses the measured Mac Word behavior for
proportional spacing on multiple grid rows. Without this variable, the existing
Windows Word metrics remain active. The scoped `testWordBodyGridProportional`
regression exercises the Mac profile without changing the rest of the test
process. Renderer builds also retain the existing typography, table grid and
ODF compatibility tests.

`government-corpus.json` records the six government download URLs, immutable
input hashes, actual Mac Word reference page counts, and the measured 21-case
line-pitch matrix. This corpus is **not accepted** yet: equal page counts do not
prove equal pagination or visual layout. The renderer fixes are independently tested; full corpus acceptance remains separate
from releasing those repairs. Residual differences must stay visible in private
validation reports instead of being inferred away from equal page counts.

The Mac profile supplies an eight-point Normal-template paragraph after-gap
when the DOCX omits `pPrDefault`. This was confirmed in Mac Word paragraph
settings and PDF exports of the Jining and Beijing government samples.
`testWordMacMissingParagraphDefaults` checks disabled, enabled, and explicit-zero
behavior using font-independent synthetic fixtures. Government originals are
never normalized or rewritten as part of rendering.

The GB2312 Mac profile uses a measured 1.3-em ordinary line pitch and avoids
adding external leading twice. Grid-row selection retains external leading,
including the 14pt two-row boundary. `government-corpus-10.json` records ten
additional unmodified government DOCX sources with 90 actual Word reference
pages. The independent PDF matrix covers 21 grid cases and 42 non-grid GB2312
cases (FangSong and KaiTi). Maximum pitch differences are 0.12pt and 0.24pt.
This does not constitute full rendering acceptance: table pagination, final
empty paragraphs, line wrapping, and footer formatting still differ.

Explicit run page breaks override `pageBreakBefore=false` in the paragraph context.
`testWordManualPageBreakOverridesParagraphDefault` generates its input at runtime
and requires two pages; a false paragraph default cannot cancel the manual break.

Mac Word also retains proportional leading in single-grid-row table text.
`testWordMacSmallTableGridProportional` checks both the unchanged default profile
and the Mac profile against a generated 12pt, 150% table paragraph.

Real GB2312 synthetic bold is tested separately with the private pinned fonts:
`testWordMacGbBoldTableWrap` requires four lines in a narrow 12pt bold cell,
matching the Mac Word export; the default profile retains its two-line layout.
The twelve private-font tests allow non-application fonts via the make command line. The 21 bundled-font regression
cases remain strict. Advance arrays retain fractional precision and exclude
whitespace from the measured 40/2048-em bold expansion.

Missing theme parts use the observed current Chinese Mac Word defaults in the optional Mac profile (Aptos/Aptos Display and DengXian/DengXian Light). Empty paragraph style references retain the normal style so that direct spacing is applied. Both cases generate their DOCX input at test runtime. Private runtime fonts remain outside Git.
Explicit DOCX punctuation compression is imported into CharacterCompressionType in the Mac profile and covered by a generated regression package.
The native-font punctuation regression follows a Mac Word PDF probe at seven cell widths in both GB2312 and YaHei, including the half-em boundary at 200pt.
Automatic Latin/CJK and digit/CJK spacing uses the 1/4-em gaps measured in Mac Word, rather than the legacy 1/5-em gaps. A generated 35pt GB2312 cell checks the changed wrap boundary.

The Mac profile respects doNotUseEastAsianBreakRules for automatic paragraphs aligned to a character-and-line grid. A lines-only grid retains punctuation compression. Both the compression option and this compatibility flag survive DOCX save/reload. Generated native-font regressions check lines-only automatic/fixed spacing and the round trip.
Mac Word renders OOXML inset/outset borders as single strokes, including colored borders. A generated bundled-font test preserves the legacy profile and checks the Mac border style, width and color. Oversized percentage tables retain their preferred width so the existing grid-width fallback can preserve their size.

The first continuous section initializes its page grid in the Mac profile. Native SimSun/YaHei Word probes additionally cover 24 grid cases: YaHei retains CJK leading for grid-row selection, with a two-row boundary at 10pt. The private grid regression checks the 9pt/10pt boundary. Literal generic font names (`sans-serif`, `monospace`) use the observed Times New Roman/SimSun fallback; explicit Helvetica and Menlo remain native.

The private font pack now pins 41 files, including Aptos, Aptos Display, Segoe UI,
Helvetica, Menlo, Wingdings and Apple Color Emoji. Fontconfig preserves explicit
fonts and uses the observed Mac Word fallback for generic family names and symbols.
Binary fonts and document samples must never be committed.

The optional `COCOLIAN_WORD_MAC_AUTOFIT=1` profile uses content minima and preferred
percentages for flat, non-floating AutoFit tables with uniform unmerged rows.
Merged tables retain their existing column-sizing algorithm. Explicitly indented or centered
percentage tables in pre-2013 OOXML compatibility modes include their outer cell
padding outside the preferred text width. This applies independently of AutoFit
and preserves the modern-mode and disabled-profile behavior. The first resize
recalculates after paragraph defaults resolve. Short ASCII tokens are kept intact
only when all column minima fit the available width; long URLs can still wrap.
Generated fixtures cover noWrap import/export, an omitted font size, short tokens,
and a long URL that must not widen the page.

`COCOLIAN_WORD_MAC_GRID_BOTTOM=1` permits unused lower grid leading at the page
end. It preserves glyph positions, excludes inline objects, and does not subtract
padding twice. Generated native-font positive/negative cases require a last pair
of text lines to fit, and a further 5pt to force the page break. The fixed-grid
baseline independently follows Word's 80% rule.

A private 15-document corpus has 229 actual Mac Word PDF reference pages. It is
not a binary test fixture and is not stored here. Page counts, unique-line page
positions, baseline coordinates and side-by-side page images are recorded locally.
Matching page counts alone must never be reported as full visual acceptance.

`COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS=1` makes the normal quarter-em Latin/CJK
and digit/CJK gaps shrinkable. Remaining line space first restores these gaps,
then expands compressed punctuation. An actual Word 35pt-cell probe requires
2.5pt gaps between 12pt Chinese text and a digit; the generated native-font
regression checks both glyph positions. `COCOLIAN_WORD_MAC_LINE_BUDGET=1`
limits opportunistic line extension by the next break unit and excludes Japanese
kana compression. Independent Word probes cover six mixed-script widths and
fourteen multi-punctuation cases. These flags accompany the server Mac profile;
legacy Writer formatting remains the default without the profile.

`COCOLIAN_WORD_MAC_CLASSIC_METRICS=1` extends the measured 1.3-em ordinary
line pitch to SimSun/NSimSun, SimHei, FangSong and KaiTi, including Chinese family
aliases. Grid selection retains external leading, preserving the 12pt/14pt
SimSun one-row/two-row boundary. Known YaHei and DengXian families use measured
centered CJK leading; unknown families retain the previous behavior. Six actual
Word baseline/pitch probes and generated native-font tests cover these paths.
Synthetic bold uses the same 40/2048-em visible-glyph advance for the classic
families and GB2312 fonts; caret, measurement and break widths share the advance.
Font samples and Word exports remain private validation artifacts.

Numbering levels without explicit first-line/left indents inherit the paragraph
style in the Mac profile; synthesized UNO zero defaults must not erase the
style's indentation. Explicit zero list indents retain priority. A generated
fixture checks number glyph positions with the profile disabled/enabled and
with a deliberately explicit zero, without committing customer document text.

The Mac profile resolves character-unit paragraph indents from the first
character's effective font size, with paragraph-mark fallback for empty text.
Inline-only image lines include drawing effect extents and recompute their
natural bounds before grid rounding, preserving vertical centering across
repeated layout passes. Generated, self-contained fixtures exercise both cases;
no private document or font binary is stored in the repository.

The Mac profile derives line-breaking behavior from the document's Word
compatibility mode: Word 2013+ left-aligned body lines keep the measured
extension limit, while legacy documents retain their original break policy.
`doNotWrapTextWithPunct` is imported and exported separately from compression.
Grid metrics use the realized physical font family after substitution; a missing
requested family must not apply the wrong grid pitch to its fallback font.

Fractional synthetic-bold advances are rounded upward for fit decisions, so two
14pt GB2312 bold glyphs do not fit a 570twip cell but do fit 571twips. Grid-bottom
leading is trimmed consistently for split paragraphs, painted text and carets.
Automatic OOXML superscript in the Mac profile uses 65% of the nominal em,
rounded to half-points, with a 28% baseline rise; DOCX export retains semantic
superscript to prevent repeated shrinking on save/reload.

Table layout type and preferred width must survive until the closing table is
handed to its import handler. Clearing them before the callback incorrectly
attached AutoFit to fixed tables and could push columns beyond the page. The
fixed-layout regression covers long noWrap content with AutoFit enabled/disabled.

Mac-profile CJK table punctuation reserves half the closing glyph inside the
cell, while body hanging retains its existing policy. A native-font synthetic
fixture checks the line break and comma caret position in a 3081twip cell.

Floating-object exclusion in the Mac grid profile uses natural glyph/inline
content bounds at the painted baseline, both for the initial width probe and
its underflow recheck. Grid leading must not push a heading below a floating
table. This covers ordinary Word standard-page grids as well as squared grids.
A native-font regression uses a full-width table, a long generic heading and
Word's missing-font fallback with 1.5-line spacing; it checks the PDF baseline.

WordMac AutoFit measures intrinsic column minima without HTML's blanket 1pt
allowance. It rounds separately measured Latin break units outward and allows
one twip for the cell-width conversion. Preferred percentage columns freeze at
their minima before the remaining weights are redistributed, keeping narrow
CJK headers and short ASCII identifiers on Word-compatible lines.

Centered modern grid headings with doNotWrapTextWithPunct use the strict line
budget instead of borrowing half an em. A generic 19-character synthetic bold
SimSun fixture toggles that option and checks the resulting one/two-line split.
The private corpus remains a layout comparison, not a full visual acceptance.

Explicit OOXML `wordWrap` values are preserved for paragraphs and paragraph
styles, including `w:val="0"` on export. In the Mac profile, false permits a
space-delimited ASCII word to break at the character that physically fits.
`testWordMacWordWrap` verifies true, false and inherited false before and after
DOCX save/reload with namespace-aware assertions. See the
[OOXML WordWrap definition](https://learn.microsoft.com/en-us/dotnet/api/documentformat.openxml.wordprocessing.wordwrap?view=openxml-3.0.1).

Closing-punctuation fitting treats decimal and percentage tokens as complete
units across script portions. The generated native-font
`testWordMacPercentageClosingUnit` covers a percentage followed by a CJK closing
parenthesis. The pipeline now runs 32 strict bundled-font and 26 native-font
regressions. These individual checks do not establish full corpus equivalence.

The Mac profile additionally renders explicit uniform positive row-level cell
spacing for flat, top-level, non-floating tables with unmerged rows. Cell content,
border rectangles and minimum row heights include that spacing. Other table
models retain their existing layout. Explicit row spacing (including zero) is
preserved as namespaced DOCX attributes for both profiles. The generated
`testWordMacRowCellSpacing` checks zero/positive values, the disabled profile,
horizontally merged rows and save/reload geometry. Only dxa units are accepted,
as specified by the
[OOXML row cell spacing definition](https://learn.microsoft.com/en-us/dotnet/api/documentformat.openxml.wordprocessing.tablecellspacing?view=openxml-3.0.1).
Table-level default spacing and mixed row-spacing layouts remain unsupported.

The Mac profile keeps an ASCII HTTP(S) path tail together after a hyphen when
the complete tail fits the next line. Explicit `wordWrap=false` still uses
character-level wrapping, including underscores in identifiers. The generated
`testWordMacUrlBreakUnits` covers plain text and HYPERLINK fields, legacy and Mac
profiles, long-tail fallback, and DOCX save/reload with reserved example.invalid
addresses. Private sample documents and font binaries remain outside Git.

Left-aligned and centered table text snapped to a strict line grid now obeys the Word 2013
punctuation setting, including whole-portion fitting before line breaking.
`testWordMacStrictGridTablePunctuation` checks the enabled/disabled setting
and DOCX save/reload with pinned Segoe UI and SimSun fonts.
The private font pack additionally pins native Courier New and Tahoma faces.
Missing Chinese Tahoma glyphs prefer SimSun; missing FangSong_GB2312 glyphs
prefer Courier New. Installed primary faces remain first in the pattern.
Font binaries and sample documents remain outside Git.

In the Mac profile, PDF export selects artificial bold for each actual glyph
font, including fallback faces. A native bold primary face no longer suppresses
needed synthesis on its fallback, and native bold fallback glyphs avoid a
second stroke. The generated `testWordMacPdfFallbackBold` checks mixed Latin/CJK
fonts, horizontal/vertical layout, outline text, and the unchanged default profile.

Cached body-grid paragraphs rebuild their position-dependent line heights when
they move. Fitting and remaining-height accounting use the same lower-leading
allowance. `testWordMacCachedGridBottom` checks backward movement after editing,
nonnegative remaining space, and a visible-font-box overflow that must still
break the page. Tables, floating frames, and the default profile are excluded.

Publishing keeps one temporary Docker archive and streams OCI conversion to the
registry, avoiding a second complete local OCI archive. The resulting registry
digest, runtime configuration, OS/architecture, and every uncompressed layer
identity are checked before candidate deployment. Base-image reuse requires
identical registry configuration and layer identities. Temporary archives are
removed on success or failure; original documents and private fonts stay outside
Git. Ten release-flow unit tests cover these gates and the existing rollback
and cache-provenance checks.

Top-level Mac-profile AutoFit tables stay within the page text width unless
OOXML `growAutofit` explicitly permits margin overflow. That compatibility
setting and the AutoFit layout mode survive DOCX save/reload. The generated
`testWordMacAutoFitPageWidth` checks both settings through two save/reload cycles,
including wrapping oversized content inside the page. Legacy-profile and fixed
table behavior are unchanged. Private sample documents remain outside Git.

Runtime COPY inputs receive the existing read/execute permission contract in the
private build context. Image recipes no longer run recursive chmod over the
installed engine, avoiding duplicate full-engine layers. Source files and
symlink targets outside that context keep their original permissions.

The Mac profile also keeps an ordinary space-delimited ASCII word intact when
a CJK break iterator cuts after its first character, provided the complete word
fits the next line. Explicit character-level wrapping, fields, hyphenation, URLs
and longer tokens retain their prior behavior. `testWordMacWordWrap` covers
English/Chinese language tags, both kinsoku flags, two widths, direct/inherited
and omitted wordWrap values, and DOCX save/reload.

Horizontal body text fitting one grid row now centers glyphs in its full
proportional pitch. This also applies to the first line, as in table cells,
while multi-row text retains its existing grid pitch.
`testWordMacSmallBodyGridProportional` verifies three-line paragraphs, the
unchanged legacy profile, and DOCX save/reload with generated documents.

Justified-line probes can measure only a prefix of a word. The Mac wordWrap
check may examine the remaining characters when the font attributes and script
stay uniform; it still checks that the whole word fits a new line. Generated
native-font cases cover normal and explicit character wrapping through DOCX
save/reload, and bundled-font cases cover both left and justified alignment.
