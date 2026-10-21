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
namespace sw
{
// Opt-in server profile. Default Writer / Windows Word metrics are unchanged.
inline bool UseWordMacMetrics()
{
    const char* pValue = std::getenv("COCOLIAN_WORD_MAC_METRICS");
    return pValue && std::strcmp(pValue, "1") == 0;
}
inline tools::Long WordMacGridHeight(tools::Long nHeight, const OutputDevice& rDev)
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
        // Writer adds 27% of the base height for Windows CJK layout.
        // Mac Word excludes this enlargement when choosing the grid multiple,
        // while retaining the ordinary font metrics outside the grid.
        const tools::Long nBase = rDev.GetTextHeight();
        nHeight -= (nBase * 127) / 100 - nBase;
    }
    return nHeight > 0 ? nHeight : 1;
}
}
#endif
