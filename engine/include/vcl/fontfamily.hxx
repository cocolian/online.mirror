/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the Collabora Office project.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef INCLUDED_VCL_FONTFAMILY_HXX
#define INCLUDED_VCL_FONTFAMILY_HXX

#include <vcl/dllapi.h>
#include <rtl/ustring.hxx>

class OutputDevice;
namespace vcl
{
/// Family of the realized font face, after substitutions; empty if unavailable.
VCL_DLLPUBLIC OUString GetPhysicalFontFamilyName(const OutputDevice& rDevice);
}
#endif
