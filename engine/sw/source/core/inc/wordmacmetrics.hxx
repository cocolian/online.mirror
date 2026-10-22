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
// Actual Mac Word PDF exports of both genuine GB2312 fonts use a
// nominal 1.3-em single-line pitch (21 sizes/spacing combinations each).
// Other fonts retain the existing font-table metrics.
inline tools::Long WordMacGbFontHeight(const OutputDevice& rDev)
{
    if (!UseWordMacMetrics())
        return 0;
    const OUString& rFamily = rDev.GetFont().GetFamilyName();
    if (rFamily.equalsIgnoreAsciiCase(u"FangSong_GB2312")
        || rFamily.equalsIgnoreAsciiCase(u"KaiTi_GB2312")
        || rFamily == u"仿宋_GB2312" || rFamily == u"楷体_GB2312")
        return rDev.GetFont().GetFontSize().Height() * 130 / 100;
    return 0;
}
inline tools::Long WordMacGridHeight(tools::Long nHeight, const OutputDevice& rDev,
                                     bool bAddExternalLeading)
{
    if (!UseWordMacMetrics())
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
        const tools::Long nMacHeight = WordMacGbFontHeight(rDev);
        const tools::Long nAdjusted = nMacHeight > 0 ? nMacHeight : (nBase * 127) / 100;
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
