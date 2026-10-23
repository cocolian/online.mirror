/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef INCLUDED_SW_SOURCE_CORE_INC_WORDMACMETRICS_HXX
#define INCLUDED_SW_SOURCE_CORE_INC_WORDMACMETRICS_HXX
#include <cstdlib>
#include <cstring>
#include <tools/long.hxx>
#include <vcl/outdev.hxx>
#include <vcl/fontcapabilities.hxx>
#include <vcl/metric.hxx>
namespace sw
{
// Opt-in server profile. Default Writer / Windows Word metrics are unchanged.
inline bool UseWordMacMetrics()
{
    const char* pValue = std::getenv("COCOLIAN_WORD_MAC_METRICS");
    return pValue && std::strcmp(pValue, "1") == 0;
}
inline bool UseWordMacClassicMetrics()
{
    const char* pValue = std::getenv("COCOLIAN_WORD_MAC_CLASSIC_METRICS");
    return UseWordMacMetrics() && pValue && std::strcmp(pValue, "1") == 0;
}
inline bool UseWordMacFlexibleScriptGaps()
{
    const char* pValue = std::getenv("COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS");
    return UseWordMacMetrics() && pValue && std::strcmp(pValue, "1") == 0;
}
inline bool UseWordMacGridBottom()
{
    const char* pValue = std::getenv("COCOLIAN_WORD_MAC_GRID_BOTTOM");
    return UseWordMacMetrics() && pValue && std::strcmp(pValue, "1") == 0;
}
// Actual Mac Word PDF exports of both genuine GB2312 fonts use a
// nominal 1.3-em single-line pitch (21 sizes/spacing combinations each).
// Classic families opt in separately; other fonts retain font-table metrics.
inline bool IsWordMacGbFont(const vcl::Font& rFont)
{
    const OUString& rFamily = rFont.GetFamilyName();
    return rFamily.equalsIgnoreAsciiCase(u"FangSong_GB2312")
        || rFamily.equalsIgnoreAsciiCase(u"KaiTi_GB2312")
        || rFamily == u"仿宋_GB2312" || rFamily == u"楷体_GB2312";
}
inline bool IsWordMacClassicCjkFont(const vcl::Font& rFont)
{
    const OUString& rFamily = rFont.GetFamilyName();
    return rFamily.equalsIgnoreAsciiCase(u"SimSun") || rFamily.equalsIgnoreAsciiCase(u"NSimSun")
        || rFamily.equalsIgnoreAsciiCase(u"SimHei") || rFamily.equalsIgnoreAsciiCase(u"FangSong")
        || rFamily.equalsIgnoreAsciiCase(u"KaiTi") || rFamily == u"宋体" || rFamily == u"新宋体"
        || rFamily == u"黑体" || rFamily == u"仿宋" || rFamily == u"楷体";
}
inline bool IsWordMacModernCjkFont(const vcl::Font& rFont)
{
    const OUString& rFamily = rFont.GetFamilyName();
    return rFamily.equalsIgnoreAsciiCase(u"Microsoft YaHei") || rFamily == u"微软雅黑"
        || rFamily.equalsIgnoreAsciiCase(u"DengXian") || rFamily.equalsIgnoreAsciiCase(u"DengXian Light")
        || rFamily == u"等线" || rFamily == u"等线 Light";
}
inline tools::Long WordMacCjkFontHeight(const OutputDevice& rDev)
{
    return UseWordMacMetrics() && (IsWordMacGbFont(rDev.GetFont())
        || (UseWordMacClassicMetrics() && IsWordMacClassicCjkFont(rDev.GetFont())))
        ? rDev.GetFont().GetFontSize().Height() * 130 / 100 : 0;
}
inline double WordMacCjkBoldAdvance(const vcl::Font& rFont)
{
    if (!UseWordMacMetrics() || !(IsWordMacGbFont(rFont)
            || (UseWordMacClassicMetrics() && IsWordMacClassicCjkFont(rFont)))
        || rFont.GetWeight() < WEIGHT_SEMIBOLD)
        return 0;
    // Mac Word synthetic bold adds 40/2048 em to visible classic CJK glyphs.
    // GB2312 size matrices and six-family probes: 12pt -> 12.234pt advance.
    return rFont.GetFontSize().Height() * 5.0 / 256;
}
inline tools::Long WordMacCjkMetricScale(const vcl::Font& rFont)
{
    if (UseWordMacClassicMetrics() && IsWordMacModernCjkFont(rFont))
        return 130;
    const OUString& rFamily = rFont.GetFamilyName();
    // 24 actual Mac Word DengXian / DengXian Light line-pitch cases.
    if (UseWordMacMetrics() && (rFamily.equalsIgnoreAsciiCase(u"DengXian")
        || rFamily.equalsIgnoreAsciiCase(u"DengXian Light") || rFamily == u"等线"
        || rFamily == u"等线 Light"))
        return 130;
    return 127;
}
inline tools::Long WordMacGridHeight(tools::Long nHeight, const OutputDevice& rDev,
                                     bool bAddExternalLeading)
{
    if (!UseWordMacMetrics())
        return nHeight;
    // Unlike the GB2312 and SimSun families, Mac Word retains YaHei's
    // CJK leading when selecting grid rows. Actual 9--14pt Word probes
    // put 9pt in one 15.6pt row, but 10--14pt in two rows.
    const OUString& rFamily = rDev.GetFont().GetFamilyName();
    if (rFamily.equalsIgnoreAsciiCase(u"Microsoft YaHei") || rFamily == u"微软雅黑")
        return nHeight;
    vcl::FontCapabilities aCaps;
    if (rDev.GetFontCapabilities(aCaps) && aCaps.oCodePageRange.has_value()
        && (aCaps.oCodePageRange->test(vcl::CodePageCoverage::CP932)
            || aCaps.oCodePageRange->test(vcl::CodePageCoverage::CP936)
            || aCaps.oCodePageRange->test(vcl::CodePageCoverage::CP949)
            || aCaps.oCodePageRange->test(vcl::CodePageCoverage::CP950)))
    {
        // Remove the CJK leading used for ordinary lines before choosing
        // a grid multiple. GB2312 fonts have a measured Mac-specific pitch.
        const tools::Long nBase = rDev.GetTextHeight();
        const tools::Long nMacHeight = WordMacCjkFontHeight(rDev);
        const tools::Long nAdjusted = nMacHeight > 0 ? nMacHeight : (nBase * WordMacCjkMetricScale(rDev.GetFont())) / 100;
        nHeight -= nAdjusted - nBase;
        // Grid row selection includes the font's external leading even
        // though ordinary Mac GB2312 line pitch already contains it.
        if (nMacHeight > 0 && bAddExternalLeading)
            nHeight += rDev.GetFontMetric().GetExternalLeading();
    }
    return nHeight > 0 ? nHeight : 1;
}
}
#endif
