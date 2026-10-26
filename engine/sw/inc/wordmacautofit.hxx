/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#pragma once
#include <sal/types.h>
#include <swdllapi.h>
#include <vector>
class SwTable;
class SwFrameFormat;
namespace sw
{
// Original DOCX percentage, when it still matches the table width setting.
SW_DLLPUBLIC sal_Int32 GetWordMacTableWidthFiftieths(const SwFrameFormat& rFormat);
SW_DLLPUBLIC bool ApplyWordMacFixedPercentColumns(SwTable& rTable,
    const std::vector<sal_Int32>& rColumnPercents);
SW_DLLPUBLIC bool AttachWordMacAutoFit(SwTable& rTable, sal_Int32 nGridWidthTwips,
    const std::vector<sal_Int32>& rColumnPercents, bool bGrowAutoFit = false);
}
