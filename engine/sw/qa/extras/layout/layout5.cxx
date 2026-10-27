/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the Collabora Office project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <array>
#include <rtl/strbuf.hxx>
#include <com/sun/star/io/XInputStream.hpp>

#include <swmodeltestbase.hxx>
#include <swtable.hxx>
#include <htmltbl.hxx>
#include <osl/process.h>

#include <com/sun/star/text/XPageCursor.hpp>
#include <com/sun/star/text/SizeType.hpp>
#include <com/sun/star/style/NumberingType.hpp>
#include <com/sun/star/style/LineSpacing.hpp>
#include <com/sun/star/style/LineSpacingMode.hpp>
#include <com/sun/star/i18n/ScriptType.hpp>
#include <com/sun/star/text/XTextFrame.hpp>
#include <com/sun/star/text/XTextSectionsSupplier.hpp>
#include <com/sun/star/text/XTextTable.hpp>
#include <com/sun/star/table/BorderLine2.hpp>
#include <com/sun/star/table/BorderLineStyle.hpp>
#include <pagedesc.hxx>
#include <tgrditem.hxx>
#include <com/sun/star/text/XTextTablesSupplier.hpp>
#include <com/sun/star/text/XTextViewCursorSupplier.hpp>
#include <com/sun/star/linguistic2/XHyphenator.hpp>
#include <com/sun/star/view/XViewSettingsSupplier.hpp>

#include <comphelper/kit.hxx>
#include <comphelper/storagehelper.hxx>
#include <com/sun/star/embed/XTransactedObject.hpp>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/io/XStream.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <comphelper/scopeguard.hxx>
#include <unotools/syslocaleoptions.hxx>
#include <editeng/unolingu.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/viewfrm.hxx>
#include <vcl/scheduler.hxx>
#include <vcl/filter/PDFiumLibrary.hxx>

#include <cmdid.h>
#include <scriptinfo.hxx>
#include <rootfrm.hxx>
#include <wrtsh.hxx>
#include <txtfrm.hxx>
#include <view.hxx>
#include <IDocumentLayoutAccess.hxx>
#include <IDocumentSettingAccess.hxx>
#include <frameformats.hxx>
#include <unotextrange.hxx>
#include <fmtanchr.hxx>

using namespace css;
using namespace ::cpo;
using namespace ::cpo::uno;

/// Test to assert layout / rendering result of Writer.
class SwLayoutWriter5 : public SwModelTestBase
{
public:
    SwLayoutWriter5()
        : SwModelTestBase(u"/sw/qa/extras/layout/data/"_ustr)
    {
    }
};

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf138194)
{
    createSwDoc("xaxis-labelbreak.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 8
    // - Actual  : 7
    // i.e. the X axis label flowed out of chart area.
    assertXPath(pXmlDoc, "//textarray", 8);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf146272)
{
    createSwDoc("tdf146272.odt");

    uno::Reference<beans::XPropertySet> xPicture1(getShapeByName(u"graphics1"), uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xDrawing1(getShapeByName(u"Shape2"), uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xFrame1(xDrawing1->getPropertyValue(u"TextBoxContent"_ustr),
                                                uno::UNO_QUERY);

    CPPUNIT_ASSERT(xPicture1);
    CPPUNIT_ASSERT(xDrawing1);
    CPPUNIT_ASSERT(xFrame1);

    const sal_uInt64 nPicture1Zorder
        = xPicture1->getPropertyValue(u"ZOrder"_ustr).get<sal_uInt64>();
    const sal_uInt64 nDrawing1Zorder
        = xDrawing1->getPropertyValue(u"ZOrder"_ustr).get<sal_uInt64>();
    const sal_uInt64 nFrame1Zorder = xFrame1->getPropertyValue(u"ZOrder"_ustr).get<sal_uInt64>();

    CPPUNIT_ASSERT_MESSAGE("Bad ZOrder!", nDrawing1Zorder < nFrame1Zorder);
    CPPUNIT_ASSERT_MESSAGE("Bad ZOrder!", nFrame1Zorder < nPicture1Zorder);

    uno::Reference<beans::XPropertySet> xPicture2(getShapeByName(u"Image423"), uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xDrawing2(getShapeByName(u"Shape3"), uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xFrame2(xDrawing2->getPropertyValue(u"TextBoxContent"_ustr),
                                                uno::UNO_QUERY);

    CPPUNIT_ASSERT(xPicture2);
    CPPUNIT_ASSERT(xDrawing2);
    CPPUNIT_ASSERT(xFrame2);

    const sal_uInt64 nPicture2Zorder
        = xPicture2->getPropertyValue(u"ZOrder"_ustr).get<sal_uInt64>();
    const sal_uInt64 nDrawing2Zorder
        = xDrawing2->getPropertyValue(u"ZOrder"_ustr).get<sal_uInt64>();
    const sal_uInt64 nFrame2Zorder = xFrame2->getPropertyValue(u"ZOrder"_ustr).get<sal_uInt64>();

    CPPUNIT_ASSERT_MESSAGE("Bad ZOrder!", nDrawing2Zorder < nFrame2Zorder);
    CPPUNIT_ASSERT_MESSAGE("Bad ZOrder!", nFrame2Zorder < nPicture2Zorder);
    CPPUNIT_ASSERT_MESSAGE("Bad ZOrder!", nDrawing1Zorder < nDrawing2Zorder);
    CPPUNIT_ASSERT_MESSAGE("Bad ZOrder!", nDrawing2Zorder < nPicture1Zorder);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf138773)
{
    createSwDoc("tdf138773.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    const sal_Int32 nFirstLabelLines
        = getXPathContent(pXmlDoc, "count(//text[contains(text(),\"2000-01\")])").toInt32();

    // This failed, if the first X axis label broke to multiple lines.
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), nFirstLabelLines);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf124796)
{
    createSwDoc("tdf124796.odt");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    // This failed, if the minimum value of Y axis is not -10.
    assertXPathContent(pXmlDoc, "(//textarray)[5]/text", u"-10");

    // This failed, if the maximum value of Y axis is not 15.
    assertXPathContent(pXmlDoc, "(//textarray)[10]/text", u"15");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf72727)
{
    createSwDoc("tdf72727.odt");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    // Without the fix in place, this test would have failed with
    // - Expected: 1
    // - Actual  : Series1
    assertXPathContent(pXmlDoc, "(//textarray)[1]/text", u"1");
    assertXPathContent(pXmlDoc, "(//textarray)[2]/text", u"2");
    assertXPathContent(pXmlDoc, "(//textarray)[3]/text", u"3");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf92714_TOC_pageBreak)
{
    // Given a document with a ToC that also defines the page style
    createSwDoc("tdf92714_TOC_pageBreak.odt");

    // When deleting the ToC
    SwView* pView = getSwDocShell()->GetView();
    SfxDispatcher& rDispatcher = *pView->GetViewFrame().GetDispatcher();
    rDispatcher.Execute(FN_REMOVE_CUR_TOX);

    saveAndReload(TestFilter::ODT);

    uno::Reference<frame::XModel> xModel(mxComponent, uno::UNO_QUERY);
    uno::Reference<text::XTextViewCursorSupplier> xTextViewCursorSupplier(
        xModel->getCurrentController(), uno::UNO_QUERY);
    uno::Reference<text::XPageCursor> xCursor(xTextViewCursorSupplier->getViewCursor(),
                                              uno::UNO_QUERY);
    // The page style should not be lost
    CPPUNIT_ASSERT_EQUAL(u"OrangePage"_ustr, getProperty<OUString>(xCursor, u"PageStyleName"_ustr));
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf130969)
{
    createSwDoc("tdf130969.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    // This failed, if the minimum value of Y axis is not 0.35781
    assertXPathContent(pXmlDoc, "(//textarray)[5]/text", u"0.35781");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf40260)
{
    createSwDoc("tdf40260.odt");
    SwDocShell* pShell = getSwDocShell();

    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    // Without the fix in place, this test would have failed with
    // - Expected: f(x) = 1.26510397865547E-06 x − 5.95245604996327E-12
    // - Actual  : f(x) = 0 x − 0
    assertXPathContent(pXmlDoc, "(//textarray)[19]/text",
                       Concat2View("f(x) = 1.26510397865547E-06 x " + OUStringChar(u'\x2212')
                                   + " 5.95245604996327E-12"));
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf129054)
{
    createSwDoc("tdf129054.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    // Test the size of diameter of Pie chart.
    sal_Int32 nYTop
        = getXPath(pXmlDoc,
                   "/metafile/push[1]/push[1]/push[1]/push[6]/push[1]/push[4]/polyline[1]/point[1]",
                   "y")
              .toInt32();
    sal_Int32 nYBottom
        = getXPath(
              pXmlDoc,
              "/metafile/push[1]/push[1]/push[1]/push[6]/push[1]/push[4]/polyline[1]/point[31]",
              "y")
              .toInt32();
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4615, nYTop - nYBottom, 5);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf129173)
{
    createSwDoc("testAreaChartNumberFormat.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    // Check the first data label of area chart.
    assertXPathContent(pXmlDoc, "(//textarray)[22]/text", u"56");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf134866)
{
    createSwDoc("tdf134866.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    // Check the data label of pie chart.
    assertXPathContent(pXmlDoc, "(//textarray)[2]/text", u"100%");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf137116)
{
    createSwDoc("tdf137116.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);
    sal_Int32 nX2 = getXPath(pXmlDoc, "(//textarray)[2]", "x").toInt32(); // second data label
    sal_Int32 nX4 = getXPath(pXmlDoc, "(//textarray)[4]", "x").toInt32(); // fourth data label
    if (comphelper::COKit::isActive())
    {
        // The Kit scales the font rather than the map mode, so the label positions are in the
        // page's own units instead of the chart's, and their distance shrinks with them
        CPPUNIT_ASSERT_DOUBLES_EQUAL(589, nX2 - nX4, 100);
        return;
    }
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 1229
    // - Actual  : -225
    // - Delta   : 100
    // i.e. the second data label appeared inside the pie slice.
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1117, nX2 - nX4, 100);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf137154)
{
    createSwDoc("tdf137154.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);
    sal_Int32 nX1 = getXPath(pXmlDoc, "(//textarray)[1]", "x").toInt32(); // first data label
    sal_Int32 nX4 = getXPath(pXmlDoc, "(//textarray)[4]", "x").toInt32(); // fourth data label
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 10865
    // - Actual  : 10252
    // - Delta   : 50
    // i.e. the first data label appeared too close to the pie.
    CPPUNIT_ASSERT_DOUBLES_EQUAL(nX4, nX1, 50);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf138777)
{
    createSwDoc("outside_long_data_label.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    const sal_Int32 nFirstLabelLines
        = getXPathContent(pXmlDoc, "count(//text[contains(text(),\"really\")])").toInt32();

    // This failed, if the first data label didn't break to multiple lines.
    CPPUNIT_ASSERT_GREATER(static_cast<sal_Int32>(1), nFirstLabelLines);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf130031)
{
    createSwDoc("tdf130031.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);
    sal_Int32 nY = getXPath(pXmlDoc, "(//textarray)[11]", "y").toInt32();
    if (comphelper::COKit::isActive())
    {
        // The Kit scales the font rather than the map mode, so the label position is in the page's
        // own units instead of the chart's
        CPPUNIT_ASSERT_DOUBLES_EQUAL(4346, nY, 50);
        return;
    }
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 4653
    // - Actual  : 2182
    // - Delta   : 50
    // i.e. the data label appeared above the data point.
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4653, nY, 50);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf130242)
{
    createSwDoc("tdf130242.odt");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);
    sal_Int32 nY = getXPath(pXmlDoc, "(//textarray)[11]", "y").toInt32();
    if (comphelper::COKit::isActive())
    {
        // The Kit scales the font rather than the map mode, so the label positions are in the
        // page's own units instead of the chart's
        CPPUNIT_ASSERT_DOUBLES_EQUAL(4958, nY, 50);
        nY = getXPath(pXmlDoc, "(//textarray)[13]", "y").toInt32();
        CPPUNIT_ASSERT_DOUBLES_EQUAL(3018, nY, 50);
        return;
    }
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 5758
    // - Actual  : 3352
    // - Delta   : 50
    // i.e. the data label appeared above the data point.
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5758, nY, 50);

    nY = getXPath(pXmlDoc, "(//textarray)[13]", "y").toInt32();
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 2335
    // - Actual  : 2343
    // - Delta   : 50
    // i.e. the data label appeared above the data point.
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2335, nY, 50);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf134121)
{
    createSwDoc("piechart_leaderline.odt");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    // Check the custom leader line on pie chart.
    assertXPath(pXmlDoc, "//polyline", 1);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf138018)
{
    createSwDoc("tdf138018.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 2
    // - Actual  : 3
    // i.e. the leader line was visible.
    assertXPath(pXmlDoc, "//polyline", 2);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf130380)
{
    createSwDoc("tdf130380.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);
    sal_Int32 nY = getXPath(pXmlDoc,
                            "/metafile/push[1]/push[1]/push[1]/push[6]/push[1]/push[1]/polypolygon/"
                            "polygon/point[1]",
                            "y")
                       .toInt32();
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 6727
    // - Actual  : 4411
    // - Delta   : 50
    // i.e. the area chart shrank.
    CPPUNIT_ASSERT_DOUBLES_EQUAL(6727, nY, 50);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf129095)
{
    createSwDoc("tdf129095.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    // check the inner chart area (relative size) visibility with testing the X axis label
    assertXPathContent(pXmlDoc, "//textarray/text", u"Category 1");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf132956)
{
    createSwDoc("tdf132956.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    // check the inner chart area (default size) visibility with testing the X axis label
    assertXPathContent(pXmlDoc, "//textarray/text", u"Category 1");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf137819)
{
    // Open the bugdoc and check if it went wrong
    createSwDoc("tdf137819.fodt");

    // Change the anchor if the textbox to As_char
    uno::Reference<beans::XPropertySet> xShapePropSet(getShape(1), uno::UNO_QUERY);
    xShapePropSet->setPropertyValue(
        u"AnchorType"_ustr,
        cpo::uno::Any(text::TextContentAnchorType::TextContentAnchorType_AS_CHARACTER));

    // Make the layout xml dump after the change
    auto pXml = parseLayoutDump();
    auto sTextRightSidePosition
        = getXPath(pXml, "/root/page/body/txt[6]/anchored/fly/infos/bounds", "right");
    auto sShapeRightSidePosition
        = getXPath(pXml, "/root/page/body/txt[6]/anchored/SwAnchoredDrawObject/bounds", "right");
    // Before the textframe did not follow the shape, now it supposed to
    // so the right side of the shape must be greater than the right side of
    // textframe:
    CPPUNIT_ASSERT(sTextRightSidePosition.toInt32() < sShapeRightSidePosition.toInt32());
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testParagraphMarkInCell)
{
    createSwDoc("572-min.rtf");

    uno::Reference<frame::XModel> xModel(mxComponent, uno::UNO_QUERY);
    uno::Reference<view::XViewSettingsSupplier> xViewSettingsSupplier(
        xModel->getCurrentController(), uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xViewSettings(xViewSettingsSupplier->getViewSettings());
    cpo::uno::Any aOldHidden{ xViewSettings->getPropertyValue(u"ShowHiddenCharacters"_ustr) };
    cpo::uno::Any aOldNon{ xViewSettings->getPropertyValue(u"ShowNonprintingCharacters"_ustr) };
    comphelper::ScopeGuard g([&] {
        xViewSettings->setPropertyValue(u"ShowHiddenCharacters"_ustr, aOldHidden);
        xViewSettings->setPropertyValue(u"ShowNonprintingCharacters"_ustr, aOldNon);
    });

    xViewSettings->setPropertyValue(u"ShowHiddenCharacters"_ustr, cpo::uno::Any(true));
    xViewSettings->setPropertyValue(u"ShowNonprintingCharacters"_ustr, cpo::uno::Any(true));

    {
        xmlDocUniquePtr pXmlDoc = parseLayoutDump();
        assertXPath(pXmlDoc, "/root/page/body/txt[1]/infos/bounds", "height", u"230");
        assertXPath(pXmlDoc, "/root/page/body/tab[1]/row[1]/infos/bounds", "height", u"690");
        assertXPath(pXmlDoc, "/root/page/body/tab[1]/row[2]/infos/bounds", "height", u"230");
        assertXPath(pXmlDoc, "/root/page/body/txt[2]/infos/bounds", "height", u"230");
    }

    xViewSettings->setPropertyValue(u"ShowNonprintingCharacters"_ustr, cpo::uno::Any(false));

    {
        xmlDocUniquePtr pXmlDoc = parseLayoutDump();
        assertXPath(pXmlDoc, "/root/page/body/txt[1]/infos/bounds", "height", u"230");
        // the problem was that the table rows were not hidden
        assertXPath(pXmlDoc, "/root/page/body/tab[1]/row[1]/infos/bounds", "height", u"0");
        assertXPath(pXmlDoc, "/root/page/body/tab[1]/row[2]/infos/bounds", "height", u"0");
        assertXPath(pXmlDoc, "/root/page/body/txt[2]/infos/bounds", "height", u"230");
    }

    xViewSettings->setPropertyValue(u"ShowNonprintingCharacters"_ustr, cpo::uno::Any(true));

    {
        xmlDocUniquePtr pXmlDoc = parseLayoutDump();
        assertXPath(pXmlDoc, "/root/page/body/txt[1]/infos/bounds", "height", u"230");
        assertXPath(pXmlDoc, "/root/page/body/tab[1]/row[1]/infos/bounds", "height", u"690");
        assertXPath(pXmlDoc, "/root/page/body/tab[1]/row[2]/infos/bounds", "height", u"230");
        assertXPath(pXmlDoc, "/root/page/body/txt[2]/infos/bounds", "height", u"230");
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testParagraphMarkLineHeight)
{
    createSwDoc("A020-min.rtf");

    uno::Reference<frame::XModel> xModel(mxComponent, uno::UNO_QUERY);
    uno::Reference<view::XViewSettingsSupplier> xViewSettingsSupplier(
        xModel->getCurrentController(), uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xViewSettings(xViewSettingsSupplier->getViewSettings());
    cpo::uno::Any aOldHidden{ xViewSettings->getPropertyValue(u"ShowHiddenCharacters"_ustr) };
    cpo::uno::Any aOldNon{ xViewSettings->getPropertyValue(u"ShowNonprintingCharacters"_ustr) };
    comphelper::ScopeGuard g([&] {
        xViewSettings->setPropertyValue(u"ShowHiddenCharacters"_ustr, aOldHidden);
        xViewSettings->setPropertyValue(u"ShowNonprintingCharacters"_ustr, aOldNon);
    });

    xViewSettings->setPropertyValue(u"ShowHiddenCharacters"_ustr, cpo::uno::Any(true));
    xViewSettings->setPropertyValue(u"ShowNonprintingCharacters"_ustr, cpo::uno::Any(true));

    {
        xmlDocUniquePtr pXmlDoc = parseLayoutDump();
        assertXPath(pXmlDoc, "/root/page/header/txt[1]/SwParaPortion/SwLineLayout[1]", "height",
                    u"184");
        assertXPath(pXmlDoc, "/root/page/header/txt[1]/SwParaPortion/SwLineLayout[2]", "height",
                    u"184");
        assertXPath(pXmlDoc, "/root/page/header/txt[1]/SwParaPortion/SwLineLayout[3]", "height",
                    u"184");
        assertXPath(pXmlDoc, "/root/page/header/txt[2]/SwParaPortion/SwLineLayout[1]", "height",
                    u"184");
        assertXPath(pXmlDoc, "/root/page/header/txt[2]/SwParaPortion/SwLineLayout[2]", "height",
                    u"184");
        assertXPath(pXmlDoc, "/root/page/header/txt[2]/SwParaPortion/SwLineLayout[3]", "height",
                    u"184");
        assertXPath(pXmlDoc, "/root/page/header/txt[2]/SwParaPortion/SwLineLayout[4]", "height",
                    u"184");
        assertXPath(pXmlDoc, "/root/page/header/txt[2]/SwParaPortion/SwLineLayout[5]", "height",
                    u"253");
    }

    xViewSettings->setPropertyValue(u"ShowNonprintingCharacters"_ustr, cpo::uno::Any(false));

    {
        xmlDocUniquePtr pXmlDoc = parseLayoutDump();
        assertXPath(pXmlDoc, "/root/page/header/txt[1]/SwParaPortion/SwLineLayout[1]", "height",
                    u"253");
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf122014)
{
    createSwDoc("tdf122014.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    // This failed, if the chart title is aligned to left.
    sal_Int32 nX1 = getXPath(pXmlDoc, "(//textarray)[13]", "x").toInt32();
    sal_Int32 nX2 = getXPath(pXmlDoc, "(//textarray)[14]", "x").toInt32();
    CPPUNIT_ASSERT_GREATER(nX1 + 100, nX2);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf167202_footnote)
{
    createSwDoc("tdf167202_footnote.docx");
    SwDocShell* pShell = getSwDocShell();
    CPPUNIT_ASSERT(pShell);

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    // Check if footnote is there.
    assertXPathContent(pXmlDoc, "(//textarray)[7]/text", u"FOOTNOTE #1");

    // Without the accompanying fix in place, this test would have failed with:
    // equality assertion failed
    // - Expected: 10
    // - Actual  : 11
    // - In <>, XPath '//textarray' number of nodes is incorrect
    // i.e. if there are 11 textarray node, it means there is an extra numbering in the footnote.
    assertXPath(pXmlDoc, "//textarray", 10);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf134659)
{
    createSwDoc("tdf134659.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    // This failed, if the axis label is aligned to left.
    sal_Int32 nX1 = getXPath(pXmlDoc, "(//textarray)[1]", "x").toInt32();
    sal_Int32 nX2 = getXPath(pXmlDoc, "(//textarray)[2]", "x").toInt32();
    CPPUNIT_ASSERT_GREATER(nX1 + 250, nX2);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf134235)
{
    createSwDoc("tdf134235.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 14
    // - Actual  : 13
    // i.e. the chart title flowed out of chart area.
    assertXPath(pXmlDoc, "//textarray", 14);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf134676)
{
    createSwDoc("tdf134676.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 14
    // - Actual  : 13
    // i.e. the X axis title didn't break to multiple lines.
    assertXPath(pXmlDoc, "//textarray", 14);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf134146)
{
    createSwDoc("tdf134146.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    const sal_Int32 nTitleLines
        = getXPathContent(pXmlDoc, "count(//text[contains(text(),\"Horizontal\")])").toInt32();

    // Without the accompanying fix in place, this test would have failed.
    // i.e. the Y axis title didn't break to multiple lines.
    CPPUNIT_ASSERT_GREATER(static_cast<sal_Int32>(1), nTitleLines);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf136061)
{
    createSwDoc("tdf136061.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);
    // This failed, if the custom text of data label is missing.
    assertXPathContent(pXmlDoc, "(//textarray)[16]/text", u"Customlabel");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf116925)
{
    createSwDoc("tdf116925.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    assertXPathContent(pXmlDoc,
                       "/metafile/push[1]/push[1]/push[1]/push[6]/push[1]/push[3]/textarray/text",
                       u"hello");
    // This failed, text color was #000000.
    assertXPath(
        pXmlDoc,
        "/metafile/push[1]/push[1]/push[1]/push[6]/push[1]/push[3]/textcolor[@color='#ffffff']", 1);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf117028)
{
    createSwDoc("tdf117028.docx");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);

    // The only polypolygon in the rendering result was the white background we
    // want to avoid.
    assertXPath(pXmlDoc, "//polypolygon", 0);

    // Make sure the text is still rendered.
    assertXPathContent(pXmlDoc, "//textarray/text", u"Hello");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf106390)
{
    createSwDoc("tdf106390.odt");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);
    sal_Int32 nBottom = getXPath(pXmlDoc, "//sectrectclipregion", "bottom").toInt32();

    // No end point of line segments shall go below the bottom of the clipping area.
    const OString sXPath = "//polyline/point[@y>" + OString::number(nBottom) + "]";

    assertXPath(pXmlDoc, sXPath, 0);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTableExtrusion1)
{
    createSwDoc("table-extrusion1.odt");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);
    sal_Int32 nRight = getXPath(pXmlDoc, "//sectrectclipregion", "right").toInt32();
    sal_Int32 nLeft = static_cast<sal_Int32>(nRight * 0.95);

    // Expect table borders in right page margin.
    const OString sXPath = "//polyline/point[@x>" + OString::number(nLeft) + " and @x<"
                           + OString::number(nRight) + "]";

    assertXPath(pXmlDoc, sXPath, 4);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTableExtrusion2)
{
    createSwDoc("table-extrusion2.odt");
    SwDocShell* pShell = getSwDocShell();

    // Dump the rendering of the first page as an XML file.
    std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
    MetafileXmlDump dumper;
    xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
    CPPUNIT_ASSERT(pXmlDoc);
    // End point position of the outer table.
    sal_Int32 nX = getXPath(pXmlDoc, "(//polyline[1]/point)[2]", "x").toInt32();

    // Do not allow inner table extrude outer table.
    const OString sXPath = "//polyline/point[@x>" + OString::number(nX) + "]";

    assertXPath(pXmlDoc, sXPath, 0);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf116848)
{
    createSwDoc("tdf116848.odt");
    // This resulted in a layout loop.
    calcLayout();
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf117245)
{
    createSwDoc("tdf117245.odt");
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    // This was 2, TabOverMargin did not use a single line when there was
    // enough space for the text.
    assertXPath(pXmlDoc, "/root/page/body/txt[1]/SwParaPortion/SwLineLayout", 1);

    // This was 2, same problem elsewhere due to code duplication.
    assertXPath(pXmlDoc, "/root/page/body/txt[2]/SwParaPortion/SwLineLayout", 1);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf159029)
{
    createSwDoc("2024-01-19_merged-cells-on-separate-pages-vertical-alignment.odt");

    {
        xmlDocUniquePtr pXmlDoc = parseLayoutDump();
        assertXPath(pXmlDoc, "/root/page[1]/body/tab[1]/row[1]/cell[2]/infos/bounds", "top",
                    u"1970");
        assertXPath(pXmlDoc, "/root/page[1]/body/tab[1]/row[1]/cell[2]/txt[1]/infos/bounds", "top",
                    u"1970");
    }

    // set vert orient
    SwWrtShell* pWrtShell = getSwDocShell()->GetWrtShell();
    pWrtShell->GotoTable(UIName(u"Table1"_ustr));
    pWrtShell->Right(SwCursorSkipMode::Cells, /*bSelect=*/false, 1, /*bBasicCall=*/false);
    pWrtShell->SetBoxAlign(css::text::VertOrientation::BOTTOM);

    {
        xmlDocUniquePtr pXmlDoc = parseLayoutDump();
        assertXPath(pXmlDoc, "/root/page[1]/body/tab[1]/row[1]/cell[2]/infos/bounds", "top",
                    u"1970");
        assertXPath(pXmlDoc, "/root/page[1]/body/tab[1]/row[1]/cell[2]/txt[1]/infos/bounds", "top",
                    u"15494");
    }

    // delete
    pWrtShell->SttEndDoc(true);
    pWrtShell->DelRight();

    {
        xmlDocUniquePtr pXmlDoc = parseLayoutDump();
        assertXPath(pXmlDoc, "/root/page[1]/body/tab[1]/row[1]/cell[2]/infos/bounds", "top",
                    u"1694");
        // the problem was that this moved to the top of the cell
        assertXPath(pXmlDoc, "/root/page[1]/body/tab[1]/row[1]/cell[2]/txt[1]/infos/bounds", "top",
                    u"15494");
    }

    pWrtShell->Undo();

    {
        xmlDocUniquePtr pXmlDoc = parseLayoutDump();
        assertXPath(pXmlDoc, "/root/page[1]/body/tab[1]/row[1]/cell[2]/infos/bounds", "top",
                    u"1970");
        // the problem was that this moved to the top of the cell
        assertXPath(pXmlDoc, "/root/page[1]/body/tab[1]/row[1]/cell[2]/txt[1]/infos/bounds", "top",
                    u"15494");
    }

    pWrtShell->Redo();

    {
        xmlDocUniquePtr pXmlDoc = parseLayoutDump();
        assertXPath(pXmlDoc, "/root/page[1]/body/tab[1]/row[1]/cell[2]/infos/bounds", "top",
                    u"1694");
        assertXPath(pXmlDoc, "/root/page[1]/body/tab[1]/row[1]/cell[2]/txt[1]/infos/bounds", "top",
                    u"15494");
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf118672)
{
    createSwDoc("tdf118672.odt");
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();

    // Check if we have hyphenation support, otherwise can't test SwHyphPortion.
    uno::Reference<linguistic2::XHyphenator> xHyphenator = LinguMgr::GetHyphenator();
    if (!xHyphenator->hasLocale(lang::Locale(u"en"_ustr, u"US"_ustr, OUString())))
        return;

    // This ended as "fol*1 2 3 4 5 6 7 8 9", i.e. "10con-" was moved to the next line.
    // Bizarrely, Word lets the text run into the right margin (loaded from .rtf)
    assertXPath(pXmlDoc, "/root/page/body/txt[1]/SwParaPortion/SwLineLayout[1]", "portion",
                u"He heard quiet steps behind him. That didn't bode well. Who could be fol*1 2 "
                "3 4 5 6 7 8 9 10consetetur");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf150200)
{
    createSwDoc("tdf150200.odt");
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    // dash
    OUString sFirstLine
        = getXPathContent(pXmlDoc, "/root/page/body/txt[1]/SwParaPortion/SwLineLayout[1]/@portion");
    CPPUNIT_ASSERT_EQUAL(true, sFirstLine.startsWith(u"-(dash)"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(93), sFirstLine.getLength());
    // en-dash
    sFirstLine
        = getXPathContent(pXmlDoc, "/root/page/body/txt[2]/SwParaPortion/SwLineLayout[1]/@portion");
    CPPUNIT_ASSERT_EQUAL(true, sFirstLine.startsWith(u"–(en-dash)"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(88), sFirstLine.getLength());
    // em-dash
    sFirstLine
        = getXPathContent(pXmlDoc, "/root/page/body/txt[3]/SwParaPortion/SwLineLayout[1]/@portion");
    CPPUNIT_ASSERT_EQUAL(true, sFirstLine.startsWith(u"—(em-dash)"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(77), sFirstLine.getLength());
    // figure dash
    sFirstLine
        = getXPathContent(pXmlDoc, "/root/page/body/txt[4]/SwParaPortion/SwLineLayout[1]/@portion");
    CPPUNIT_ASSERT_EQUAL(true, sFirstLine.startsWith(u"‒(figure dash)"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(87), sFirstLine.getLength());
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf150200_DOCX)
{
    createSwDoc("tdf150200.docx");
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    // dash
    OUString sFirstLine
        = getXPathContent(pXmlDoc, "/root/page/body/txt[1]/SwParaPortion/SwLineLayout[1]/@portion");
    CPPUNIT_ASSERT_EQUAL(true, sFirstLine.startsWith(u"-(dash)"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(93), sFirstLine.getLength());
    // en-dash
    sFirstLine
        = getXPathContent(pXmlDoc, "/root/page/body/txt[2]/SwParaPortion/SwLineLayout[1]/@portion");
    CPPUNIT_ASSERT_EQUAL(true, sFirstLine.startsWith(u"–(en-dash)"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(88), sFirstLine.getLength());
    // em-dash
    sFirstLine
        = getXPathContent(pXmlDoc, "/root/page/body/txt[3]/SwParaPortion/SwLineLayout[1]/@portion");
    CPPUNIT_ASSERT_EQUAL(true, sFirstLine.startsWith(u"—(em-dash)"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(77), sFirstLine.getLength());
    // figure dash
    sFirstLine
        = getXPathContent(pXmlDoc, "/root/page/body/txt[4]/SwParaPortion/SwLineLayout[1]/@portion");
    CPPUNIT_ASSERT_EQUAL(true, sFirstLine.startsWith(u"‒(figure dash)"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(87), sFirstLine.getLength());
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf150438)
{
    createSwDoc("tdf150438.odt");
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    // left double quotation mark
    OUString sFirstLine
        = getXPathContent(pXmlDoc, "/root/page/body/txt[1]/SwParaPortion/SwLineLayout[1]/@portion");
    CPPUNIT_ASSERT_EQUAL(true, sFirstLine.startsWith(u"“Lorem ipsum"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(92), sFirstLine.getLength());
    // right double quotation mark
    sFirstLine
        = getXPathContent(pXmlDoc, "/root/page/body/txt[2]/SwParaPortion/SwLineLayout[1]/@portion");
    CPPUNIT_ASSERT_EQUAL(true, sFirstLine.startsWith(u"”Nunc viverra imperdiet enim."));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(97), sFirstLine.getLength());
    // left single quotation mark
    sFirstLine
        = getXPathContent(pXmlDoc, "/root/page/body/txt[3]/SwParaPortion/SwLineLayout[1]/@portion");
    CPPUNIT_ASSERT_EQUAL(true, sFirstLine.startsWith(u"‘Aenean nec lorem."));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(85), sFirstLine.getLength());
    // right single quotation mark or apostrophe
    sFirstLine
        = getXPathContent(pXmlDoc, "/root/page/body/txt[4]/SwParaPortion/SwLineLayout[1]/@portion");
    CPPUNIT_ASSERT_EQUAL(true, sFirstLine.startsWith(u"’Aenean nec lorem."));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(85), sFirstLine.getLength());
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf150438_DOCX)
{
    createSwDoc("tdf150438.docx");
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    // left double quotation mark
    OUString sFirstLine
        = getXPathContent(pXmlDoc, "/root/page/body/txt[1]/SwParaPortion/SwLineLayout[1]/@portion");
    CPPUNIT_ASSERT_EQUAL(true, sFirstLine.startsWith(u"“Lorem ipsum"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(92), sFirstLine.getLength());
    // right double quotation mark
    sFirstLine
        = getXPathContent(pXmlDoc, "/root/page/body/txt[2]/SwParaPortion/SwLineLayout[1]/@portion");
    CPPUNIT_ASSERT_EQUAL(true, sFirstLine.startsWith(u"”Nunc viverra imperdiet enim."));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(97), sFirstLine.getLength());
    // left single quotation mark
    sFirstLine
        = getXPathContent(pXmlDoc, "/root/page/body/txt[3]/SwParaPortion/SwLineLayout[1]/@portion");
    CPPUNIT_ASSERT_EQUAL(true, sFirstLine.startsWith(u"‘Aenean nec lorem."));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(85), sFirstLine.getLength());
    // right single quotation mark or apostrophe
    sFirstLine
        = getXPathContent(pXmlDoc, "/root/page/body/txt[4]/SwParaPortion/SwLineLayout[1]/@portion");
    CPPUNIT_ASSERT_EQUAL(true, sFirstLine.startsWith(u"’Aenean nec lorem."));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(85), sFirstLine.getLength());
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf117923)
{
    createSwDoc("tdf117923.doc");
    // Ensure that all text portions are calculated before testing.
    SwViewShell* pViewShell = getSwDoc()->getIDocumentLayoutAccess().GetCurrentViewShell();
    CPPUNIT_ASSERT(pViewShell);
    pViewShell->Reformat();

    xmlDocUniquePtr pXmlDoc = parseLayoutDump();

    // Check that we actually test the line we need
    assertXPathContent(pXmlDoc, "/root/page/body/tab/row/cell/txt[3]", u"GHI GHI GHI GHI");
    assertXPath(pXmlDoc,
                "/root/page/body/tab/row/cell/txt[3]/SwParaPortion/SwLineLayout/SwFieldPortion",
                "type", u"PortionType::Number");
    assertXPath(pXmlDoc,
                "/root/page/body/tab/row/cell/txt[3]/SwParaPortion/SwLineLayout/SwFieldPortion",
                "expand", u"2.");
    // The numbering height was 960.
    assertXPath(
        pXmlDoc,
        "/root/page/body/tab/row/cell/txt[3]/SwParaPortion/SwLineLayout/SwFieldPortion/SwFont",
        "height", u"220");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf127606)
{
    createSwDoc("tdf117923.docx");
    // Ensure that all text portions are calculated before testing.
    SwViewShell* pViewShell = getSwDoc()->getIDocumentLayoutAccess().GetCurrentViewShell();
    CPPUNIT_ASSERT(pViewShell);
    pViewShell->Reformat();

    xmlDocUniquePtr pXmlDoc = parseLayoutDump();

    // Check that we actually test the line we need
    assertXPathContent(pXmlDoc, "/root/page/body/tab/row/cell/txt[3]", u"GHI GHI GHI GHI");
    assertXPath(pXmlDoc,
                "/root/page/body/tab/row/cell/txt[3]/SwParaPortion/SwLineLayout/SwFieldPortion",
                "type", u"PortionType::Number");
    assertXPath(pXmlDoc,
                "/root/page/body/tab/row/cell/txt[3]/SwParaPortion/SwLineLayout/SwFieldPortion",
                "expand", u"2.");
    // The numbering height was 960 in DOC format.
    assertXPath(
        pXmlDoc,
        "/root/page/body/tab/row/cell/txt[3]/SwParaPortion/SwLineLayout/SwFieldPortion/SwFont",
        "height", u"220");

    // tdf#127606: now it's possible to change formatting of numbering
    // increase font size (220 -> 260)
    dispatchCommand(mxComponent, u".uno:SelectAll"_ustr, {});
    dispatchCommand(mxComponent, u".uno:Grow"_ustr, {});
    pViewShell->Reformat();
    pXmlDoc = parseLayoutDump();
    assertXPath(
        pXmlDoc,
        "/root/page/body/tab/row/cell/txt[3]/SwParaPortion/SwLineLayout/SwFieldPortion/SwFont",
        "height", u"260");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf127118)
{
    createSwDoc("tdf127118.docx");
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    // This was Horizontal: merged cell split between pages didn't keep vertical writing direction
    assertXPath(pXmlDoc, "/root/page[2]/body/tab/row[1]/cell[1]/txt[1]", "WritingMode",
                u"VertBTLR");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf141220)
{
    createSwDoc("tdf141220.docx");

    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    sal_Int32 nShapeTop
        = getXPath(pXmlDoc, "//anchored/SwAnchoredDrawObject/bounds", "top").toInt32();
    sal_Int32 nTextBoxTop = getXPath(pXmlDoc, "//anchored/fly/infos/bounds", "top").toInt32();
    // Make sure the textbox stays inside the shape.
    CPPUNIT_ASSERT_LESS(static_cast<sal_Int32>(15), nTextBoxTop - nShapeTop);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, TestTextBoxChangeViaUNO)
{
    createSwDoc("TextBoxFrame.odt");
    // this file has a shape and a frame inside. Try to set up
    // the frame for the shape as textbox. Before this was not
    // implemented. This will be necessary for proper WPG import.

    CPPUNIT_ASSERT_EQUAL_MESSAGE("There must be a shape and a frame!", 2, getShapes());

    CPPUNIT_ASSERT_EQUAL_MESSAGE("This must be a custom shape!",
                                 u"com.sun.star.drawing.CustomShape"_ustr,
                                 getShape(1)->getShapeType());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("This must be a frame shape!", u"FrameShape"_ustr,
                                 getShape(2)->getShapeType());

    CPPUNIT_ASSERT_MESSAGE("This is not supposed to be a textbox!",
                           !uno::Reference<beans::XPropertySet>(getShape(1), uno::UNO_QUERY_THROW)
                                ->getPropertyValue(u"TextBox"_ustr)
                                .get<bool>());
    // Without the fix it will crash at this line:
    CPPUNIT_ASSERT_MESSAGE("This is not supposed to be a textbox!",
                           !uno::Reference<beans::XPropertySet>(getShape(1), uno::UNO_QUERY_THROW)
                                ->getPropertyValue(u"TextBoxContent"_ustr)
                                .hasValue());

    // So now set the frame as textbox for the shape!
    uno::Reference<beans::XPropertySet>(getShape(1), uno::UNO_QUERY_THROW)
        ->setPropertyValue(u"TextBoxContent"_ustr, cpo::uno::Any(uno::Reference<text::XTextFrame>(
                                                       getShape(2), uno::UNO_QUERY_THROW)));

    CPPUNIT_ASSERT_MESSAGE("This is supposed to be a textbox!",
                           uno::Reference<beans::XPropertySet>(getShape(1), uno::UNO_QUERY_THROW)
                               ->getPropertyValue(u"TextBox"_ustr)
                               .get<bool>());

    CPPUNIT_ASSERT_MESSAGE("This is supposed to be a textbox!",
                           uno::Reference<beans::XPropertySet>(getShape(1), uno::UNO_QUERY_THROW)
                               ->getPropertyValue(u"TextBoxContent"_ustr)
                               .hasValue());
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf121509)
{
    createSwDoc("Tdf121509.odt");
    auto pDoc = getSwDoc();

    // Get all shape/frame formats
    auto vFrameFormats = pDoc->GetSpzFrameFormats();
    // Get the textbox
    auto xTextFrame = SwTextBoxHelper::getUnoTextFrame(getShape(1));
    // Get The triangle
    auto pTriangleShapeFormat = vFrameFormats->GetFormat(2);
    CPPUNIT_ASSERT(xTextFrame);
    CPPUNIT_ASSERT(pTriangleShapeFormat);

    // Get the position inside the textbox
    auto xTextContentStart = xTextFrame->getText()->getStart();
    SwUnoInternalPaM aCursor(*pDoc);
    CPPUNIT_ASSERT(sw::XTextRangeToSwPaM(aCursor, xTextContentStart));

    // Put the triangle into the textbox
    SwFormatAnchor aNewAnch(pTriangleShapeFormat->GetAnchor());
    aNewAnch.SetAnchor(aCursor.Start());
    CPPUNIT_ASSERT(pTriangleShapeFormat->SetFormatAttr(aNewAnch));

    save(TestFilter::DOCX);

    // The second part: check if the reloaded doc has flys inside a fly
    // FIXME: if we use 'saveAndReload' or 'loadFromURL' here, it fails with
    //  Assertion `!m_pFirst && !m_pLast && "There are still indices registered"' failed.
    loadFromDesktop(maTempFile.GetURL(), u"com.sun.star.text.TextDocument"_ustr);
    auto pSecondDoc = getSwDoc();
    auto pSecondFormats = pSecondDoc->GetSpzFrameFormats();

    bool bFlyInFlyFound = false;
    for (auto secondformat : *pSecondFormats)
    {
        const SwNode* pNd = secondformat->GetAnchor().GetAnchorNode();
        if (pNd->FindFlyStartNode())
        {
            // So there is a fly inside another -> problem.
            bFlyInFlyFound = true;
            break;
        }
    }
    // Drop the tempfile
    maTempFile.CloseStream();

    // With the fix this cannot be true, if it is, that means Word unable to read the file..
    CPPUNIT_ASSERT_MESSAGE("Corrupt exported docx file!", !bFlyInFlyFound);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf134685)
{
    createSwDoc("tdf134685.docx");
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    sal_Int32 nWidth
        = getXPath(pXmlDoc, "/root/page/body/tab/row[6]/cell[1]/infos/bounds", "width").toInt32();
    // This was 2223: the content was only partially visible according to the lost cell width
    CPPUNIT_ASSERT_GREATER(static_cast<sal_Int32>(14000), nWidth);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf109077)
{
    createSwDoc("tdf109077.docx");
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    sal_Int32 nShapeTop
        = getXPath(pXmlDoc, "//anchored/SwAnchoredDrawObject/bounds", "top").toInt32();
    sal_Int32 nTextBoxTop = getXPath(pXmlDoc, "//anchored/fly/infos/bounds", "top").toInt32();
    // This was 281: the top of the shape and its textbox should match, though
    // tolerate differences <= 1px (about 15 twips).
    CPPUNIT_ASSERT_LESS(static_cast<sal_Int32>(15), nTextBoxTop - nShapeTop);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testUserFieldTypeLanguage)
{
    // Set the system locale to German, the document will be English.
    SvtSysLocaleOptions aOptions;
    OUString sLocaleConfigString = aOptions.GetLanguageTag().getBcp47();
    aOptions.SetLocaleConfigString(u"de-DE"_ustr);
    aOptions.Commit();
    comphelper::ScopeGuard g1([&aOptions, &sLocaleConfigString] {
        aOptions.SetLocaleConfigString(sLocaleConfigString);
        aOptions.Commit();
    });

    createSwDoc("user-field-type-language.fodt");
    SwDoc* pDoc = getSwDoc();
    SwViewShell* pViewShell = pDoc->getIDocumentLayoutAccess().GetCurrentViewShell();
    pViewShell->UpdateFields();
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    // This was "123,456.00", via a buggy 1234.56 -> 1234,56 -> 123456 ->
    // 123,456.00 transform chain.
    assertXPath(
        pXmlDoc,
        "/root/page/body/txt/SwParaPortion/SwLineLayout/SwFieldPortion[@type='PortionType::Field']",
        "expand", u"1,234.56");

    // Now change the system locale to English (before this was failing, 1234,56 -> 0.00)
    aOptions.SetLocaleConfigString(u"en-GB"_ustr);
    aOptions.Commit();
    comphelper::ScopeGuard g2([&aOptions, &sLocaleConfigString] {
        aOptions.SetLocaleConfigString(sLocaleConfigString);
        aOptions.Commit();
    });
    pViewShell->UpdateFields();
    pXmlDoc = parseLayoutDump();
    // We expect, that the field value is not changed. Otherwise there is a problem:
    assertXPath(
        pXmlDoc,
        "/root/page/body/txt/SwParaPortion/SwLineLayout/SwFieldPortion[@type='PortionType::Field']",
        "expand", u"1,234.56");
    // Now change the system locale to German
    aOptions.SetLocaleConfigString(u"de-DE"_ustr);
    aOptions.Commit();
    comphelper::ScopeGuard g3([&aOptions, &sLocaleConfigString] {
        aOptions.SetLocaleConfigString(sLocaleConfigString);
        aOptions.Commit();
    });
    pViewShell->UpdateFields();
    pXmlDoc = parseLayoutDump();
    // We expect, that the field value is not changed. Otherwise there is a problem:
    assertXPath(
        pXmlDoc,
        "/root/page/body/txt/SwParaPortion/SwLineLayout/SwFieldPortion[@type='PortionType::Field']",
        "expand", u"1,234.56");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf124261)
{
#if !defined(_WIN32) && !defined(MACOSX)
    // Make sure that pressing a key in a btlr cell frame causes an immediate, correct repaint.
    createSwDoc("tdf124261.docx");
    SwDoc* pDoc = getSwDoc();
    SwRootFrame* pLayout = pDoc->getIDocumentLayoutAccess().GetCurrentLayout();
    SwFrame* pPageFrame = pLayout->GetLower();
    CPPUNIT_ASSERT(pPageFrame->IsPageFrame());

    SwFrame* pBodyFrame = pPageFrame->GetLower();
    CPPUNIT_ASSERT(pBodyFrame->IsBodyFrame());

    SwFrame* pTabFrame = pBodyFrame->GetLower();
    CPPUNIT_ASSERT(pTabFrame->IsTabFrame());

    SwFrame* pRowFrame = pTabFrame->GetLower();
    CPPUNIT_ASSERT(pRowFrame->IsRowFrame());

    SwFrame* pCellFrame = pRowFrame->GetLower();
    CPPUNIT_ASSERT(pCellFrame->IsCellFrame());

    SwFrame* pFrame = pCellFrame->GetLower();
    CPPUNIT_ASSERT(pFrame->IsTextFrame());

    // Make sure that the text frame's area and the paint rectangle match.
    // Without the accompanying fix in place, this test would have failed with 'Expected: 1721;
    // Actual: 1547', i.e. an area other than the text frame was invalidated for a single-line
    // paragraph.
    SwTextFrame* pTextFrame = static_cast<SwTextFrame*>(pFrame);
    SwRect aRect = pTextFrame->GetPaintSwRect();
    CPPUNIT_ASSERT_EQUAL(pTextFrame->getFrameArea().Top(), aRect.Top());
#endif
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf164932)
{
    // text frame with AutoText setting must use the left offset
    // of the list style of its text content
    createSwDoc("tdf164932.fodt");
    auto pDump = parseLayoutDump();
    // This was 4 (three lines in the text frame anchored as character in the first line
    assertXPath(pDump, "//SwLineLayout", 2);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf164903)
{
    // ignore top margin of inline heading, like MSO does
    createSwDoc("tdf164903.docx");
    auto pDump = parseLayoutDump();
    auto nInlineHeadingHeight
        = getXPath(pDump, "/root/page[1]/body/txt[6]/infos/bounds", "height").toInt32();
    // This was 493 (adding top margin to the height of the Inline Heading frame)
    CPPUNIT_ASSERT_GREATER(sal_Int32(250), nInlineHeadingHeight);
    CPPUNIT_ASSERT_LESS(sal_Int32(260), nInlineHeadingHeight);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf135991)
{
    createSwDoc("tdf135991.odt");
    auto pDump = parseLayoutDump();
    // There used to be negative values that made the column frames invisible.
    assertXPath(pDump, "//bounds[@top<0]", 0);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf150642)
{
    createSwDoc("tdf150642.odt");
    auto pDump = parseLayoutDump();
    // There used to be negative values that made the cell frame invisible.
    assertXPath(pDump, "//bounds[@left<0]", 0);
    assertXPath(pDump, "//bounds[@right<0]", 0);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf152085)
{
    createSwDoc("tdf152085-section-tblr.odt");
    auto pDump = parseLayoutDump();
    sal_Int32 nSectionHeight
        = getXPath(pDump, "//section/infos/bounds", "bottom").toInt32(); // was 8391
    sal_Int32 nColumnHeight
        = getXPath(pDump, "(//column/infos/bounds)[2]", "bottom").toInt32(); // was 16216
    CPPUNIT_ASSERT_MESSAGE("The column in a TBRL page should be shorter than the section.",
                           nColumnHeight <= nSectionHeight);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf152031)
{
    createSwDoc("tdf152031-stair.odt");

    // reproduce the bug by shrinking the table width.
    uno::Reference<text::XTextTablesSupplier> xTablesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xTablesSupplier->getTextTables(),
                                                    uno::UNO_QUERY);
    uno::Reference<text::XTextTable> xTable(xTables->getByIndex(0), uno::UNO_QUERY);
    // Shrink table width from 5" to 4"
    sal_Int32 nWidth = getProperty<sal_Int32>(xTable, u"Width"_ustr) * 4 / 5;

    uno::Reference<beans::XPropertySet> xSet(xTable, uno::UNO_QUERY);
    xSet->setPropertyValue(u"Width"_ustr, cpo::uno::Any(nWidth));

    auto pDump = parseLayoutDump();
    // There was a stair effect after change the table size.
    sal_Int32 nLeft_Row1 = getXPath(pDump, "(//row/infos/bounds)[1]", "left").toInt32();
    sal_Int32 nLeft_Row2 = getXPath(pDump, "(//row/infos/bounds)[2]", "left").toInt32();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("left values of SwRowFrames should be consistent.", nLeft_Row1,
                                 nLeft_Row2);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf153136)
{
    createSwDoc("tdf153136.docx");
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();

    const sal_Int32 small = 300; // Small-height lines are around 276 twip
    const sal_Int32 large = 1000; // Large-height lines are 1104 twip or more

    // Page 1: standalone paragraphs

    // U+0009 CHARACTER TABULATION: height is ignored
    sal_Int32 height = getXPath(pXmlDoc, "(/root/page[1]//SwLineLayout)[1]", "height").toInt32();
    CPPUNIT_ASSERT_LESS(small, height);

    // U+0020 SPACE: height is ignored
    height = getXPath(pXmlDoc, "(/root/page[1]//SwLineLayout)[2]", "height").toInt32();
    CPPUNIT_ASSERT_LESS(small, height);

    // U+00A0 NO-BREAK SPACE: height is considered
    height = getXPath(pXmlDoc, "(/root/page[1]//SwLineLayout)[3]", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+1680 OGHAM SPACE MARK: height is considered; not tested, because Liberation Serif lacks it

    // U+2000 EN QUAD: height is considered
    height = getXPath(pXmlDoc, "(/root/page[1]//SwLineLayout)[4]", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+2001 EM QUAD: height is considered
    height = getXPath(pXmlDoc, "(/root/page[1]//SwLineLayout)[5]", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+2002 EN SPACE: height is ignored
    height = getXPath(pXmlDoc, "(/root/page[1]//SwLineLayout)[6]", "height").toInt32();
    CPPUNIT_ASSERT_LESS(small, height);

    // U+2003 EM SPACE: height is ignored
    height = getXPath(pXmlDoc, "(/root/page[1]//SwLineLayout)[7]", "height").toInt32();
    CPPUNIT_ASSERT_LESS(small, height);

    // U+2004 THREE-PER-EM SPACE: height is considered
    height = getXPath(pXmlDoc, "(/root/page[1]//SwLineLayout)[8]", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+2005 FOUR-PER-EM SPACE: height is ignored
    height = getXPath(pXmlDoc, "(/root/page[1]//SwLineLayout)[9]", "height").toInt32();
    CPPUNIT_ASSERT_LESS(small, height);

    // U+2006 SIX-PER-EM SPACE: height is considered
    height = getXPath(pXmlDoc, "(/root/page[1]//SwLineLayout)[10]", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+2007 FIGURE SPACE: height is considered
    height = getXPath(pXmlDoc, "(/root/page[1]//SwLineLayout)[11]", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+2008 PUNCTUATION SPACE: height is considered
    height = getXPath(pXmlDoc, "(/root/page[1]//SwLineLayout)[12]", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+2009 THIN SPACE: height is considered
    height = getXPath(pXmlDoc, "(/root/page[1]//SwLineLayout)[13]", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+200A HAIR SPACE: height is considered
    height = getXPath(pXmlDoc, "(/root/page[1]//SwLineLayout)[14]", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+202F NARROW NO-BREAK SPACE: height is considered
    height = getXPath(pXmlDoc, "(/root/page[1]//SwLineLayout)[15]", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+205F MEDIUM MATHEMATICAL SPACE: height is considered; not tested, because Liberation Serif lacks it
    // U+3000 IDEOGRAPHIC SPACE: height is ignored; not tested, because Liberation Serif lacks it

    // Page 2: table rows (no paragraph-level size DF)

    // U+0020 SPACE: height is ignored
    height = getXPath(pXmlDoc, "(/root/page[2]//row)[1]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_LESS(small, height);

    // U+00A0 NO-BREAK SPACE: height is considered (1104 or so)
    height = getXPath(pXmlDoc, "(/root/page[2]//row)[2]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+1680 OGHAM SPACE MARK: height is considered; not tested, because Liberation Serif lacks it

    // U+2000 EN QUAD: height is considered
    height = getXPath(pXmlDoc, "(/root/page[2]//row)[3]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+2001 EM QUAD: height is considered
    height = getXPath(pXmlDoc, "(/root/page[2]//row)[4]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+2002 EN SPACE: height is ignored
    height = getXPath(pXmlDoc, "(/root/page[2]//row)[5]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_LESS(small, height);

    // U+2003 EM SPACE: height is ignored
    height = getXPath(pXmlDoc, "(/root/page[2]//row)[6]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_LESS(small, height);

    // U+2004 THREE-PER-EM SPACE: height is considered
    height = getXPath(pXmlDoc, "(/root/page[2]//row)[7]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+2005 FOUR-PER-EM SPACE: height is ignored
    height = getXPath(pXmlDoc, "(/root/page[2]//row)[8]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_LESS(small, height);

    // U+2006 SIX-PER-EM SPACE: height is considered
    height = getXPath(pXmlDoc, "(/root/page[2]//row)[9]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+2007 FIGURE SPACE: height is considered
    height = getXPath(pXmlDoc, "(/root/page[2]//row)[10]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+2008 PUNCTUATION SPACE: height is considered
    height = getXPath(pXmlDoc, "(/root/page[2]//row)[11]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+2009 THIN SPACE: height is considered
    height = getXPath(pXmlDoc, "(/root/page[2]//row)[12]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+200A HAIR SPACE: height is considered
    height = getXPath(pXmlDoc, "(/root/page[2]//row)[13]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+202F NARROW NO-BREAK SPACE: height is considered
    height = getXPath(pXmlDoc, "(/root/page[2]//row)[14]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(large, height);

    // U+205F MEDIUM MATHEMATICAL SPACE: height is considered; not tested, because Liberation Serif lacks it
    // U+3000 IDEOGRAPHIC SPACE: height is ignored; not tested, because Liberation Serif lacks it

    // TODO: page 3, with table having paragraphs with paragraph-level size DF;
    // all rows should have large height

    // height = getXPath(pXmlDoc, "(/root/page[3]//row)[1]/infos/bounds", "height").toInt32();
    // CPPUNIT_ASSERT_GREATER(large, height);
    // height = getXPath(pXmlDoc, "(/root/page[3]//row)[2]/infos/bounds", "height").toInt32();
    // CPPUNIT_ASSERT_GREATER(large, height);
    // height = getXPath(pXmlDoc, "(/root/page[3]//row)[3]/infos/bounds", "height").toInt32();
    // CPPUNIT_ASSERT_GREATER(large, height);
    // height = getXPath(pXmlDoc, "(/root/page[3]//row)[4]/infos/bounds", "height").toInt32();
    // CPPUNIT_ASSERT_GREATER(large, height);
    // height = getXPath(pXmlDoc, "(/root/page[3]//row)[5]/infos/bounds", "height").toInt32();
    // CPPUNIT_ASSERT_GREATER(large, height);
    // height = getXPath(pXmlDoc, "(/root/page[3]//row)[6]/infos/bounds", "height").toInt32();
    // CPPUNIT_ASSERT_GREATER(large, height);
    // height = getXPath(pXmlDoc, "(/root/page[3]//row)[7]/infos/bounds", "height").toInt32();
    // CPPUNIT_ASSERT_GREATER(large, height);
    // height = getXPath(pXmlDoc, "(/root/page[3]//row)[8]/infos/bounds", "height").toInt32();
    // CPPUNIT_ASSERT_GREATER(large, height);
    // height = getXPath(pXmlDoc, "(/root/page[3]//row)[9]/infos/bounds", "height").toInt32();
    // CPPUNIT_ASSERT_GREATER(large, height);
    // height = getXPath(pXmlDoc, "(/root/page[3]//row)[10]/infos/bounds", "height").toInt32();
    // CPPUNIT_ASSERT_GREATER(large, height);
    // height = getXPath(pXmlDoc, "(/root/page[3]//row)[11]/infos/bounds", "height").toInt32();
    // CPPUNIT_ASSERT_GREATER(large, height);
    // height = getXPath(pXmlDoc, "(/root/page[3]//row)[12]/infos/bounds", "height").toInt32();
    // CPPUNIT_ASSERT_GREATER(large, height);
    // height = getXPath(pXmlDoc, "(/root/page[3]//row)[13]/infos/bounds", "height").toInt32();
    // CPPUNIT_ASSERT_GREATER(large, height);
    // height = getXPath(pXmlDoc, "(/root/page[3]//row)[14]/infos/bounds", "height").toInt32();
    // CPPUNIT_ASSERT_GREATER(large, height);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testFoMargin)
{
    createSwDoc("fomargin.fodt");
    auto pXmlDoc = parseLayoutDump();

    auto nPage1Width
        = getXPath(pXmlDoc, "/root/page[1]/body/txt[1]/infos/bounds", "width").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(9600), nPage1Width);
    CPPUNIT_ASSERT_LESS(sal_Int32(9700), nPage1Width);

    auto nPage1Height
        = getXPath(pXmlDoc, "/root/page[1]/body/txt[1]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(13800), nPage1Height);
    CPPUNIT_ASSERT_LESS(sal_Int32(14000), nPage1Height);

    auto nPage2Width
        = getXPath(pXmlDoc, "/root/page[2]/body/txt[1]/infos/bounds", "width").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(9600), nPage2Width);
    CPPUNIT_ASSERT_LESS(sal_Int32(9700), nPage2Width);

    auto nPage2Height
        = getXPath(pXmlDoc, "/root/page[2]/body/txt[1]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(13100), nPage2Height);
    CPPUNIT_ASSERT_LESS(sal_Int32(13400), nPage2Height);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf144450)
{
    createSwDoc("tdf144450.fodt");
    auto pXmlDoc = parseLayoutDump();

    // Basic case: default, single-line spacing
    auto nCase1 = getXPath(pXmlDoc, "/root/page[1]/body/txt[1]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(3995), nCase1);
    CPPUNIT_ASSERT_LESS(sal_Int32(4005), nCase1);

    // Default spacing with snap-to-grid disabled
    auto nCase2 = getXPath(pXmlDoc, "/root/page[2]/body/txt[1]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(2175), nCase2);
    CPPUNIT_ASSERT_LESS(sal_Int32(2185), nCase2);

    // 1.15-line spacing
    auto nCase3 = getXPath(pXmlDoc, "/root/page[3]/body/txt[1]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(4295), nCase3);
    CPPUNIT_ASSERT_LESS(sal_Int32(4305), nCase3);

    // 1.5-line spacing
    auto nCase4 = getXPath(pXmlDoc, "/root/page[4]/body/txt[1]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(4995), nCase4);
    CPPUNIT_ASSERT_LESS(sal_Int32(5005), nCase4);

    // 2.0-line spacing
    auto nCase5 = getXPath(pXmlDoc, "/root/page[5]/body/txt[1]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(5995), nCase5);
    CPPUNIT_ASSERT_LESS(sal_Int32(6005), nCase5);

    // Prop spacing, 2.5x
    auto nCase6 = getXPath(pXmlDoc, "/root/page[6]/body/txt[1]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(6995), nCase6);
    CPPUNIT_ASSERT_LESS(sal_Int32(7005), nCase6);

    // At-least 0 spacing: this is a special case that disables extra grid spacing
    auto nCase7 = getXPath(pXmlDoc, "/root/page[7]/body/txt[1]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(2575), nCase7);
    CPPUNIT_ASSERT_LESS(sal_Int32(2585), nCase7);

    // At-least 0.01": should match the basic case
    auto nCase8 = getXPath(pXmlDoc, "/root/page[8]/body/txt[1]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(3995), nCase8);
    CPPUNIT_ASSERT_LESS(sal_Int32(4005), nCase8);

    // At-least 4"
    auto nCase9 = getXPath(pXmlDoc, "/root/page[9]/body/txt[1]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(11515), nCase9);
    CPPUNIT_ASSERT_LESS(sal_Int32(11525), nCase9);

    // Fixed 1"
    auto nCase10 = getXPath(pXmlDoc, "/root/page[10]/body/txt[1]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(2875), nCase10);
    CPPUNIT_ASSERT_LESS(sal_Int32(2885), nCase10);

    // Fixed 2"
    auto nCase11 = getXPath(pXmlDoc, "/root/page[11]/body/txt[1]/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(5755), nCase11);
    CPPUNIT_ASSERT_LESS(sal_Int32(5765), nCase11);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf149089)
{
    createSwDoc("tdf149089.fodt");
    auto pXmlDoc = parseLayoutDump();

    // Tests that kern portions aren't inserted for lines-and-chars grids when the
    // MS_WORD_COMP_GRID_METRICS compatibility flag is set.
    //
    // Without the fix, this would be 4
    sal_Int32 nKernPors
        = getXPathContent(pXmlDoc, "count(//SwLinePortion[@type='PortionType::Kern'])").toInt32();
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), nKernPors);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf165089)
{
    createSwDoc("tdf165089.fodt");
    auto pXmlDoc = parseLayoutDump();

    // Tests that the grid is not centered when the MS_WORD_COMP_GRID_METRICS flag is set.
    //
    // Without the fix, this would be 6703
    auto nTop = getXPath(pXmlDoc, "/root/page/body/txt/infos/bounds", "top").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(1400), nTop);
    CPPUNIT_ASSERT_LESS(sal_Int32(1450), nTop);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf88908)
{
    createSwDoc("tdf88908.fodt");
    auto pXmlDoc = parseLayoutDump();

    assertXPath(pXmlDoc, "/root/page/body/txt/SwParaPortion/SwLineLayout[1]", "portion",
                u"                                  Without  the  fix,  ");
    assertXPath(pXmlDoc, "/root/page/body/txt/SwParaPortion/SwLineLayout[2]", "portion",
                u"this  will  use  one  line");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf166152)
{
    createSwDoc("tdf166152.fodt");

    auto* pWrtShell = getSwDocShell()->GetWrtShell();
    auto& rTextFrame
        = dynamic_cast<SwTextFrame&>(*pWrtShell->GetLayout()->GetLower()->GetLower()->GetLower());
    const SwScriptInfo* pSI = rTextFrame.GetScriptInfo();

    // Sanity check script changes before the edit
    CPPUNIT_ASSERT_EQUAL(size_t(8), pSI->CountScriptChg());

    std::vector<sal_Int32> aExpectedChgBefore{ 5, 10, 15, 20, 25, 30, 35, 40 };
    std::vector<sal_uInt8> aExpectedTypeBefore{ 3, 1, 3, 1, 3, 1, 3, 1 };
    for (size_t i = 0; i < 8; ++i)
    {
        CPPUNIT_ASSERT_EQUAL(aExpectedChgBefore.at(i), pSI->GetScriptChg(i).get());
        CPPUNIT_ASSERT_EQUAL(aExpectedTypeBefore.at(i), pSI->GetScriptType(i));
    }

    // Insert text somewhere in the middle of the document.
    // This causes an incremental update of the script change vector.
    pWrtShell->Right(SwCursorSkipMode::Chars, /*bSelect*/ false, /*nCount*/ 17,
                     /*bBasicCall*/ false);
    pWrtShell->Insert(u"A"_ustr);

    // Check script changes after edit
    //
    // Without the fix, the count of script changes will be 11
    CPPUNIT_ASSERT_EQUAL(size_t(8), pSI->CountScriptChg());

    std::vector<sal_Int32> aExpectedChgAfter{ 5, 10, 15, 21, 26, 31, 36, 41 };
    std::vector<sal_uInt8> aExpectedTypeAfter{ 3, 1, 3, 1, 3, 1, 3, 1 };
    for (size_t i = 0; i < 8; ++i)
    {
        CPPUNIT_ASSERT_EQUAL(aExpectedChgAfter.at(i), pSI->GetScriptChg(i).get());
        CPPUNIT_ASSERT_EQUAL(aExpectedTypeAfter.at(i), pSI->GetScriptType(i));
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf166210)
{
    // Given a document with a table, inside which there are two conditionally hidden sections
    createSwDoc("tdf166210.fodt");

    auto xTextSectionsSupplier = mxComponent.queryThrow<css::text::XTextSectionsSupplier>();
    auto xSections = xTextSectionsSupplier->getTextSections();
    CPPUNIT_ASSERT(xSections);
    auto xSection1 = xSections->getByName(u"Section1"_ustr).queryThrow<css::beans::XPropertySet>();
    auto xSection2 = xSections->getByName(u"Section2"_ustr).queryThrow<css::beans::XPropertySet>();

    Scheduler::ProcessEventsToIdle();
    auto pXmlDoc = parseLayoutDump();
    auto rowHeight1 = getXPath(pXmlDoc, "//body/tab/infos/bounds", "height").toInt32();

    // Hide first section
    xSection1->setPropertyValue(u"Condition"_ustr, cpo::uno::Any(u"1"_ustr));
    Scheduler::ProcessEventsToIdle();
    pXmlDoc = parseLayoutDump();
    auto rowHeight2 = getXPath(pXmlDoc, "//body/tab/infos/bounds", "height").toInt32();
    // Make sure that the table has shrunk its height
    CPPUNIT_ASSERT_LESS(rowHeight1, rowHeight2);

    // Hide second section
    xSection2->setPropertyValue(u"Condition"_ustr, cpo::uno::Any(u"1"_ustr));
    Scheduler::ProcessEventsToIdle();
    pXmlDoc = parseLayoutDump();
    auto rowHeight3 = getXPath(pXmlDoc, "//body/tab/infos/bounds", "height").toInt32();
    // Make sure that the table has shrunk its height
    CPPUNIT_ASSERT_LESS(rowHeight2, rowHeight3);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf88752)
{
    createSwDoc("tdf88752.fodt");
    auto pXmlDoc = parseLayoutDump();

    // Without the fix in place, this test would have failed with
    // - Expected: 1
    // - Actual  : 2
    assertXPath(pXmlDoc, "/root/page", 1);

    auto nHeight = getXPath(pXmlDoc, "/root/page[1]//row[3]//txt/infos/bounds", "height").toInt32();

    CPPUNIT_ASSERT_GREATER(sal_Int32(1670), nHeight);

    // Without the fix in place, this test would have failed with
    // - Expected less than: 1690
    // - Actual  : 3480
    CPPUNIT_ASSERT_LESS(sal_Int32(1690), nHeight);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf167583)
{
    createSwDoc("tdf167583.fodt");
    auto pXmlDoc = parseLayoutDump();

    // Verifies that the ADJUST_TABLE_LINE_HEIGHTS_TO_GRID_HEIGHT compat flag defaults to true
    // Otherwise, this test would have failed with
    // - Expected: 2
    // - Actual  : 1
    assertXPath(pXmlDoc, "/root/page", 2);

    auto nHeight = getXPath(pXmlDoc, "/root/page[1]//row[3]//txt/infos/bounds", "height").toInt32();

    CPPUNIT_ASSERT_GREATER(sal_Int32(3400), nHeight);
    CPPUNIT_ASSERT_LESS(sal_Int32(3500), nHeight);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordImportTypography)
{
    createSwDoc("word-import-typography.docx");
    CPPUNIT_ASSERT(getProperty<bool>(getShape(1), u"TextAutoGrowWidth"_ustr));
    uno::Reference<beans::XPropertySet> xShape(getShape(1), uno::UNO_QUERY_THROW);
    uno::Reference<beans::XPropertySet> xFrame(
        xShape->getPropertyValue(u"TextBoxContent"_ustr), uno::UNO_QUERY_THROW);
    CPPUNIT_ASSERT_EQUAL(text::SizeType::VARIABLE, getProperty<sal_Int16>(xFrame, u"WidthType"_ustr));
    xShape->setPropertyValue(u"TextAutoGrowWidth"_ustr, uno::Any(false));
    CPPUNIT_ASSERT_EQUAL(text::SizeType::MIN, getProperty<sal_Int16>(xFrame, u"WidthType"_ustr));
    xShape->setPropertyValue(u"TextAutoGrowWidth"_ustr, uno::Any(true));
    xShape->setPropertyValue(u"TextWordWrap"_ustr, uno::Any(true));
    CPPUNIT_ASSERT_EQUAL(text::SizeType::FIX, getProperty<sal_Int16>(xFrame, u"WidthType"_ustr));
    xShape->setPropertyValue(u"TextWordWrap"_ustr, uno::Any(false));
    CPPUNIT_ASSERT_EQUAL(text::SizeType::VARIABLE, getProperty<sal_Int16>(xFrame, u"WidthType"_ustr));
    SwWrtShell* pShell = getSwDocShell()->GetWrtShell();
    pShell->SttEndDoc(true);
    // 1440 twips page margin + two characters at the 16pt paragraph mark size.
    auto pXmlDoc = parseLayoutDump();
    const auto nPageLeft = getXPath(pXmlDoc, "/root/page[1]/infos/bounds", "left").toInt32();
    CPPUNIT_ASSERT_EQUAL(tools::Long(2080), pShell->GetCharRect().Left() - nPageLeft);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordTableGridProportional)
{
    createSwDoc("word-table-grid-proportional.docx");
    auto pXmlDoc = parseLayoutDump();
    // Three table lines must use the 312-twip grid, without another 150% factor.
    // Before the fix, this was 1248 rather than 936.
    assertXPath(pXmlDoc, "/root/page[1]/body/tab/row/cell/txt/infos/bounds", "height", u"936");
    // The same proportional paragraph outside the table keeps normal spacing.
    assertXPath(pXmlDoc, "/root/page[1]/body/txt/infos/bounds", "height", u"1248");
}


// Generate minimal regression input in a temporary ZIP. No DOCX samples or
// downloaded government documents are stored in the source repository.
static void writeWordMacPart(const uno::Reference<embed::XStorage>& xStorage,
                const OUString& rName, const OString& rContent)
{
    auto xStream = xStorage->openStreamElement(
        rName, embed::ElementModes::WRITE | embed::ElementModes::TRUNCATE);
    auto xOutput = xStream->getOutputStream();
    xOutput->writeBytes(uno::Sequence<sal_Int8>(
        reinterpret_cast<const sal_Int8*>(rContent.getStr()), rContent.getLength()));
    xOutput->closeOutput();
}

static void makeWordMacPackage(utl::TempFileNamed& rFile, bool bBodyGrid,
                               bool bExplicitParagraphDefaults, bool bManualPageBreak = false,
                               bool bSmallGridTable = false, bool bNarrowBoldTable = false,
                               bool bMissingTheme = false, bool bEmptyStyle = false,
                               bool bCompression = false, bool bPunctuationWrap = false,
                               bool bAutoSpacingWrap = false, sal_Int16 nCompressionGrid = 0,
                               sal_Int16 nLegacyBorder = 0, bool bContinuousSection = false,
                               sal_Int16 nYaHeiSize = 0)
{
    auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
        ZIP_STORAGE_FORMAT_STRING, rFile.GetURL(), embed::ElementModes::READWRITE);
    writeWordMacPart(xRoot, u"[Content_Types].xml"_ustr,
        R"xml(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="xml" ContentType="application/xml"/><Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/><Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/><Override PartName="/word/settings.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"/></Types>)xml"_ostr);
    auto xRels = xRoot->openStorageElement(u"_rels"_ustr, embed::ElementModes::WRITE);
    writeWordMacPart(xRels, u".rels"_ustr,
        R"xml(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/></Relationships>)xml"_ostr);
    uno::Reference<embed::XTransactedObject>(xRels, uno::UNO_QUERY_THROW)->commit();
    auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
    auto xWordRels = xWord->openStorageElement(u"_rels"_ustr, embed::ElementModes::WRITE);
    writeWordMacPart(xWordRels, u"document.xml.rels"_ustr,
        R"xml(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/><Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings" Target="settings.xml"/></Relationships>)xml"_ostr);
    uno::Reference<embed::XTransactedObject>(xWordRels, uno::UNO_QUERY_THROW)->commit();
    OString aDefaults = bExplicitParagraphDefaults
        ? R"xml(<w:pPrDefault><w:pPr><w:spacing w:after="0"/></w:pPr></w:pPrDefault>)xml"_ostr
        : OString();
    writeWordMacPart(xWord, u"styles.xml"_ustr,
        R"xml(<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:docDefaults><w:rPrDefault><w:rPr><w:rFonts w:ascii="Liberation Serif" w:eastAsia="Liberation Serif" w:hAnsi="Liberation Serif" w:cs="Liberation Serif"/></w:rPr></w:rPrDefault>)xml"_ostr
        + aDefaults + R"xml(</w:docDefaults><w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/></w:style></w:styles>)xml"_ostr);
    OString aProperties = bBodyGrid
        ? R"xml(<w:pPr><w:spacing w:before="0" w:after="0" w:line="360" w:lineRule="auto"/><w:snapToGrid/><w:rPr><w:rFonts w:ascii="Liberation Serif" w:eastAsia="Liberation Serif"/><w:sz w:val="36"/></w:rPr></w:pPr>)xml"_ostr
        : R"xml(<w:pPr><w:spacing w:line="600" w:lineRule="exact"/></w:pPr>)xml"_ostr;
    OString aRun = bBodyGrid
        ? R"xml(<w:r><w:rPr><w:rFonts w:ascii="Liberation Serif" w:eastAsia="Liberation Serif"/><w:sz w:val="36"/></w:rPr><w:t>First line</w:t><w:br/><w:t>Second line</w:t><w:br/><w:t>Third line</w:t></w:r>)xml"_ostr
        : R"xml(<w:r><w:rPr><w:rFonts w:ascii="Liberation Serif" w:eastAsia="Liberation Serif"/><w:sz w:val="32"/></w:rPr><w:t>Paragraph defaults</w:t></w:r>)xml"_ostr;
    if (nYaHeiSize)
    {
        aProperties = aProperties.replaceAll("Liberation Serif"_ostr, "Microsoft YaHei"_ostr)
            .replaceAll("w:val=\"36\""_ostr,
                        "w:val=\""_ostr + OString::number(nYaHeiSize * 2) + "\""_ostr);
        aRun = aRun.replaceAll("Liberation Serif"_ostr, "Microsoft YaHei"_ostr)
            .replaceAll("w:val=\"36\""_ostr,
                        "w:val=\""_ostr + OString::number(nYaHeiSize * 2) + "\""_ostr);
    }
    if (bSmallGridTable)
    {
        aProperties = aProperties.replaceAll("w:val=\"36\""_ostr, "w:val=\"24\""_ostr);
        aRun = aRun.replaceAll("w:val=\"36\""_ostr, "w:val=\"24\""_ostr);
    }
    if (bMissingTheme)
        aRun = aRun.replaceAll(
            R"xml(<w:rFonts w:ascii="Liberation Serif" w:eastAsia="Liberation Serif"/>)xml"_ostr,
            R"xml(<w:rFonts w:asciiTheme="minorHAnsi" w:hAnsiTheme="minorHAnsi" w:eastAsiaTheme="minorEastAsia"/>)xml"_ostr);
    if (bEmptyStyle)
        aProperties = R"xml(<w:pPr><w:pStyle w:val=""/><w:spacing w:before="240" w:line="600" w:lineRule="exact"/></w:pPr>)xml"_ostr;
    OString aParagraph = "<w:p>"_ostr + aProperties + aRun + "</w:p>"_ostr;
    if (bManualPageBreak)
        aParagraph += R"xml(<w:p><w:pPr><w:pageBreakBefore w:val="0"/><w:spacing w:line="600" w:lineRule="exact"/></w:pPr><w:r><w:br w:type="page"/></w:r><w:r><w:t>Second page</w:t></w:r></w:p>)xml"_ostr;
    if (bBodyGrid && !bSmallGridTable)
        aParagraph = aParagraph + aParagraph + "<w:p>"_ostr + aProperties
            + "</w:p><w:p>"_ostr + aProperties + "</w:p>"_ostr;
    if (bSmallGridTable)
        aParagraph = R"xml(<w:tbl><w:tblPr><w:tblW w:w="9026" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="9026"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="9026" w:type="dxa"/></w:tcPr>)xml"_ostr
            + aParagraph + "<w:p>"_ostr + aProperties + "</w:p></w:tc></w:tr></w:tbl>"_ostr;
    if (bNarrowBoldTable || bPunctuationWrap || bAutoSpacingWrap)
        aParagraph = R"xml(<w:tbl><w:tblPr><w:tblW w:w="698" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="108" w:type="dxa"/><w:right w:w="108" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="698"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="698" w:type="dxa"/><w:tcBorders><w:left w:val="single" w:sz="4"/><w:right w:val="single" w:sz="4"/></w:tcBorders></w:tcPr><w:p><w:pPr><w:snapToGrid w:val="0"/><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="auto"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="24"/><w:b/></w:rPr><w:t>架构维度</w:t></w:r></w:p></w:tc></w:tr></w:tbl>)xml"_ostr;
    if (bPunctuationWrap)
    {
        aParagraph = aParagraph.replaceAll("698"_ostr, "4000"_ostr)
            .replaceAll("108"_ostr, "0"_ostr)
            .replaceAll("w:val=\"24\""_ostr, "w:val=\"32\""_ostr)
            .replaceAll("<w:b/>"_ostr, ""_ostr)
            .replaceAll("架构维度"_ostr, "甲乙丙丁戊己，庚辛壬癸子丑"_ostr);
    }
    if (bAutoSpacingWrap)
    {
        aParagraph = aParagraph.replaceAll("698"_ostr, "700"_ostr)
            .replaceAll("108"_ostr, "0"_ostr)
            .replaceAll("<w:b/>"_ostr, ""_ostr)
            .replaceAll("<w:pPr>"_ostr, "<w:pPr><w:autoSpaceDE/><w:autoSpaceDN/>"_ostr)
            .replaceAll("架构维度"_ostr, "甲A乙"_ostr);
    }
    if (bPunctuationWrap || bAutoSpacingWrap)
        aParagraph = aParagraph.replaceAll(
            R"xml(<w:tcBorders><w:left w:val="single" w:sz="4"/><w:right w:val="single" w:sz="4"/></w:tcBorders>)xml"_ostr,
            ""_ostr);
    if (nLegacyBorder)
    {
        const OString aStyle = nLegacyBorder == 1 ? "outset"_ostr : "inset"_ostr;
        aParagraph = aParagraph.replaceAll("<w:tblPr>"_ostr,
            R"xml(<w:tblPr><w:tblBorders><w:top w:val=")xml"_ostr + aStyle
            + R"xml(" w:sz="6" w:color="FF0000"/></w:tblBorders>)xml"_ostr);
    }
    if (nCompressionGrid)
    {
        aParagraph = aParagraph.replaceAll(R"xml(<w:snapToGrid w:val="0"/>)xml"_ostr,
                                           "<w:snapToGrid/>"_ostr);
        if (nCompressionGrid == 2)
            aParagraph = aParagraph.replaceAll(R"xml(w:lineRule="auto")xml"_ostr,
                                               R"xml(w:lineRule="exact")xml"_ostr);
    }
    if (bContinuousSection)
        aParagraph += R"xml(<w:p><w:pPr><w:sectPr><w:type w:val="continuous"/><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:pPr></w:p><w:p><w:r><w:t>After section</w:t></w:r></w:p>)xml"_ostr;
    writeWordMacPart(xWord, u"document.xml"_ustr,
        R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body>)xml"_ostr
        + aParagraph + R"xml(<w:sectPr>)xml"_ostr
        + R"xml(<w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:body></w:document>)xml"_ostr);
    OString aSettings = R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:compat><w:adjustLineHeightInTable/><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val="15"/></w:compat></w:settings>)xml"_ostr;
    if (bCompression || bPunctuationWrap)
        aSettings = aSettings.replaceAll("<w:compat>"_ostr,
            R"xml(<w:characterSpacingControl w:val="compressPunctuation"/><w:compat>)xml"_ostr);
    if (nCompressionGrid)
        aSettings = aSettings.replaceAll("<w:compat>"_ostr,
            "<w:compat><w:doNotUseEastAsianBreakRules/>"_ostr);
    writeWordMacPart(xWord, u"settings.xml"_ustr, aSettings);
    uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
    uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordManualPageBreakOverridesParagraphDefault)
{
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true, true);
    loadFromURL(aFile.GetURL());
    // pageBreakBefore=false controls the paragraph's automatic break, not
    // an explicit page break inside a run. Word places this text on page 2.
    CPPUNIT_ASSERT_EQUAL(2, getPages());
    assertXPathContent(parseLayoutDump(), "/root/page[2]/body/txt[1]", u"Second page");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacRowCellSpacing)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    for (bool bMac : { false, true })
    {
        const OUString aValue = bMac ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aName.pData, aValue.pData);
        for (bool bMerged : { false, true })
        {
            sal_Int32 nZeroHeight = 0;
            sal_Int32 nFirstSpacedHeight = 0;
            sal_Int32 nFirstSpacedLeft = 0;
            sal_Int32 nFirstSpacedTop = 0;
            sal_Int32 nFirstSpacedWidth = 0;
            for (sal_Int32 nSpacing : { 0, 20, 40 })
            {
                utl::TempFileNamed aFile;
                aFile.EnableKillingFile();
                makeWordMacPackage(aFile, false, true);
                {
                    auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                        ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
                    auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
                    const OString aCell = R"xml(<w:tc><w:tcPr><w:tcW w:w="2000" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="exact"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:sz w:val="24"/></w:rPr><w:t>Cell</w:t></w:r></w:p></w:tc>)xml"_ostr;
                    const OString aRow = R"xml(<w:tr><w:trPr><w:tblCellSpacing w:w=")xml"_ostr
                        + OString::number(nSpacing) + R"xml(" w:type="dxa"/></w:trPr>)xml"_ostr
                        + aCell + aCell + "</w:tr>"_ostr;
                    const OString aSecondRow = bMerged
                        ? R"xml(<w:tr><w:trPr><w:tblCellSpacing w:w=")xml"_ostr
                            + OString::number(nSpacing) + R"xml(" w:type="dxa"/></w:trPr>)xml"_ostr
                            + aCell.replaceAll("w:w=\"2000\""_ostr, "w:w=\"4000\""_ostr)
                                .replaceAll("<w:tcPr>"_ostr, "<w:tcPr><w:gridSpan w:val=\"2\"/>"_ostr)
                            + "</w:tr>"_ostr
                        : aRow;
                    writeWordMacPart(xWord, u"document.xml"_ustr,
                        R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="4000" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblBorders><w:top w:val="single" w:sz="8"/><w:bottom w:val="single" w:sz="8"/><w:left w:val="single" w:sz="8"/><w:right w:val="single" w:sz="8"/><w:insideH w:val="single" w:sz="8"/><w:insideV w:val="single" w:sz="8"/></w:tblBorders><w:tblCellMar><w:top w:w="10" w:type="dxa"/><w:bottom w:w="10" w:type="dxa"/><w:left w:w="10" w:type="dxa"/><w:right w:w="10" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="2000"/><w:gridCol w:w="2000"/></w:tblGrid>)xml"_ostr
                        + aRow + aSecondRow + R"xml(</w:tbl><w:p/><w:sectPr/></w:body></w:document>)xml"_ostr);
                    uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
                    uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
                }
                loadFromURL(aFile.GetURL());
                auto pDump = parseLayoutDump();
                const OString aRowBounds = "/root/page/body/tab/row[1]/infos/bounds"_ostr;
                const sal_Int32 nHeight = getXPath(pDump, aRowBounds, "height").toInt32();
                if (!nSpacing)
                    nZeroHeight = nHeight;
                else if (bMac && !bMerged)
                    CPPUNIT_ASSERT_GREATER(nZeroHeight + 39, nHeight);
                else
                    CPPUNIT_ASSERT_EQUAL(nZeroHeight, nHeight);
                if (bMac && !bMerged && nSpacing)
                {
                    const OString aCellPrint = "/root/page/body/tab/row[1]/cell[1]/infos/prtBounds"_ostr;
                    const sal_Int32 nLeft = getXPath(pDump, aCellPrint, "left").toInt32();
                    const sal_Int32 nTop = getXPath(pDump, aCellPrint, "top").toInt32();
                    const sal_Int32 nWidth = getXPath(pDump, aCellPrint, "width").toInt32();
                    if (nSpacing == 20)
                    {
                        nFirstSpacedHeight = nHeight;
                        nFirstSpacedLeft = nLeft;
                        nFirstSpacedTop = nTop;
                        nFirstSpacedWidth = nWidth;
                    }
                    else
                    {
                        // Mac Word uses two spacing units on each outside
                        // edge and one on each inside edge. Changing 20 to 40
                        // twips adds 40 outside plus 20 inside to a corner cell.
                        CPPUNIT_ASSERT_EQUAL(nFirstSpacedHeight + 60, nHeight);
                        CPPUNIT_ASSERT_EQUAL(nFirstSpacedLeft + 40, nLeft);
                        CPPUNIT_ASSERT_EQUAL(nFirstSpacedTop + 40, nTop);
                        CPPUNIT_ASSERT_EQUAL(nFirstSpacedWidth - 60, nWidth);
                    }
                }
                saveAndReload(TestFilter::DOCX);
                auto pXml = parseExport(u"word/document.xml"_ustr);
                assertXPath(pXml, "/w:document/w:body/w:tbl/w:tr/w:trPr/w:tblCellSpacing[@w:type='dxa' and @w:w='"_ostr
                    + OString::number(nSpacing) + "']"_ostr, 2);
                CPPUNIT_ASSERT_EQUAL(nHeight, getXPath(parseLayoutDump(), aRowBounds, "height").toInt32());
            }
        }
    }

    // Word's percentage columns divide the space left after reserving the
    // separated cell gaps. Unequal columns expose a different result from
    // applying the percentages to the frame slots themselves.
    const OUString aAutoFitName = u"COCOLIAN_WORD_MAC_AUTOFIT"_ustr;
    OUString aPreviousAutoFit;
    const bool bHadAutoFit = osl_getEnvironment(aAutoFitName.pData, &aPreviousAutoFit.pData)
        == osl_Process_E_None;
    comphelper::ScopeGuard aRestoreAutoFit([&] {
        if (bHadAutoFit)
            osl_setEnvironment(aAutoFitName.pData, aPreviousAutoFit.pData);
        else
            osl_clearEnvironment(aAutoFitName.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    osl_setEnvironment(aAutoFitName.pData, aOn.pData);
    for (sal_Int32 nSpacing : { 0, 15, 30 })
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            const OString aCell = R"xml(<w:tc><w:tcPr><w:tcW w:w="1000" w:type="pct"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="280" w:lineRule="exact"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:t>A</w:t></w:r></w:p></w:tc>)xml"_ostr;
            writeWordMacPart(xWord, u"document.xml"_ustr,
                R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="5000" w:type="pct"/><w:tblInd w:w="15" w:type="dxa"/><w:tblLayout w:type="autofit"/><w:tblCellMar><w:left w:w="15" w:type="dxa"/><w:right w:w="15" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="1661"/><w:gridCol w:w="6645"/></w:tblGrid><w:tr><w:trPr><w:tblCellSpacing w:w=")xml"_ostr
                + OString::number(nSpacing) + R"xml(" w:type="dxa"/></w:trPr>)xml"_ostr
                + aCell + aCell.replaceAll("w:w=\"1000\""_ostr, "w:w=\"4000\""_ostr)
                + R"xml(</w:tr></w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1800" w:right="1800"/></w:sectPr></w:body></w:document>)xml"_ostr);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        loadFromURL(aFile.GetURL());
        for (int nRound = 0; nRound < 2; ++nRound)
        {
            calcLayout();
            auto pDump = parseLayoutDump();
            const sal_Int32 nTableWidth = getXPath(pDump,
                "/root/page/body/tab/infos/prtBounds", "width").toInt32();
            const sal_Int32 nFirstWidth = getXPath(pDump,
                "/root/page/body/tab/row/cell[1]/infos/bounds", "width").toInt32();
            // For two columns the two outside edges use 2S each and the
            // two inside edges use S each. The first frame slot adds 3S.
            const sal_Int32 nExpected = (nTableWidth - 6 * nSpacing) / 5 + 3 * nSpacing;
            const OString aDiagnostic = "spacing="_ostr + OString::number(nSpacing)
                + " round="_ostr + OString::number(nRound)
                + " actual="_ostr + OString::number(nFirstWidth)
                + " expected="_ostr + OString::number(nExpected);
            CPPUNIT_ASSERT_MESSAGE(aDiagnostic.getStr(),
                std::abs(nFirstWidth - nExpected) <= 2);
            if (!nRound)
                saveAndReload(TestFilter::DOCX);
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacUrlBreakUnits)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    const OUString aBudgetName = u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr;
    OUString aPreviousBudget;
    const bool bHadBudget = osl_getEnvironment(aBudgetName.pData, &aPreviousBudget.pData)
        == osl_Process_E_None;
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aBudgetName.pData, aOn.pData);
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
        if (bHadBudget)
            osl_setEnvironment(aBudgetName.pData, aPreviousBudget.pData);
        else
            osl_clearEnvironment(aBudgetName.pData);
    });
    const std::array<OString, 3> aUrls = {
        "https://example.invalid/open-api/auth/v1/access_token"_ostr,
        "https://example.invalid/open-api/auth/v1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"_ostr,
        "https://example.invalid/api/auth/v1/access_token"_ostr };
    for (bool bMac : { false, true })
    {
        const OUString aValue = bMac ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aName.pData, aValue.pData);
        for (bool bField : { false, true })
        {
            for (bool bWordWrap : { true, false })
            {
                for (size_t nCase = 0; nCase < aUrls.size(); ++nCase)
                {
                    const auto& rUrl = aUrls[nCase];
                    utl::TempFileNamed aFile;
                    aFile.EnableKillingFile();
                    makeWordMacPackage(aFile, false, true);
                    {
                        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
                        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
                        const OString aRun = R"xml(<w:r><w:rPr><w:rFonts w:ascii="Liberation Mono" w:hAnsi="Liberation Mono"/><w:sz w:val="24"/></w:rPr><w:t>)xml"_ostr
                            + rUrl + "</w:t></w:r>"_ostr;
                        const OString aContents = bField
                            ? R"xml(<w:r><w:fldChar w:fldCharType="begin"/></w:r><w:r><w:instrText xml:space="preserve"> HYPERLINK "https://example.invalid/" </w:instrText></w:r><w:r><w:fldChar w:fldCharType="separate"/></w:r>)xml"_ostr
                                + aRun + R"xml(<w:r><w:fldChar w:fldCharType="end"/></w:r>)xml"_ostr
                            : aRun;
                        writeWordMacPart(xWord, u"document.xml"_ustr,
                            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="6200" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="6200"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="6200" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:wordWrap w:val=")xml"_ostr
                            + (bWordWrap ? "1"_ostr : "0"_ostr)
                            + R"xml("/><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="exact"/><w:ind w:firstLine="0"/><w:snapToGrid w:val="0"/></w:pPr>)xml"_ostr
                            + aContents + R"xml(</w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr/></w:body></w:document>)xml"_ostr);
                        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
                        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
                    }
                    loadFromURL(aFile.GetURL());
                    const OUString aExpected = bMac && !bWordWrap
                        ? OStringToOUString(rUrl.copy(0, 43), RTL_TEXTENCODING_UTF8)
                        : bMac && nCase == 0 ? u"https://example.invalid/open-"_ustr
                        : nCase == 2 ? u"https://example.invalid/api/auth/v1/"_ustr
                        : u"https://example.invalid/open-api/auth/v1/"_ustr;
                    for (bool bReload : { false, true })
                    {
                        if (bReload)
                            saveAndReload(TestFilter::DOCX);
                        auto pDump = parseLayoutDump();
                        assertXPath(pDump, "//tab/row/cell/txt/SwParaPortion/SwLineLayout[1]",
                            "portion", aExpected);
                        if (nCase == 0 && bWordWrap)
                            assertXPath(pDump, "//tab/row/cell/txt/SwParaPortion/SwLineLayout", 2);
                    }
                }
            }
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacWordWrap)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    for (const OString& rAlignment : { "left"_ostr, "both"_ostr })
    for (const OString& rLanguage : { "en-US"_ostr, "zh-CN"_ostr })
    for (bool bKinsoku : { false, true })
    for (sal_Int32 nWidth : { 3900, 4040 })
    for (sal_Int32 nCase : { 0, 1, 2, 3 })
    {
        const bool bWordWrap = nCase == 0 || nCase == 3;
        const bool bInherited = nCase == 2;
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            if (bInherited)
                writeWordMacPart(xWord, u"styles.xml"_ustr,
                    R"xml(<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/><w:pPr><w:wordWrap w:val="0"/></w:pPr></w:style></w:styles>)xml"_ostr);
            writeWordMacPart(xWord, u"document.xml"_ustr,
                R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr>)xml"_ostr
                + (bInherited || nCase == 3 ? OString() : "<w:wordWrap w:val=\""_ostr
                    + (bWordWrap ? "1"_ostr : "0"_ostr) + "\"/>"_ostr)
                + "<w:kinsoku w:val=\""_ostr + (bKinsoku ? "1"_ostr : "0"_ostr) + "\"/>"_ostr
                + "<w:jc w:val=\""_ostr + rAlignment + "\"/>"_ostr
                + R"xml(<w:snapToGrid w:val="0"/><w:spacing w:before="0" w:after="0"/><w:ind w:firstLine="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Liberation Mono" w:hAnsi="Liberation Mono"/><w:sz w:val="24"/><w:lang w:val="LANGUAGE"/></w:rPr><w:t>alpha beta</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="PAGE_WIDTH" w:h="6000"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/><w:docGrid w:type="default"/></w:sectPr></w:body></w:document>)xml"_ostr.replaceAll("LANGUAGE"_ostr, rLanguage)
                    .replaceAll("PAGE_WIDTH"_ostr, OString::number(nWidth)));
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        loadFromURL(aFile.GetURL());
        for (bool bReload : { false, true })
        {
            if (bReload)
            {
                saveAndReload(TestFilter::DOCX);
                auto pXml = parseExport(bInherited ? u"word/styles.xml"_ustr : u"word/document.xml"_ustr);
                const OString aPath = bInherited
                    ? "/w:styles/w:style[@w:styleId='Normal']/w:pPr/w:wordWrap"_ostr
                    : "/w:document/w:body/w:p/w:pPr/w:wordWrap"_ostr;
                if (nCase == 3)
                    assertXPath(pXml, aPath, 0);
                else
                    assertXPath(pXml, aPath + (bWordWrap ? "[@w:val='1']"_ostr : "[@w:val='0']"_ostr), 1);
            }
            const OString aInputs = rAlignment + " "_ostr + rLanguage + " kinsoku="_ostr + (bKinsoku ? "1"_ostr : "0"_ostr)
                + " case="_ostr + OString::number(nCase) + " width="_ostr + OString::number(nWidth);
            auto pDump = parseLayoutDump();
            const OString aLines = "/root/page[1]/body/txt/SwParaPortion/SwLineLayout"_ostr;
            assertXPath(pDump, aLines, 2);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(aInputs.getStr(),
                bWordWrap ? u"alpha "_ustr : nWidth == 3900 ? u"alpha b"_ustr : u"alpha be"_ustr,
                getXPath(pDump, aLines + "[1]"_ostr, "portion"));
            CPPUNIT_ASSERT_EQUAL_MESSAGE(aInputs.getStr(),
                bWordWrap ? u"beta"_ustr : nWidth == 3900 ? u"eta"_ustr : u"ta"_ustr,
                getXPath(pDump, aLines + "[2]"_ostr, "portion"));
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacCellNoWrap)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="5000" w:type="pct"/></w:tblPr><w:tblGrid><w:gridCol w:w="4500"/><w:gridCol w:w="4500"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="2500" w:type="pct"/><w:noWrap/></w:tcPr><w:p><w:r><w:t>content-type</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:tcW w:w="2500" w:type="pct"/><w:noWrap w:val="0"/></w:tcPr><w:p><w:r><w:t>May wrap</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr/></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    loadFromURL(aFile.GetURL());
    saveAndReload(TestFilter::DOCX);
    auto pXml = parseExport(u"word/document.xml"_ustr);
    assertXPath(pXml, "/w:document/w:body/w:tbl/w:tr/w:tc[1]/w:tcPr/w:noWrap", "val", u"true");
    assertXPath(pXml, "/w:document/w:body/w:tbl/w:tr/w:tc[2]/w:tcPr/w:noWrap", "val", u"false");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacAutoFitNoWrap)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    const OUString aFitName = u"COCOLIAN_WORD_MAC_AUTOFIT"_ustr;
    OUString aPreviousFit;
    const bool bHadFit = osl_getEnvironment(aFitName.pData, &aPreviousFit.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestoreFit([&] {
        if (bHadFit)
            osl_setEnvironment(aFitName.pData, aPreviousFit.pData);
        else
            osl_clearEnvironment(aFitName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="5000" w:type="pct"/></w:tblPr><w:tblGrid><w:gridCol w:w="900"/><w:gridCol w:w="8100"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="500" w:type="pct"/><w:noWrap/></w:tcPr><w:p><w:r><w:rPr><w:rFonts w:ascii="Liberation Serif" w:hAnsi="Liberation Serif"/><w:sz w:val="24"/></w:rPr><w:t>Unbreakable-parameter-name</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:tcW w:w="4500" w:type="pct"/><w:noWrap w:val="0"/></w:tcPr><w:p><w:r><w:rPr><w:rFonts w:ascii="Liberation Serif" w:hAnsi="Liberation Serif"/><w:sz w:val="24"/></w:rPr><w:t>May wrap</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr/></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    const OUString aOff = u"0"_ustr;
    osl_setEnvironment(aFitName.pData, aOff.pData);
    loadFromURL(aFile.GetURL());
    auto pFixed = parseLayoutDump();
    const auto nFixed = getXPath(pFixed, "/root/page[1]/body/tab/row/cell[1]/infos/bounds", "width").toInt32();
    osl_setEnvironment(aFitName.pData, aOn.pData);
    loadFromURL(aFile.GetURL());
    Scheduler::ProcessEventsToIdle();
    auto pFit = parseLayoutDump();
    const auto nFit = getXPath(pFit, "/root/page[1]/body/tab/row/cell[1]/infos/bounds", "width").toInt32();
    // Cached grid widths cannot split a noWrap cell merely to preserve its
    // preferred percentage. AutoFit must expand it to its content minimum.
    CPPUNIT_ASSERT_GREATER(nFixed + 200, nFit);
    CPPUNIT_ASSERT_EQUAL(u"Unbreakable-parameter-name"_ustr,
        getXPath(pFit, "/root/page[1]/body/tab/row/cell[1]/txt/SwParaPortion/SwLineLayout[1]", "portion"));
    CPPUNIT_ASSERT_EQUAL(1, getPages());
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacFixedLayoutNoAutoFit)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    const OUString aFitName = u"COCOLIAN_WORD_MAC_AUTOFIT"_ustr;
    OUString aPreviousFit;
    const bool bHadFit = osl_getEnvironment(aFitName.pData, &aPreviousFit.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestoreFit([&] {
        if (bHadFit)
            osl_setEnvironment(aFitName.pData, aPreviousFit.pData);
        else
            osl_clearEnvironment(aFitName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="5000" w:type="pct"/><w:tblLayout w:type="fixed"/></w:tblPr><w:tblGrid><w:gridCol w:w="900"/><w:gridCol w:w="8100"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="500" w:type="pct"/><w:noWrap/></w:tcPr><w:p><w:r><w:rPr><w:rFonts w:ascii="Liberation Serif" w:hAnsi="Liberation Serif"/><w:sz w:val="24"/></w:rPr><w:t>Unbreakable-parameter-name</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:tcW w:w="4500" w:type="pct"/><w:noWrap w:val="0"/></w:tcPr><w:p><w:r><w:rPr><w:rFonts w:ascii="Liberation Serif" w:hAnsi="Liberation Serif"/><w:sz w:val="24"/></w:rPr><w:t>May wrap</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr/></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    const OUString aOff = u"0"_ustr;
    osl_setEnvironment(aFitName.pData, aOff.pData);
    loadFromURL(aFile.GetURL());
    auto pFixed = parseLayoutDump();
    const auto nFixed = getXPath(pFixed, "/root/page[1]/body/tab/row/cell[1]/infos/bounds", "width").toInt32();
    osl_setEnvironment(aFitName.pData, aOn.pData);
    loadFromURL(aFile.GetURL());
    Scheduler::ProcessEventsToIdle();
    auto pFit = parseLayoutDump();
    const auto nFit = getXPath(pFit, "/root/page[1]/body/tab/row/cell[1]/infos/bounds", "width").toInt32();
    // A fixed table must retain its column grid even with long text and
    // noWrap. Accidental AutoFit used to push subsequent columns off-page.
    CPPUNIT_ASSERT_EQUAL(nFixed, nFit);
    CPPUNIT_ASSERT_GREATER(sal_Int32(0), nFit);
    CPPUNIT_ASSERT_LESS(sal_Int32(1200), nFit);
    CPPUNIT_ASSERT(getXPath(pFit,
        "/root/page[1]/body/tab/row/cell[1]/txt/SwParaPortion/SwLineLayout[1]", "portion")
        != u"Unbreakable-parameter-name"_ustr);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacLegacyPercentTablePadding)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    const OUString aFitName = u"COCOLIAN_WORD_MAC_AUTOFIT"_ustr;
    OUString aPreviousFit;
    const bool bHadFit = osl_getEnvironment(aFitName.pData, &aPreviousFit.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestoreFit([&] {
        if (bHadFit)
            osl_setEnvironment(aFitName.pData, aPreviousFit.pData);
        else
            osl_clearEnvironment(aFitName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    const OString aDocument = R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="5000" w:type="pct"/><w:tblInd w:w="0" w:type="dxa"/><w:tblLayout w:type="autofit"/><w:tblCellMar><w:left w:w="108" w:type="dxa"/><w:right w:w="108" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4500"/><w:gridCol w:w="4500"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="2500" w:type="pct"/></w:tcPr><w:p><w:r><w:t>Alpha</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:tcW w:w="2500" w:type="pct"/></w:tcPr><w:p><w:r><w:t>Beta</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr;
    osl_setEnvironment(aFitName.pData, aOn.pData);
    for (bool bCentered : {false, true})
    {
        for (bool bEnabled : {false, true})
        {
            const OUString aValue = bEnabled ? u"1"_ustr : u"0"_ustr;
            osl_setEnvironment(aName.pData, aValue.pData);
            sal_Int32 nLegacyWidth = 0;
            for (sal_Int32 nMode : {14, 15})
            {
                {
                    auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                        ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
                    auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
                    writeWordMacPart(xWord, u"document.xml"_ustr,
                        bCentered ? aDocument.replaceFirst(
                            R"xml(<w:tblInd w:w="0" w:type="dxa"/>)xml"_ostr,
                            R"xml(<w:jc w:val="center"/>)xml"_ostr) : aDocument);
                    const OString aXml = R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:compat><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val=")xml"_ostr
                        + OString::number(nMode) + R"xml("/></w:compat></w:settings>)xml"_ostr;
                    writeWordMacPart(xWord, u"settings.xml"_ustr, aXml);
                    uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
                    uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
                }
                loadFromURL(aFile.GetURL());
                Scheduler::ProcessEventsToIdle();
                const auto nWidth = getXPath(parseLayoutDump(),
                    "/root/page[1]/body/tab/infos/prtBounds", "width").toInt32();
                if (bEnabled)
                {
                    const auto* pTable = SwTable::FindTable(
                        (*getSwDocShell()->GetDoc()->GetTableFrameFormats())[0]);
                    CPPUNIT_ASSERT(pTable);
                    const auto* pLayout = pTable->GetHTMLTableLayout();
                    CPPUNIT_ASSERT_MESSAGE("AutoFit layout must be attached", pLayout);
                    CPPUNIT_ASSERT_MESSAGE("legacy outer padding",
                        std::abs(pTable->GetWordMacPercentPadding() - (nMode == 14 ? 216 : 0)) <= 1);
                }
                if (nMode == 14)
                    nLegacyWidth = nWidth;
                else
                {
                    const OString aFailure = "centered="_ostr + OString::number(bCentered)
                        + " profile="_ostr + OString::number(bEnabled)
                        + " legacy="_ostr + OString::number(nLegacyWidth)
                        + " modern="_ostr + OString::number(nWidth);
                    CPPUNIT_ASSERT_MESSAGE(aFailure.getStr(),
                        std::abs(nLegacyWidth - nWidth - (bEnabled ? 216 : 0)) <= 2);
                }
            }
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacAutoFitPageWidth)
{
    const std::array<OUString, 2> aNames = {
        u"COCOLIAN_WORD_MAC_METRICS"_ustr, u"COCOLIAN_WORD_MAC_AUTOFIT"_ustr };
    std::array<OUString, 2> aPrevious;
    std::array<bool, 2> aHadPrevious;
    for (size_t i = 0; i < aNames.size(); ++i)
        aHadPrevious[i] = osl_getEnvironment(aNames[i].pData, &aPrevious[i].pData)
            == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
        {
            if (aHadPrevious[i])
                osl_setEnvironment(aNames[i].pData, aPrevious[i].pData);
            else
                osl_clearEnvironment(aNames[i].pData);
        }
    });
    const OUString aOn = u"1"_ustr;
    for (const auto& rName : aNames)
        osl_setEnvironment(rName.pData, aOn.pData);
    for (bool bGrow : { false, true })
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            const OString aDocument = R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="5000" w:type="pct"/><w:tblInd w:w="0" w:type="dxa"/><w:tblLayout w:type="autofit"/><w:tblCellMar><w:left w:w="108" w:type="dxa"/><w:right w:w="108" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="2768"/><w:gridCol w:w="2768"/><w:gridCol w:w="2770"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="1666" w:type="pct"/></w:tcPr><w:p><w:r><w:rPr><w:rFonts w:ascii="Liberation Serif" w:hAnsi="Liberation Serif"/><w:sz w:val="24"/></w:rPr><w:t>TOKEN</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:tcW w:w="1666" w:type="pct"/></w:tcPr><w:p><w:r><w:rPr><w:rFonts w:ascii="Liberation Serif" w:hAnsi="Liberation Serif"/><w:sz w:val="24"/></w:rPr><w:t>TOKEN</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:tcW w:w="1668" w:type="pct"/></w:tcPr><w:p><w:r><w:rPr><w:rFonts w:ascii="Liberation Serif" w:hAnsi="Liberation Serif"/><w:sz w:val="24"/></w:rPr><w:t>TOKEN</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1800" w:right="1800"/></w:sectPr></w:body></w:document>)xml"_ostr;
            writeWordMacPart(xWord, u"document.xml"_ustr, aDocument.replaceAll("TOKEN"_ostr,
                "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM"_ostr));
            const OString aSettings = R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:compat><w:growAutofit w:val=")xml"_ostr
                + (bGrow ? "1"_ostr : "0"_ostr)
                + R"xml("/><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val="15"/></w:compat></w:settings>)xml"_ostr;
            writeWordMacPart(xWord, u"settings.xml"_ustr, aSettings);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        loadFromURL(aFile.GetURL());
        for (int nRound = 0; nRound < 3; ++nRound)
        {
            Scheduler::ProcessEventsToIdle();
            calcLayout();
            if (nRound > 0)
            {
                assertXPath(parseExport(u"word/settings.xml"_ustr),
                    "/w:settings/w:compat/w:growAutofit", "val", bGrow ? u"1"_ustr : u"0"_ustr);
                assertXPath(parseExport(u"word/document.xml"_ustr),
                    "/w:document/w:body/w:tbl/w:tblPr/w:tblLayout", "type", u"autofit"_ustr);
            }
            auto pLayout = parseLayoutDump();
            const auto nBody = getXPath(pLayout, "/root/page[1]/body/infos/prtBounds", "width").toInt32();
            const auto nTable = getXPath(pLayout, "/root/page[1]/body/tab/infos/prtBounds", "width").toInt32();
            const auto* pTable = SwTable::FindTable(
                (*getSwDocShell()->GetDoc()->GetTableFrameFormats())[0]);
            CPPUNIT_ASSERT(pTable);
            const auto* pAutoFit = pTable->GetHTMLTableLayout();
            if (nRound == 0)
                CPPUNIT_ASSERT_MESSAGE("AutoFit layout must be attached", pAutoFit);
            const OString aDiagnostic = "grow="_ostr + (bGrow ? "1"_ostr : "0"_ostr)
                + " round="_ostr + OString::number(nRound)
                + " body="_ostr + OString::number(nBody)
                + " table="_ostr + OString::number(nTable)
                + " minimum="_ostr + OString::number(pAutoFit ? pAutoFit->GetMin() : 0);
            if (nRound == 0)
                CPPUNIT_ASSERT_MESSAGE(aDiagnostic.getStr(), pAutoFit->GetMin() > sal_uLong(nBody));
            if (bGrow)
                CPPUNIT_ASSERT_MESSAGE(aDiagnostic.getStr(), nTable > nBody);
            else
            {
                CPPUNIT_ASSERT_MESSAGE(aDiagnostic.getStr(), std::abs(nTable - nBody) <= 1);
                CPPUNIT_ASSERT_GREATER(1, countXPathNodes(pLayout,
                    "/root/page[1]/body/tab/row/cell[1]/txt/SwParaPortion/SwLineLayout"));
            }
            if (nRound < 2)
                saveAndReload(TestFilter::DOCX);
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacAutoFitAsciiToken)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    const OUString aFitName = u"COCOLIAN_WORD_MAC_AUTOFIT"_ustr;
    OUString aPreviousFit;
    const bool bHadFit = osl_getEnvironment(aFitName.pData, &aPreviousFit.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestoreFit([&] {
        if (bHadFit)
            osl_setEnvironment(aFitName.pData, aPreviousFit.pData);
        else
            osl_clearEnvironment(aFitName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="5000" w:type="pct"/></w:tblPr><w:tblGrid><w:gridCol w:w="900"/><w:gridCol w:w="8100"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="500" w:type="pct"/><w:noWrap w:val="0"/></w:tcPr><w:p><w:r><w:rPr><w:rFonts w:ascii="Liberation Serif" w:hAnsi="Liberation Serif"/><w:sz w:val="24"/></w:rPr><w:t>&quot;application/json&quot;</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:tcW w:w="4500" w:type="pct"/><w:noWrap w:val="0"/></w:tcPr><w:p><w:r><w:rPr><w:rFonts w:ascii="Liberation Serif" w:hAnsi="Liberation Serif"/><w:sz w:val="24"/></w:rPr><w:t>May wrap</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr/></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    const OUString aOff = u"0"_ustr;
    osl_setEnvironment(aFitName.pData, aOff.pData);
    loadFromURL(aFile.GetURL());
    auto pFixed = parseLayoutDump();
    const auto nFixed = getXPath(pFixed, "/root/page[1]/body/tab/row/cell[1]/infos/bounds", "width").toInt32();
    osl_setEnvironment(aFitName.pData, aOn.pData);
    loadFromURL(aFile.GetURL());
    Scheduler::ProcessEventsToIdle();
    auto pFit = parseLayoutDump();
    const auto nFit = getXPath(pFit, "/root/page[1]/body/tab/row/cell[1]/infos/bounds", "width").toInt32();
    // Cached grid widths cannot split a noWrap cell merely to preserve its
    // preferred percentage. AutoFit must expand it to its content minimum.
    CPPUNIT_ASSERT_GREATER(nFixed + 200, nFit);
    CPPUNIT_ASSERT_EQUAL(u"\"application/json\""_ustr,
        getXPath(pFit, "/root/page[1]/body/tab/row/cell[1]/txt/SwParaPortion/SwLineLayout[1]", "portion"));
    CPPUNIT_ASSERT_EQUAL(1, getPages());
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacAutoFitLongUrl)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    const OUString aFitName = u"COCOLIAN_WORD_MAC_AUTOFIT"_ustr;
    OUString aPreviousFit;
    const bool bHadFit = osl_getEnvironment(aFitName.pData, &aPreviousFit.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestoreFit([&] {
        if (bHadFit)
            osl_setEnvironment(aFitName.pData, aPreviousFit.pData);
        else
            osl_clearEnvironment(aFitName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="5000" w:type="pct"/></w:tblPr><w:tblGrid><w:gridCol w:w="900"/><w:gridCol w:w="8100"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="500" w:type="pct"/><w:noWrap w:val="0"/></w:tcPr><w:p><w:r><w:rPr><w:rFonts w:ascii="Liberation Serif" w:hAnsi="Liberation Serif"/><w:sz w:val="24"/></w:rPr><w:t>https://example.test/alpha/beta/gamma/delta/epsilon/zeta/eta/theta/iota/kappa/lambda/mu/nu/xi/omicron/pi/rho/sigma/tau/upsilon/phi/chi/psi/omega</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:tcW w:w="4500" w:type="pct"/><w:noWrap w:val="0"/></w:tcPr><w:p><w:r><w:rPr><w:rFonts w:ascii="Liberation Serif" w:hAnsi="Liberation Serif"/><w:sz w:val="24"/></w:rPr><w:t>May wrap</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr/></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    const OUString aOff = u"0"_ustr;
    osl_setEnvironment(aFitName.pData, aOff.pData);
    loadFromURL(aFile.GetURL());
    auto pFixed = parseLayoutDump();
    const auto nFixed = getXPath(pFixed, "/root/page[1]/body/tab/row/cell[1]/infos/bounds", "width").toInt32();
    osl_setEnvironment(aFitName.pData, aOn.pData);
    loadFromURL(aFile.GetURL());
    Scheduler::ProcessEventsToIdle();
    auto pFit = parseLayoutDump();
    const auto nFit = getXPath(pFit, "/root/page[1]/body/tab/row/cell[1]/infos/bounds", "width").toInt32();
    // Cached grid widths cannot split a noWrap cell merely to preserve its
    // preferred percentage. AutoFit must expand it to its content minimum.
    CPPUNIT_ASSERT(nFit >= nFixed);
    CPPUNIT_ASSERT_GREATER(sal_Int32(1), getXPathContent(pFit,
        "count(/root/page[1]/body/tab/row/cell[1]/txt/SwParaPortion/SwLineLayout)").toInt32());
    const auto nTableWidth = getXPath(pFit, "/root/page[1]/body/tab/infos/bounds", "width").toInt32();
    const auto nBodyWidth = getXPath(pFit, "/root/page[1]/body/infos/bounds", "width").toInt32();
    CPPUNIT_ASSERT(nTableWidth <= nBodyWidth + 1);
    CPPUNIT_ASSERT_EQUAL(1, getPages());
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacAutoFitDefaultFont)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    const OUString aFitName = u"COCOLIAN_WORD_MAC_AUTOFIT"_ustr;
    OUString aPreviousFit;
    const bool bHadFit = osl_getEnvironment(aFitName.pData, &aPreviousFit.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestoreFit([&] {
        if (bHadFit)
            osl_setEnvironment(aFitName.pData, aPreviousFit.pData);
        else
            osl_clearEnvironment(aFitName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="5000" w:type="pct"/></w:tblPr><w:tblGrid><w:gridCol w:w="900"/><w:gridCol w:w="8100"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="500" w:type="pct"/><w:noWrap/></w:tcPr><w:p><w:r><w:t>Unbreakable-parameter-name</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:tcW w:w="4500" w:type="pct"/><w:noWrap w:val="0"/></w:tcPr><w:p><w:r><w:t>May wrap</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr/></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    const OUString aOff = u"0"_ustr;
    osl_setEnvironment(aFitName.pData, aOff.pData);
    loadFromURL(aFile.GetURL());
    auto pFixed = parseLayoutDump();
    const auto nFixed = getXPath(pFixed, "/root/page[1]/body/tab/row/cell[1]/infos/bounds", "width").toInt32();
    osl_setEnvironment(aFitName.pData, aOn.pData);
    loadFromURL(aFile.GetURL());
    Scheduler::ProcessEventsToIdle();
    auto pFit = parseLayoutDump();
    const auto nFit = getXPath(pFit, "/root/page[1]/body/tab/row/cell[1]/infos/bounds", "width").toInt32();
    // Cached grid widths cannot split a noWrap cell merely to preserve its
    // preferred percentage. AutoFit must expand it to its content minimum.
    CPPUNIT_ASSERT_GREATER(nFixed + 200, nFit);
    CPPUNIT_ASSERT_EQUAL(u"Unbreakable-parameter-name"_ustr,
        getXPath(pFit, "/root/page[1]/body/tab/row/cell[1]/txt/SwParaPortion/SwLineLayout[1]", "portion"));
    CPPUNIT_ASSERT_EQUAL(1, getPages());
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacScriptSpacingSettings)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:autoSpaceDE w:val="0"/><w:autoSpaceDN w:val="0"/></w:pPr><w:r><w:t>Spacing disabled</w:t></w:r></w:p><w:p><w:pPr><w:autoSpaceDE w:val="1"/><w:autoSpaceDN w:val="1"/></w:pPr><w:r><w:t>Spacing enabled</w:t></w:r></w:p><w:sectPr/></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    loadFromURL(aFile.GetURL());
    CPPUNIT_ASSERT(!getProperty<bool>(getParagraph(1), u"ParaIsCharacterDistance"_ustr));
    CPPUNIT_ASSERT(getProperty<bool>(getParagraph(2), u"ParaIsCharacterDistance"_ustr));
    saveAndReload(TestFilter::DOCX);
    CPPUNIT_ASSERT(!getProperty<bool>(getParagraph(1), u"ParaIsCharacterDistance"_ustr));
    CPPUNIT_ASSERT(getProperty<bool>(getParagraph(2), u"ParaIsCharacterDistance"_ustr));
    auto pXml = parseExport(u"word/document.xml"_ustr);
    assertXPath(pXml, "/w:document/w:body/w:p[1]/w:pPr/w:autoSpaceDE", "val", u"false");
    assertXPath(pXml, "/w:document/w:body/w:p[1]/w:pPr/w:autoSpaceDN", "val", u"false");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacFixedGridBaseline)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, true, true);
    loadFromURL(aFile.GetURL());
    uno::Reference<beans::XPropertySet> xParagraph(getParagraph(1), uno::UNO_QUERY_THROW);
    style::LineSpacing aSpacing;
    aSpacing.Mode = style::LineSpacingMode::FIX;
    aSpacing.Height = 988; // 28pt = 560 twips.
    xParagraph->setPropertyValue(u"ParaLineSpacing"_ustr, uno::Any(aSpacing));
    auto pLayout = parseLayoutDump();
    const sal_Int32 nPageTop = getXPath(pLayout, "/root/page[1]/infos/bounds", "top").toInt32();
    auto xMetaFile = getSwDocShell()->GetPreviewMetaFile();
    MetafileXmlDump aDumper;
    auto pXml = dumpAndParse(aDumper, *xMetaFile);
    // 72pt top margin + 80% of the fixed 28pt line height.
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1888),
        getXPath(pXml, "(//textarray)[1]", "y").toInt32() - nPageTop);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacCachedGridBottom)
{
    const std::array<OUString, 2> aNames = {
        u"COCOLIAN_WORD_MAC_METRICS"_ustr, u"COCOLIAN_WORD_MAC_GRID_BOTTOM"_ustr };
    std::array<OUString, 2> aPrevious;
    std::array<bool, 2> aHadPrevious;
    for (size_t i = 0; i < aNames.size(); ++i)
        aHadPrevious[i] = osl_getEnvironment(aNames[i].pData, &aPrevious[i].pData)
            == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
        {
            if (aHadPrevious[i])
                osl_setEnvironment(aNames[i].pData, aPrevious[i].pData);
            else
                osl_clearEnvironment(aNames[i].pData);
        }
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aNames[0].pData, aOn.pData);
    for (bool bEnabled : { false, true })
    {
        const OUString aValue = bEnabled ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aNames[1].pData, aValue.pData);
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            writeWordMacPart(xWord, u"document.xml"_ustr,
                R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="3100" w:lineRule="exact"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:t>Spacer</w:t></w:r></w:p><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="auto"/><w:snapToGrid/><w:widowControl w:val="0"/><w:rPr><w:rFonts w:ascii="Menlo" w:hAnsi="Menlo" w:eastAsia="SimSun"/><w:sz w:val="20"/></w:rPr></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Menlo" w:hAnsi="Menlo" w:eastAsia="SimSun"/><w:sz w:val="20"/></w:rPr><w:t>{</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="4730"/><w:pgMar w:top="720" w:bottom="720" w:left="1440" w:right="1440"/><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:body></w:document>)xml"_ostr);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        loadFromURL(aFile.GetURL());
        CPPUNIT_ASSERT_EQUAL(2, getPages());
        auto* pShell = getSwDocShell()->GetWrtShell();
        pShell->SttEndDoc(false);
        auto* pFrame = const_cast<SwTextFrame*>(
            static_cast<const SwTextFrame*>(pShell->GetCurrFrame()));
        SwTwips nRemaining = 290;
        bool bSplit = false;
        const bool bFits = pFrame->WouldFit(nRemaining, bSplit, false, true);
        CPPUNIT_ASSERT_EQUAL(bEnabled, bFits);
        if (bFits)
        {
            CPPUNIT_ASSERT(!bSplit);
            CPPUNIT_ASSERT_GREATEREQUAL(SwTwips(0), nRemaining);
        }
        uno::Reference<beans::XPropertySet> xParagraph(getParagraph(1), uno::UNO_QUERY_THROW);
        style::LineSpacing aSpacing;
        aSpacing.Mode = style::LineSpacingMode::FIX;
        aSpacing.Height = 5292; // 3000 twips: 290 twips remain for the cached 312-twip line.
        xParagraph->setPropertyValue(u"ParaLineSpacing"_ustr, uno::Any(aSpacing));
        calcLayout();
        CPPUNIT_ASSERT_EQUAL(bEnabled ? 1 : 2, getPages());
        // A line whose visible font box does not fit must still move forward.
        aSpacing.Height = 5556; // 3150 twips: only 140 twips remain.
        xParagraph->setPropertyValue(u"ParaLineSpacing"_ustr, uno::Any(aSpacing));
        calcLayout();
        CPPUNIT_ASSERT_EQUAL(2, getPages());
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacGridBottomPadding)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    const OUString aGridName = u"COCOLIAN_WORD_MAC_GRID_BOTTOM"_ustr;
    OUString aPrevious, aPreviousGrid;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    const bool bHadGrid = osl_getEnvironment(aGridName.pData, &aPreviousGrid.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
        if (bHadGrid)
            osl_setEnvironment(aGridName.pData, aPreviousGrid.pData);
        else
            osl_clearEnvironment(aGridName.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="12800" w:lineRule="exact"/></w:pPr><w:r><w:t>Spacer</w:t></w:r></w:p><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="auto"/><w:snapToGrid/><w:widowControl/><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="32"/></w:rPr></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="32"/></w:rPr><w:t>Grid bottom 甲</w:t><w:br/><w:t>Grid bottom 乙</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    const OUString aOff = u"0"_ustr;
    osl_setEnvironment(aGridName.pData, aOff.pData);
    loadFromURL(aFile.GetURL());
    CPPUNIT_ASSERT_EQUAL(2, getPages());
    osl_setEnvironment(aGridName.pData, aOn.pData);
    loadFromURL(aFile.GetURL());
    // Actual Word keeps both final grid lines on the first page; only the
    // unused lower half of the last grid line reaches beyond the body.
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // A further 5pt must still force a page break: fitting must never
    // subtract the unused grid padding twice.
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="12900" w:lineRule="exact"/></w:pPr><w:r><w:t>Spacer</w:t></w:r></w:p><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="auto"/><w:snapToGrid/><w:widowControl/><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="32"/></w:rPr></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="32"/></w:rPr><w:t>Grid bottom 甲</w:t><w:br/><w:t>Grid bottom 乙</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    loadFromURL(aFile.GetURL());
    CPPUNIT_ASSERT_EQUAL(2, getPages());
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="12800" w:lineRule="exact"/></w:pPr><w:r><w:t>Spacer</w:t></w:r></w:p><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="auto"/><w:snapToGrid/><w:widowControl w:val="0"/><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="32"/></w:rPr></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="32"/></w:rPr><w:t>Grid bottom 甲</w:t><w:br/><w:t>Grid bottom 乙</w:t><w:br/><w:t>Grid bottom 丙</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    for (bool bEnabled : {false, true})
    {
        const OUString aValue = bEnabled ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aGridName.pData, aValue.pData);
        loadFromURL(aFile.GetURL());
        CPPUNIT_ASSERT_EQUAL(2, getPages());
        // Word also uses the available ink space when the paragraph itself
        // continues on the next page; this is not limited to its final line.
        assertXPath(parseLayoutDump(), "/root/page[1]/body/txt[2]/SwParaPortion/SwLineLayout",
            bEnabled ? 2 : 1);
        if (bEnabled)
        {
            auto* pShell = getSwDocShell()->GetWrtShell();
            pShell->SttEndDoc(true);
            pShell->Right(SwCursorSkipMode::Chars, false,
                u"Spacer\nGrid bottom "_ustr.getLength(), false);
            const auto nFirst = pShell->GetCharRect().Top();
            pShell->Right(SwCursorSkipMode::Chars, false,
                u"甲\nGrid bottom "_ustr.getLength(), false);
            const auto nSecond = pShell->GetCharRect().Top();
            CPPUNIT_ASSERT_EQUAL(SwTwips(624), nSecond - nFirst);
            save(TestFilter::PDF_WRITER);
            auto pPdf = parsePDFExport();
            if (pPdf)
            {
                auto pPage = pPdf->openPage(0);
                auto pText = pPage->getTextPage();
                double fFirstTop = -1;
                double fLastTop = -1;
                for (int nChar = 0; nChar < pText->countChars(); ++nChar)
                {
                    if (pText->getUnicode(nChar) != 'G')
                        continue;
                    const auto aBox = pText->getCharBox(nChar, pPage->getHeight() * 2540 / 72);
                    if (fFirstTop < 0)
                        fFirstTop = aBox.getMinY();
                    fLastTop = aBox.getMinY();
                }
                CPPUNIT_ASSERT(fFirstTop >= 0 && fLastTop > fFirstTop);
                CPPUNIT_ASSERT_DOUBLES_EQUAL(31.2, (fLastTop - fFirstTop) * 72 / 2540, 0.05);
            }
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacClassicBoldAdvance)
{
    const OUString aProfile = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    const OUString aClassic = u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr;
    OUString aPrevious, aPreviousClassic;
    const bool bHadProfile = osl_getEnvironment(aProfile.pData, &aPrevious.pData) == osl_Process_E_None;
    const bool bHadClassic = osl_getEnvironment(aClassic.pData, &aPreviousClassic.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadProfile)
            osl_setEnvironment(aProfile.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aProfile.pData);
        if (bHadClassic)
            osl_setEnvironment(aClassic.pData, aPreviousClassic.pData);
        else
            osl_clearEnvironment(aClassic.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aProfile.pData, aOn.pData);
    osl_setEnvironment(aClassic.pData, aOn.pData);
    for (const char* pFamily : {"SimSun", "SimHei", "FangSong", "KaiTi"})
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            OString aXml = R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="560" w:lineRule="exact"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FONT" w:hAnsi="FONT" w:eastAsia="FONT"/><w:sz w:val="24"/><w:b/></w:rPr><w:t>甲乙丙丁</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr;
            aXml = aXml.replaceAll("FONT"_ostr, OString(pFamily));
            writeWordMacPart(xWord, u"document.xml"_ustr, aXml);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        loadFromURL(aFile.GetURL());
        parseLayoutDump();
        auto* pShell = getSwDocShell()->GetWrtShell();
        pShell->SttEndDoc(true);
        const SwTwips nFirst = pShell->GetCharRect().Left();
        pShell->Right(SwCursorSkipMode::Chars, false, 3, false);
        // Actual Word 12pt synthetic bold advances 12.234pt per CJK glyph
        // in all four classic families, matching the independent GB2312 matrix.
        CPPUNIT_ASSERT_MESSAGE(pFamily,
            std::abs(pShell->GetCharRect().Left() - nFirst - 734) <= 2);
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacResolvedFontGrid)
{
    const OUString aProfile = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    const OUString aClassic = u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr;
    OUString aPrevious, aPreviousClassic;
    const bool bHadProfile = osl_getEnvironment(aProfile.pData, &aPrevious.pData) == osl_Process_E_None;
    const bool bHadClassic = osl_getEnvironment(aClassic.pData, &aPreviousClassic.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadProfile)
            osl_setEnvironment(aProfile.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aProfile.pData);
        if (bHadClassic)
            osl_setEnvironment(aClassic.pData, aPreviousClassic.pData);
        else
            osl_clearEnvironment(aClassic.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aProfile.pData, aOn.pData);
    osl_setEnvironment(aClassic.pData, aOn.pData);
    sal_Int32 nActualBaseline = 0;
    for (bool bMissing : {false, true})
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            OString aXml = R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="360" w:lineRule="auto"/><w:snapToGrid/><w:rPr><w:rFonts w:ascii="FONT" w:hAnsi="FONT" w:eastAsia="FONT"/><w:sz w:val="44"/></w:rPr></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FONT" w:hAnsi="FONT" w:eastAsia="FONT"/><w:sz w:val="44"/></w:rPr><w:t>标题测试</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:body></w:document>)xml"_ostr;
            aXml = aXml.replaceAll("FONT"_ostr,
                bMissing ? OUStringToOString(u"方正小标宋_GBK"_ustr, RTL_TEXTENCODING_UTF8)
                         : "Microsoft YaHei"_ostr);
            writeWordMacPart(xWord, u"document.xml"_ustr, aXml);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        loadFromURL(aFile.GetURL());
        // The private native fontconfig matches Word's actual fallback to
        // YaHei. Both requests require three 15.6pt rows at 22pt, not two.
        assertXPath(parseLayoutDump(), "/root/page/body/txt/SwParaPortion/SwLineLayout",
            "height", u"936");
        auto xMeta = getSwDocShell()->GetPreviewMetaFile();
        MetafileXmlDump aDump;
        auto pMeta = dumpAndParse(aDump, *xMeta);
        const auto nBaseline = getXPath(pMeta, "(//textarray)[1]", "y").toInt32();
        if (bMissing)
            CPPUNIT_ASSERT(std::abs(nBaseline - nActualBaseline) <= 1);
        else
            nActualBaseline = nBaseline;
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacCjkBaseline)
{
    const OUString aProfile = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    const OUString aClassic = u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr;
    OUString aPrevious, aPreviousClassic;
    const bool bHadProfile = osl_getEnvironment(aProfile.pData, &aPrevious.pData) == osl_Process_E_None;
    const bool bHadClassic = osl_getEnvironment(aClassic.pData, &aPreviousClassic.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadProfile)
            osl_setEnvironment(aProfile.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aProfile.pData);
        if (bHadClassic)
            osl_setEnvironment(aClassic.pData, aPreviousClassic.pData);
        else
            osl_clearEnvironment(aClassic.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aProfile.pData, aOn.pData);
    osl_setEnvironment(aClassic.pData, aOn.pData);
    struct FontCase { const char* pFamily; int nSize; bool bBold; int nAscent; int nPitch; };
    // Actual Word PDF baseline/pitch measurements in twips, relative to a
    // 72pt top margin. The six fonts have independently pinned native files.
    for (const auto& rCase : {FontCase{"SimSun", 24, false, 245, 312},
        FontCase{"SimHei", 44, true, 441, 571}, FontCase{"FangSong", 36, false, 360, 468},
        FontCase{"Microsoft YaHei", 24, false, 302, 410},
        FontCase{"DengXian", 24, false, 235, 324},
        FontCase{"FangSong_GB2312", 24, false, 245, 312}})
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            OString aXml = R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="auto"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FONT" w:hAnsi="FONT" w:eastAsia="FONT"/><w:sz w:val="SIZE"/>BOLD</w:rPr><w:t>甲乙丙 ABC123</w:t><w:br/><w:t>甲乙丙 ABC123</w:t><w:br/><w:t>甲乙丙 ABC123</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr;
            aXml = aXml.replaceAll("FONT"_ostr, OString(rCase.pFamily))
                .replaceAll("SIZE"_ostr, OString::number(rCase.nSize))
                .replaceAll("BOLD"_ostr, rCase.bBold ? "<w:b/>"_ostr : OString());
            writeWordMacPart(xWord, u"document.xml"_ustr, aXml);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        loadFromURL(aFile.GetURL());
        auto pLayout = parseLayoutDump();
        const auto nTop = getXPath(pLayout, "/root/page[1]/infos/bounds", "top").toInt32();
        auto xMetaFile = getSwDocShell()->GetPreviewMetaFile();
        MetafileXmlDump aDumper;
        auto pMeta = dumpAndParse(aDumper, *xMetaFile);
        const auto nFirst = getXPath(pMeta, "(//textarray)[1]", "y").toInt32();
        const auto nLast = getXPath(pMeta, "(//textarray)[last()]", "y").toInt32();
        CPPUNIT_ASSERT_MESSAGE(rCase.pFamily,
            std::abs(nFirst - nTop - 1440 - rCase.nAscent) <= 8);
        CPPUNIT_ASSERT_MESSAGE(rCase.pFamily,
            std::abs(nLast - nFirst - 2 * rCase.nPitch) <= 8);
    }
    // The ordinary-font correction must preserve Word's SimSun grid
    // boundary: 12pt fits one row; 14pt includes leading and needs two.
    for (int nSize : {24, 28})
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            OString aXml = R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="auto"/><w:snapToGrid/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="SimSun" w:hAnsi="SimSun" w:eastAsia="SimSun"/><w:sz w:val="SIZE"/></w:rPr><w:t>甲乙</w:t><w:br/><w:t>丙丁</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:body></w:document>)xml"_ostr;
            aXml = aXml.replaceAll("SIZE"_ostr, OString::number(nSize));
            writeWordMacPart(xWord, u"document.xml"_ustr, aXml);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        loadFromURL(aFile.GetURL());
        assertXPath(parseLayoutDump(), "/root/page/body/txt/infos/bounds", "height",
            nSize == 24 ? u"624"_ustr : u"1248"_ustr);
    }

}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacDengXianPitch)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="288" w:lineRule="auto"/><w:snapToGrid w:val="0"/><w:rPr><w:rFonts w:ascii="DengXian" w:eastAsia="DengXian"/><w:sz w:val="24"/></w:rPr></w:pPr><w:r><w:rPr><w:rFonts w:ascii="DengXian" w:hAnsi="DengXian" w:eastAsia="DengXian"/><w:sz w:val="24"/></w:rPr><w:t>甲</w:t><w:br/><w:t>乙</w:t></w:r></w:p><w:sectPr/></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    loadFromURL(aFile.GetURL());
    // Actual Word: 16.32pt single pitch (12pt at 100%); the intrinsic
    // line box is 16.25pt. Proportional spacing is applied separately.
    const auto nHeight = getXPath(parseLayoutDump(),
        "/root/page/body/txt/SwParaPortion/SwLineLayout[2]", "height").toInt32();
    CPPUNIT_ASSERT_GREATEREQUAL(sal_Int32(321), nHeight);
    CPPUNIT_ASSERT_LESSEQUAL(sal_Int32(329), nHeight);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacMissingThemeFonts)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true, false, false, false, true);
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    loadFromURL(aFile.GetURL());
    CPPUNIT_ASSERT_EQUAL(u"Aptos"_ustr,
        getProperty<OUString>(getRun(getParagraph(1), 1), u"CharFontName"_ustr));
    CPPUNIT_ASSERT_EQUAL(u"DengXian"_ustr,
        getProperty<OUString>(getRun(getParagraph(1), 1), u"CharFontNameAsian"_ustr));
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacEmptyParagraphStyle)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true, false, false, false, false, true);
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    loadFromURL(aFile.GetURL());
    CPPUNIT_ASSERT_EQUAL(sal_Int32(423),
        getProperty<sal_Int32>(getParagraph(1), u"ParaTopMargin"_ustr));
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacPunctuationCompression)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true, false, false, false, false, false, true);
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    loadFromURL(aFile.GetURL());
    CPPUNIT_ASSERT_EQUAL(sal_Int16(1), static_cast<sal_Int16>(
        getSwDocShell()->GetDoc()->getIDocumentSettingAccess().getCharacterCompressionType()));
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacFractionalTableWidth)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    for (sal_Int32 nFiftieths : { 4934, 4998 })
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            writeWordMacPart(xWord, u"document.xml"_ustr,
                R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="4998" w:type="pct"/><w:tblLayout w:type="fixed"/></w:tblPr><w:tblGrid><w:gridCol w:w="4500"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="4500" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>Width</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr/></w:body></w:document>)xml"_ostr.replaceAll("4998"_ostr, OString::number(nFiftieths)));
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        for (bool bMac : { false, true })
        {
            const OUString aValue = bMac ? u"1"_ustr : u"0"_ustr;
            osl_setEnvironment(aName.pData, aValue.pData);
            loadFromURL(aFile.GetURL());
            const sal_Int16 nWholePercent = (nFiftieths + (bMac ? 25 : 0)) / 50;
            for (int nRound = 0; nRound < 2; ++nRound)
            {
                calcLayout();
                uno::Reference<text::XTextTablesSupplier> xSupplier(mxComponent, uno::UNO_QUERY_THROW);
                uno::Reference<container::XIndexAccess> xTables(xSupplier->getTextTables(), uno::UNO_QUERY_THROW);
                uno::Reference<text::XTextTable> xTable(xTables->getByIndex(0), uno::UNO_QUERY_THROW);
                CPPUNIT_ASSERT_EQUAL(nWholePercent,
                    getProperty<sal_Int16>(xTable, u"RelativeWidth"_ustr));
                auto pDump = parseLayoutDump();
                const sal_Int32 nBody = getXPath(pDump,
                    "/root/page/body/infos/prtBounds", "width").toInt32();
                const sal_Int32 nWidth = getXPath(pDump,
                    "/root/page/body/tab/infos/prtBounds", "width").toInt32();
                const sal_Int32 nExpected = bMac ? sal_Int64(nBody) * nFiftieths / 5000
                    : sal_Int64(nBody) * nWholePercent / 100;
                const OString aDiagnostic = "fiftieths="_ostr + OString::number(nFiftieths)
                    + " mac="_ostr + OString::number(bMac)
                    + " round="_ostr + OString::number(nRound)
                    + " width="_ostr + OString::number(nWidth)
                    + " expected="_ostr + OString::number(nExpected);
                CPPUNIT_ASSERT_MESSAGE(aDiagnostic.getStr(), std::abs(nWidth - nExpected) <= 1);
                if (!nRound)
                    saveAndReload(TestFilter::DOCX);
                else
                {
                    assertXPath(parseExport(u"word/document.xml"_ustr),
                        "/w:document/w:body/w:tbl/w:tblPr/w:tblW", "w",
                        OUString::number(bMac ? nFiftieths : nWholePercent * 50));
                    if (bMac && nFiftieths == 4934)
                    {
                        // An explicit edit to the rounded UNO value means
                        // 99%, even when the UI previously displayed 99%.
                        uno::Reference<beans::XPropertySet>(xTable, uno::UNO_QUERY_THROW)
                            ->setPropertyValue(u"RelativeWidth"_ustr, uno::Any(sal_Int16(99)));
                        saveAndReload(TestFilter::DOCX);
                        assertXPath(parseExport(u"word/document.xml"_ustr),
                            "/w:document/w:body/w:tbl/w:tblPr/w:tblW", "w", u"4950");
                    }
                }
            }
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacMissingNumberingLevelIndex)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        writeWordMacPart(xRoot, u"[Content_Types].xml"_ustr,
            R"xml(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="xml" ContentType="application/xml"/><Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/><Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/><Override PartName="/word/settings.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"/><Override PartName="/word/numbering.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"/></Types>)xml"_ostr);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        auto xRels = xWord->openStorageElement(u"_rels"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xRels, u"document.xml.rels"_ustr,
            R"xml(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/><Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings" Target="settings.xml"/><Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering" Target="numbering.xml"/></Relationships>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xRels, uno::UNO_QUERY_THROW)->commit();
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:numPr><w:ilvl w:val="0"/><w:numId w:val="1"/></w:numPr><w:ind w:left="425" w:leftChars="0" w:hanging="425" w:firstLineChars="0"/></w:pPr><w:r><w:t>Alpha</w:t></w:r></w:p><w:p><w:pPr><w:numPr><w:ilvl w:val="0"/><w:numId w:val="2"/></w:numPr></w:pPr><w:r><w:t>Beta</w:t></w:r></w:p><w:sectPr/></w:body></w:document>)xml"_ostr);
        writeWordMacPart(xWord, u"numbering.xml"_ustr,
            R"xml(<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:abstractNum w:abstractNumId="1"><w:lvl><w:start w:val="7"/><w:numFmt w:val="decimal"/><w:lvlText w:val="%1."/><w:pPr><w:ind w:left="425" w:hanging="425"/></w:pPr></w:lvl></w:abstractNum><w:abstractNum w:abstractNumId="2"><w:lvl><w:start w:val="0"/><w:numFmt w:val="bullet"/><w:lvlText w:val="*"/><w:rPr><w:rFonts w:ascii="Liberation Serif" w:hAnsi="Liberation Serif"/></w:rPr></w:lvl></w:abstractNum><w:num w:numId="1"><w:abstractNumId w:val="1"/></w:num><w:num w:numId="2"><w:abstractNumId w:val="2"/></w:num></w:numbering>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    loadFromURL(aFile.GetURL());
    // Zero character indentation must not erase the matching list indent.
    CPPUNIT_ASSERT_EQUAL(sal_Int32(750),
        getProperty<sal_Int32>(getParagraph(1), u"ParaLeftMargin"_ustr));
    for (int nPara : {1, 2})
    {
        auto xRules = getProperty<uno::Reference<container::XIndexAccess>>(
            getParagraph(nPara), u"NumberingRules"_ustr);
        uno::Sequence<beans::PropertyValue> aProps;
        xRules->getByIndex(0) >>= aProps;
        bool bChecked = false;
        for (const auto& rProp : aProps)
        {
            if (nPara == 1 && rProp.Name == u"StartWith")
            {
                CPPUNIT_ASSERT_EQUAL(sal_Int16(7), rProp.Value.get<sal_Int16>());
                bChecked = true;
            }
            if (nPara == 2 && rProp.Name == u"NumberingType")
            {
                CPPUNIT_ASSERT_EQUAL(style::NumberingType::CHAR_SPECIAL, rProp.Value.get<sal_Int16>());
                bChecked = true;
            }
        }
        CPPUNIT_ASSERT(bChecked);
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacInlineImageGridLeading)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        writeWordMacPart(xRoot, u"[Content_Types].xml"_ustr,
            R"xml(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="xml" ContentType="application/xml"/><Default Extension="png" ContentType="image/png"/><Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/><Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/><Override PartName="/word/settings.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"/></Types>)xml"_ostr);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        auto xRels = xWord->openStorageElement(u"_rels"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xRels, u"document.xml.rels"_ustr,
            R"xml(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/><Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings" Target="settings.xml"/><Relationship Id="image" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" Target="media/probe.png"/></Relationships>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xRels, uno::UNO_QUERY_THROW)->commit();
        auto xMedia = xWord->openStorageElement(u"media"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xMedia, u"probe.png"_ustr,
            OString("\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52\x00\x00\x00\x01\x00\x00\x00\x01\x08\x02\x00\x00\x00\x90\x77\x53\xde\x00\x00\x00\x0c\x49\x44\x41\x54\x78\x9c\x63\x60\x60\x60\x00\x00\x00\x04\x00\x01\xf6\x17\x38\x55\x00\x00\x00\x00\x49\x45\x4e\x44\xae\x42\x60\x82", 69));
        uno::Reference<embed::XTransactedObject>(xMedia, uno::UNO_QUERY_THROW)->commit();
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main" xmlns:wp="http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing" xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:pic="http://schemas.openxmlformats.org/drawingml/2006/picture" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"><w:body><w:p><w:pPr><w:spacing w:line="240" w:lineRule="auto"/><w:snapToGrid/><w:rPr><w:sz w:val="28"/></w:rPr></w:pPr><w:r><w:drawing><wp:inline><wp:extent cx="1270000" cy="886460"/><wp:effectExtent l="0" t="0" r="0" b="8890"/><wp:docPr id="1" name="Synthetic1"/><a:graphic><a:graphicData uri="http://schemas.openxmlformats.org/drawingml/2006/picture"><pic:pic><pic:nvPicPr><pic:cNvPr id="1" name="probe.png"/><pic:cNvPicPr/></pic:nvPicPr><pic:blipFill><a:blip r:embed="image"/><a:stretch><a:fillRect/></a:stretch></pic:blipFill><pic:spPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="1270000" cy="886460"/></a:xfrm><a:prstGeom prst="rect"><a:avLst/></a:prstGeom></pic:spPr></pic:pic></a:graphicData></a:graphic></wp:inline></w:drawing></w:r></w:p><w:p><w:pPr><w:spacing w:line="240" w:lineRule="auto"/><w:snapToGrid/><w:rPr><w:sz w:val="32"/></w:rPr></w:pPr><w:r><w:drawing><wp:inline><wp:extent cx="1270000" cy="1375410"/><wp:effectExtent l="0" t="0" r="0" b="15240"/><wp:docPr id="2" name="Synthetic2"/><a:graphic><a:graphicData uri="http://schemas.openxmlformats.org/drawingml/2006/picture"><pic:pic><pic:nvPicPr><pic:cNvPr id="2" name="probe.png"/><pic:cNvPicPr/></pic:nvPicPr><pic:blipFill><a:blip r:embed="image"/><a:stretch><a:fillRect/></a:stretch></pic:blipFill><pic:spPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="1270000" cy="1375410"/></a:xfrm><a:prstGeom prst="rect"><a:avLst/></a:prstGeom></pic:spPr></pic:pic></a:graphicData></a:graphic></wp:inline></w:drawing></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    loadFromURL(aFile.GetURL());
    for (int nPass = 0; nPass < 3; ++nPass)
    {
        calcLayout();
        auto pXml = parseLayoutDump();
        for (sal_Int32 nImage : {1, 2})
        {
            const OString aPara = "/root/page[1]/body/txt["_ostr + OString::number(nImage) + "]"_ostr;
            const auto nTop = getXPath(pXml, aPara + "/infos/bounds", "top").toInt32();
            const auto nFlyTop = getXPath(pXml, aPara + "/anchored/fly/infos/bounds", "top").toInt32();
            const auto nHeight = getXPath(pXml, aPara + "/SwParaPortion/SwLineLayout", "height").toInt32();
            CPPUNIT_ASSERT_EQUAL(sal_Int32(nImage == 1 ? 1560 : 2496), nHeight);
            // Actual Word PDF centers the 70.5/109.5pt effect bounds within
            // 78/124.8pt grid lines, including after repeated layout passes.
            CPPUNIT_ASSERT(std::abs(nFlyTop - nTop - (nImage == 1 ? 75 : 153)) <= 2);
        }
        getSwDocShell()->GetWrtShell()->Reformat();
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacNumberingStyleIndent)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        writeWordMacPart(xRoot, u"[Content_Types].xml"_ustr,
            R"xml(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="xml" ContentType="application/xml"/><Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/><Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/><Override PartName="/word/settings.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"/><Override PartName="/word/numbering.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"/></Types>)xml"_ostr);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        auto xRels = xWord->openStorageElement(u"_rels"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xRels, u"document.xml.rels"_ustr,
            R"xml(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/><Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings" Target="settings.xml"/><Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering" Target="numbering.xml"/></Relationships>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xRels, uno::UNO_QUERY_THROW)->commit();
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:pStyle w:val="Indented"/><w:numPr><w:ilvl w:val="0"/><w:numId w:val="1"/></w:numPr></w:pPr><w:r><w:t>Alpha</w:t></w:r></w:p><w:p><w:pPr><w:pStyle w:val="Indented"/><w:numPr><w:ilvl w:val="0"/><w:numId w:val="2"/></w:numPr></w:pPr><w:r><w:t>Beta</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr);
        writeWordMacPart(xWord, u"styles.xml"_ustr,
            R"xml(<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:docDefaults><w:rPrDefault><w:rPr><w:rFonts w:ascii="Liberation Serif" w:hAnsi="Liberation Serif"/></w:rPr></w:rPrDefault><w:pPrDefault><w:pPr><w:spacing w:after="0"/></w:pPr></w:pPrDefault></w:docDefaults><w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/></w:style><w:style w:type="paragraph" w:styleId="Indented"><w:name w:val="Indented"/><w:basedOn w:val="Normal"/><w:pPr><w:ind w:firstLine="480"/></w:pPr></w:style></w:styles>)xml"_ostr);
        writeWordMacPart(xWord, u"numbering.xml"_ustr,
            R"xml(<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:abstractNum w:abstractNumId="1"><w:lvl w:ilvl="0"><w:start w:val="1"/><w:numFmt w:val="decimal"/><w:suff w:val="nothing"/><w:lvlText w:val="%1."/></w:lvl></w:abstractNum><w:abstractNum w:abstractNumId="2"><w:lvl w:ilvl="0"><w:start w:val="1"/><w:numFmt w:val="decimal"/><w:suff w:val="nothing"/><w:lvlText w:val="%1."/><w:pPr><w:ind w:left="0" w:firstLine="0"/></w:pPr></w:lvl></w:abstractNum><w:num w:numId="1"><w:abstractNumId w:val="1"/></w:num><w:num w:numId="2"><w:abstractNumId w:val="2"/></w:num></w:numbering>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    for (bool bEnabled : {false, true})
    {
        const OUString aValue = bEnabled ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aName.pData, aValue.pData);
        loadFromURL(aFile.GetURL());
        auto xMetaFile = getSwDocShell()->GetPreviewMetaFile();
        MetafileXmlDump aDumper;
        auto pMeta = dumpAndParse(aDumper, *xMetaFile);
        assertXPath(pMeta, "//textarray", 4);
        const auto nMissing = getXPath(pMeta, "(//textarray)[1]", "x").toInt32();
        const auto nExplicitZero = getXPath(pMeta, "(//textarray)[3]", "x").toInt32();
        // Word keeps the style's 24pt first-line indent only for the missing
        // attribute. An explicit zero remains zero. The legacy profile is unchanged.
        CPPUNIT_ASSERT_EQUAL(sal_Int32(bEnabled ? 480 : 0), nMissing - nExplicitZero);
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacAutomaticSuperscript)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });

    for (bool bEnabled : {false, true})
    {
        const OUString aValue = bEnabled ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aName.pData, aValue.pData);
        for (bool bLarge : {false, true})
        {
            utl::TempFileNamed aFile;
            aFile.EnableKillingFile();
            makeWordMacPackage(aFile, false, true);
            {
                auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                    ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
                auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
                OString aXml = R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:line="800" w:lineRule="exact"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FONT" w:hAnsi="FONT" w:eastAsia="FONT"/><w:sz w:val="SIZE"/></w:rPr><w:t>甲</w:t></w:r><w:r><w:rPr><w:rFonts w:ascii="FONT" w:hAnsi="FONT" w:eastAsia="FONT"/><w:sz w:val="SIZE"/><w:vertAlign w:val="superscript"/></w:rPr><w:t>1</w:t></w:r><w:r><w:rPr><w:rFonts w:ascii="FONT" w:hAnsi="FONT" w:eastAsia="FONT"/><w:sz w:val="SIZE"/></w:rPr><w:t>乙</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr;
                aXml = aXml.replaceAll("FONT"_ostr, bLarge ? "Microsoft YaHei"_ostr : "SimSun"_ostr)
                    .replaceAll("SIZE"_ostr, bLarge ? "36"_ostr : "18"_ostr);
                writeWordMacPart(xWord, u"document.xml"_ustr, aXml);
                uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
                uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
            }
            loadFromURL(aFile.GetURL());
            const sal_Int8 nExpectedProp = bEnabled ? 65 : 58;
            CPPUNIT_ASSERT_EQUAL(nExpectedProp,
                getProperty<sal_Int8>(getRun(getParagraph(1), 2), u"CharEscapementHeight"_ustr));
            // Automatic superscript must remain semantic on DOCX roundtrip,
            // never become a smaller explicit font that is shrunk a second time.
            saveAndReload(TestFilter::DOCX);
            CPPUNIT_ASSERT_EQUAL(nExpectedProp,
                getProperty<sal_Int8>(getRun(getParagraph(1), 2), u"CharEscapementHeight"_ustr));
            if (!bEnabled)
                continue;
            save(TestFilter::PDF_WRITER);
            auto pPdf = parsePDFExport();
            if (!pPdf)
                continue;
            auto pPage = pPdf->openPage(0);
            auto pText = pPage->getTextPage();
            double fBase = 0, fSuper = 0, fSize = 0;
            for (int i = 0; i < pPage->getObjectCount(); ++i)
            {
                auto pObject = pPage->getObject(i);
                const auto aText = pObject->getText(pText);
                if (aText.indexOf(u'甲') >= 0)
                    fBase = pObject->getMatrix().get(1, 2);
                if (aText == "1")
                {
                    fSuper = pObject->getMatrix().get(1, 2);
                    fSize = pObject->getFontSize();
                }
            }
            CPPUNIT_ASSERT(fBase > 0 && fSuper > fBase);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(bLarge ? 11.5 : 6., fSize, 0.02);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(bLarge ? 5.05 : 2.5, fSuper - fBase, 0.05);
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacCenteredGridHeading)
{
    const std::array<OUString, 3> aNames = {
        u"COCOLIAN_WORD_MAC_METRICS"_ustr, u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr };
    std::array<OUString, 3> aPrevious;
    std::array<bool, 3> aHadPrevious;
    for (size_t i = 0; i < aNames.size(); ++i)
    {
        aHadPrevious[i] = osl_getEnvironment(aNames[i].pData, &aPrevious[i].pData)
            == osl_Process_E_None;
        const OUString aOn = u"1"_ustr;
        osl_setEnvironment(aNames[i].pData, aOn.pData);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
        {
            if (aHadPrevious[i])
                osl_setEnvironment(aNames[i].pData, aPrevious[i].pData);
            else
                osl_clearEnvironment(aNames[i].pData);
        }
    });
    for (bool bStrict : { false, true })
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            writeWordMacPart(xWord, u"document.xml"_ustr,
                R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:jc w:val="center"/><w:snapToGrid/><w:spacing w:before="0" w:after="0" w:line="360" w:lineRule="auto"/><w:ind w:firstLine="0"/><w:rPr><w:rFonts w:ascii="SimSun" w:hAnsi="SimSun" w:eastAsia="SimSun"/><w:b/><w:sz w:val="44"/></w:rPr></w:pPr><w:r><w:rPr><w:rFonts w:ascii="SimSun" w:hAnsi="SimSun" w:eastAsia="SimSun"/><w:b/><w:sz w:val="44"/></w:rPr><w:t>甲乙丙丁戊（己庚辛）壬癸子丑寅卯辰巳午</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1800" w:right="1800"/><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:body></w:document>)xml"_ostr);
            writeWordMacPart(xWord, u"settings.xml"_ustr,
                R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:characterSpacingControl w:val="compressPunctuation"/><w:compat><w:doNotWrapTextWithPunct w:val=")xml"_ostr
                + (bStrict ? "1"_ostr : "0"_ostr)
                + R"xml("/><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val="15"/></w:compat></w:settings>)xml"_ostr);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        loadFromURL(aFile.GetURL());
        auto pDump = parseLayoutDump();
        const OString aLines = "/root/page[1]/body/txt/SwParaPortion/SwLineLayout"_ostr;
        assertXPath(pDump, aLines, bStrict ? 2 : 1);
        if (bStrict)
            assertXPath(pDump, aLines + "[2]"_ostr, "portion", u"午"_ustr);
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacGridFloatingTableTitle)
{
    const OUString aProfile = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    const OUString aClassic = u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr;
    OUString aPrevious, aPreviousClassic;
    const bool bHadProfile = osl_getEnvironment(aProfile.pData, &aPrevious.pData) == osl_Process_E_None;
    const bool bHadClassic = osl_getEnvironment(aClassic.pData, &aPreviousClassic.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadProfile)
            osl_setEnvironment(aProfile.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aProfile.pData);
        if (bHadClassic)
            osl_setEnvironment(aClassic.pData, aPreviousClassic.pData);
        else
            osl_clearEnvironment(aClassic.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aProfile.pData, aOn.pData);
    osl_setEnvironment(aClassic.pData, aOn.pData);
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblpPr w:vertAnchor="page" w:horzAnchor="margin" w:tblpY="2281" w:leftFromText="180" w:rightFromText="180"/><w:tblW w:w="8306" w:type="dxa"/><w:tblLayout w:type="fixed"/></w:tblPr><w:tblGrid><w:gridCol w:w="8306"/></w:tblGrid><w:tr><w:trPr><w:trHeight w:val="2000" w:hRule="exact"/></w:trPr><w:tc><w:tcPr><w:tcW w:w="8306" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>Table</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:pPr><w:jc w:val="center"/><w:spacing w:before="0" w:after="0" w:line="360" w:lineRule="auto"/><w:snapToGrid/><w:rPr><w:rFonts w:ascii="方正小标宋_GBK" w:hAnsi="方正小标宋_GBK" w:eastAsia="方正小标宋_GBK"/><w:sz w:val="40"/></w:rPr></w:pPr><w:r><w:rPr><w:rFonts w:ascii="方正小标宋_GBK" w:hAnsi="方正小标宋_GBK" w:eastAsia="方正小标宋_GBK"/><w:sz w:val="40"/></w:rPr><w:t>甲乙丙丁戊己庚辛壬癸子丑寅卯辰</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1800" w:right="1800"/><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    loadFromURL(aFile.GetURL());
    save(TestFilter::PDF_WRITER);
    auto pPdf = parsePDFExport();
    if (!pPdf)
        return;
    auto pPage = pPdf->openPage(0);
    auto pText = pPage->getTextPage();
    bool bFound = false;
    for (int i = 0; i < pPage->getObjectCount(); ++i)
    {
        auto pObject = pPage->getObject(i);
        if (pObject->getText(pText).indexOf(u'甲') < 0)
            continue;
        const double fBaseline = pPage->getHeight() - pObject->getMatrix().get(1, 2);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(103.44, fBaseline, 0.2);
        bFound = true;
    }
    CPPUNIT_ASSERT(bFound);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacGbBoldFractionalFit)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    for (bool bEnabled : {false, true})
    {
        const OUString aValue = bEnabled ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aName.pData, aValue.pData);
        for (sal_Int32 nWidth : {570, 571})
        {
            utl::TempFileNamed aFile;
            aFile.EnableKillingFile();
            makeWordMacPackage(aFile, false, true);
            {
                auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                    ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
                auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
                OString aXml = R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="WIDTH" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="WIDTH"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="WIDTH" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:jc w:val="left"/><w:snapToGrid w:val="0"/><w:spacing w:line="400" w:lineRule="exact"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="28"/><w:b/></w:rPr><w:t>甲乙</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr;
                aXml = aXml.replaceAll("WIDTH"_ostr, OString::number(nWidth));
                writeWordMacPart(xWord, u"document.xml"_ustr, aXml);
                uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
                uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
            }
            loadFromURL(aFile.GetURL());
            // Two real GB2312 14pt bold glyphs advance 570.94twips: the
            // 570twip cell wraps while 571 fits. Legacy measurement is retained.
            assertXPath(parseLayoutDump(), "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout",
                bEnabled && nWidth == 570 ? 2 : 1);
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacFirstCharacterIndentFont)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:ind w:firstLine="640" w:firstLineChars="200"/><w:rPr><w:sz w:val="28"/></w:rPr></w:pPr><w:r><w:rPr><w:sz w:val="32"/></w:rPr><w:t>Alpha</w:t></w:r></w:p><w:p><w:pPr><w:ind w:firstLine="0"/><w:rPr><w:sz w:val="28"/></w:rPr></w:pPr><w:r><w:rPr><w:sz w:val="32"/></w:rPr><w:t>Beta</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    for (bool bEnabled : {false, true})
    {
        const OUString aValue = bEnabled ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aName.pData, aValue.pData);
        loadFromURL(aFile.GetURL());
        auto xMetaFile = getSwDocShell()->GetPreviewMetaFile();
        MetafileXmlDump aDumper;
        auto pMeta = dumpAndParse(aDumper, *xMetaFile);
        assertXPath(pMeta, "//textarray", 2);
        const auto nMissing = getXPath(pMeta, "(//textarray)[1]", "x").toInt32();
        const auto nExplicitZero = getXPath(pMeta, "(//textarray)[2]", "x").toInt32();
        // Two characters at 16pt override the 14pt paragraph-mark font.
        CPPUNIT_ASSERT_EQUAL(sal_Int32(bEnabled ? 640 : 560), nMissing - nExplicitZero);
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacContinuousFirstSectionGrid)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, true, true, false, false, false, false, false,
                       false, false, false, 0, 0, true);
    for (bool bMac : {false, true})
    {
        const OUString aValue = bMac ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aName.pData, aValue.pData);
        loadFromURL(aFile.GetURL());
        const auto& rGrid = getSwDocShell()->GetDoc()->GetPageDesc(0).GetMaster().GetTextGrid();
        CPPUNIT_ASSERT_EQUAL(!bMac, rGrid.GetGridType() == SwTextGrid::NONE);
        if (bMac)
            CPPUNIT_ASSERT_EQUAL(sal_uInt16(312), rGrid.GetBaseHeight());
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacLegacyTableBorders)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    for (sal_Int16 nBorder : {sal_Int16(1), sal_Int16(2)})
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, true, true, false, true, false, false, false,
                           false, false, false, 0, nBorder);
        for (bool bMac : {false, true})
        {
            const OUString aValue = bMac ? u"1"_ustr : u"0"_ustr;
            osl_setEnvironment(aName.pData, aValue.pData);
            loadFromURL(aFile.GetURL());
            uno::Reference<text::XTextTablesSupplier> xSupplier(mxComponent, uno::UNO_QUERY_THROW);
            uno::Reference<container::XIndexAccess> xTables(xSupplier->getTextTables(), uno::UNO_QUERY_THROW);
            uno::Reference<text::XTextTable> xTable(xTables->getByIndex(0), uno::UNO_QUERY_THROW);
            const auto aBorder = getProperty<table::BorderLine2>(xTable->getCellByName(u"A1"_ustr), u"TopBorder"_ustr);
            if (bMac)
            {
                CPPUNIT_ASSERT_EQUAL(table::BorderLineStyle::SOLID, aBorder.LineStyle);
                CPPUNIT_ASSERT(aBorder.LineWidth >= 25 && aBorder.LineWidth <= 28);
                CPPUNIT_ASSERT_EQUAL(sal_Int32(0xFF0000), aBorder.Color);
            }
            else
                CPPUNIT_ASSERT(aBorder.LineStyle != table::BorderLineStyle::SOLID);
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacGbBoldTableWrap)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true, false, false, true);
    const OUString aOff = u"0"_ustr;
    osl_setEnvironment(aName.pData, aOff.pData);
    loadFromURL(aFile.GetURL());
    assertXPath(parseLayoutDump(), "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout", 2);
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    loadFromURL(aFile.GetURL());
    // Genuine 12pt GB2312 synthetic bold advances by 12.234pt in Mac Word.
    // Two characters no longer fit the 24.1pt cell edit area.
    assertXPath(parseLayoutDump(), "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout", 4);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacPunctuationWrap)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true, false, false, false, false, false, false, true);
    const OUString aOff = u"0"_ustr;
    osl_setEnvironment(aName.pData, aOff.pData);
    loadFromURL(aFile.GetURL());
    assertXPath(parseLayoutDump(), "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout", 2);
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    loadFromURL(aFile.GetURL());
    // Actual Mac Word export fits thirteen 16pt glyphs, including a comma,
    // in this 200pt cell by compressing the punctuation by half an em.
    assertXPath(parseLayoutDump(), "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout", 1);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:snapToGrid w:val="0"/><w:ind w:right="5646"/><w:jc w:val="both"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="28"/></w:rPr><w:t>甲乙，丁戊己，庚辛，癸子丑寅卯辰巳午未</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    loadFromURL(aFile.GetURL());
    // The actual Word 169pt three-comma probe fits twelve, not thirteen,
    // 14pt glyphs. Three half-em allowances must not accumulate.
    assertXPath(parseLayoutDump(), "/root/page/body/txt/SwParaPortion/SwLineLayout[1]", "length", u"12");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacWesternPunctuationHint)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:r><w:rPr><w:rFonts w:ascii="Aptos" w:hAnsi="Aptos" w:eastAsia="DengXian"/></w:rPr><w:t>甲“乙”丙—丁</w:t></w:r></w:p><w:p><w:r><w:rPr><w:rFonts w:hint="eastAsia" w:ascii="Aptos" w:hAnsi="Aptos" w:eastAsia="DengXian"/></w:rPr><w:t>甲“乙”丙—丁</w:t></w:r></w:p><w:sectPr/></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    for (bool bMac : {false, true})
    {
        const OUString aValue = bMac ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aName.pData, aValue.pData);
        loadFromURL(aFile.GetURL());
        parseLayoutDump();
        auto* pShell = getSwDocShell()->GetWrtShell();
        auto* pFrame = dynamic_cast<SwTextFrame*>(pShell->GetLayout()->GetLower()->GetLower()->GetLower());
        CPPUNIT_ASSERT(pFrame);
        auto scriptAt = [](const SwScriptInfo& rInfo, sal_Int32 nIndex) {
            for (size_t i = 0; i < rInfo.CountScriptChg(); ++i)
                if (nIndex < rInfo.GetScriptChg(i).get())
                    return sal_Int16(rInfo.GetScriptType(i));
            return sal_Int16(-1);
        };
        for (sal_Int32 nIndex : {1, 3, 5})
            CPPUNIT_ASSERT_EQUAL(bMac ? i18n::ScriptType::LATIN : i18n::ScriptType::ASIAN,
                                 scriptAt(*pFrame->GetScriptInfo(), nIndex));
        pFrame = dynamic_cast<SwTextFrame*>(pFrame->GetNext());
        CPPUNIT_ASSERT(pFrame);
        for (sal_Int32 nIndex : {1, 3, 5})
            CPPUNIT_ASSERT_EQUAL(i18n::ScriptType::ASIAN,
                                 scriptAt(*pFrame->GetScriptInfo(), nIndex));
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacAutoFitCjkBreakUnits)
{
    const std::array<OUString, 6> aNames = {
        u"COCOLIAN_WORD_MAC_METRICS"_ustr, u"COCOLIAN_WORD_MAC_AUTOFIT"_ustr,
        u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr, u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr, u"COCOLIAN_WORD_MAC_GRID_BOTTOM"_ustr };
    std::array<OUString, 6> aPrevious;
    std::array<bool, 6> aHadPrevious;
    for (size_t i = 0; i < aNames.size(); ++i)
        aHadPrevious[i] = osl_getEnvironment(aNames[i].pData, &aPrevious[i].pData)
            == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
        {
            if (aHadPrevious[i])
                osl_setEnvironment(aNames[i].pData, aPrevious[i].pData);
            else
                osl_clearEnvironment(aNames[i].pData);
        }
    });
    const OUString aOn = u"1"_ustr;
    for (const auto& rName : aNames)
        osl_setEnvironment(rName.pData, aOn.pData);
    for (const OUString& rText : { u"类型"_ustr, u"类，"_ustr, u"（类"_ustr })
    for (bool bNoWrap : { false, true })
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            const OString aXml = R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="4998" w:type="pct"/><w:tblInd w:w="0" w:type="dxa"/><w:tblLayout w:type="autofit"/></w:tblPr><w:tblGrid><w:gridCol w:w="1518"/><w:gridCol w:w="868"/><w:gridCol w:w="6307"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="873" w:type="pct"/><w:noWrap/><w:tcMar><w:left w:w="195" w:type="dxa"/><w:right w:w="195" w:type="dxa"/></w:tcMar></w:tcPr><w:p><w:pPr><w:ind w:left="0" w:right="0"/><w:spacing w:after="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Segoe UI" w:hAnsi="Segoe UI" w:eastAsia="SimSun"/><w:b/><w:sz w:val="24"/><w:lang w:val="en-US" w:eastAsia="zh-CN"/></w:rPr><w:t>参数</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:tcW w:w="499" w:type="pct"/>NO_WRAP<w:tcMar><w:left w:w="195" w:type="dxa"/><w:right w:w="195" w:type="dxa"/></w:tcMar></w:tcPr><w:p><w:pPr><w:ind w:left="0" w:right="0"/><w:spacing w:after="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Segoe UI" w:hAnsi="Segoe UI" w:eastAsia="SimSun"/><w:b/><w:sz w:val="24"/><w:lang w:val="en-US" w:eastAsia="zh-CN"/></w:rPr><w:t>CJK_TEXT</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:tcW w:w="3627" w:type="pct"/><w:tcMar><w:left w:w="195" w:type="dxa"/><w:right w:w="195" w:type="dxa"/></w:tcMar></w:tcPr><w:p><w:pPr><w:ind w:left="0" w:right="0"/><w:spacing w:after="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Segoe UI" w:hAnsi="Segoe UI" w:eastAsia="SimSun"/><w:b/><w:sz w:val="24"/><w:lang w:val="en-US" w:eastAsia="zh-CN"/></w:rPr><w:t>说明</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1800" w:right="1800"/></w:sectPr></w:body></w:document>)xml"_ostr;
            writeWordMacPart(xWord, u"document.xml"_ustr,
                aXml.replaceAll("NO_WRAP"_ostr, bNoWrap ? "<w:noWrap/>"_ostr : ""_ostr)
                    .replaceAll("CJK_TEXT"_ostr, OUStringToOString(rText, RTL_TEXTENCODING_UTF8)));
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        loadFromURL(aFile.GetURL());
        Scheduler::ProcessEventsToIdle();
        calcLayout();
        auto pLayout = parseLayoutDump();
        // CJK line-break opportunities, not a multi-character dictionary word,
        // set an AutoFit column minimum. Explicit noWrap keeps that word intact.
        const auto nLines = countXPathNodes(pLayout,
            "/root/page[1]/body/tab/row/cell[2]/txt/SwParaPortion/SwLineLayout");
        const auto nWidth = getXPath(pLayout,
            "/root/page[1]/body/tab/row/cell[2]/infos/bounds", "width").toInt32();
        const OString aDiagnostic = "noWrap="_ostr + (bNoWrap ? "1"_ostr : "0"_ostr)
            + " width="_ostr + OString::number(nWidth)
            + " text="_ostr + OUStringToOString(rText, RTL_TEXTENCODING_UTF8);
        const bool bCanSplit = !bNoWrap && rText == u"类型";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(aDiagnostic.getStr(), bCanSplit ? 2 : 1, nLines);
        if (bCanSplit)
            CPPUNIT_ASSERT_MESSAGE(aDiagnostic.getStr(), nWidth < 860);
        else
            CPPUNIT_ASSERT_MESSAGE(aDiagnostic.getStr(), nWidth >= 870);
    }

    // A mixed CJK/Latin header must keep its existing dictionary boundaries.
    // Joining both sides of the slash used to widen this column and collapse
    // the four-line Word header to two lines.
    utl::TempFileNamed aMixedFile;
    aMixedFile.EnableKillingFile();
    makeWordMacPackage(aMixedFile, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aMixedFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        const std::array<int, 5> aWeights = { 711, 499, 499, 875, 2413 };
        const std::array<OString, 5> aHeaders = {
            "参数"_ostr, "类型"_ostr, "必填/选填"_ostr, "示例"_ostr, "说明"_ostr };
        const std::array<OString, 5> aValues = {
            "content-type"_ostr, "string"_ostr, "必填"_ostr,
            "\"application/json\""_ostr, "application/json"_ostr };
        OStringBuffer aRows;
        for (bool bHeader : { true, false })
        {
            aRows.append("<w:tr>");
            if (bHeader)
                aRows.append("<w:trPr><w:tblHeader/></w:trPr>");
            for (size_t i = 0; i < aWeights.size(); ++i)
            {
                aRows.append(R"xml(<w:tc><w:tcPr><w:tcW w:type="pct" w:w=")xml"_ostr
                    + OString::number(aWeights[i]) + "\"/>"_ostr);
                if (!bHeader && i < 4)
                    aRows.append("<w:noWrap/>");
                aRows.append(R"xml(<w:tcMar><w:left w:w="195" w:type="dxa"/><w:right w:w="195" w:type="dxa"/></w:tcMar></w:tcPr><w:p><w:pPr>)xml"_ostr
                    + (bHeader ? "<w:snapToGrid/>"_ostr : "<w:snapToGrid w:val=\"0\"/>"_ostr)
                    + R"xml(<w:spacing w:before="0" w:after="0" w:line="312" w:lineRule="exact"/>)xml"_ostr);
                if (bHeader)
                    aRows.append("<w:jc w:val=\"center\"/>");
                aRows.append("</w:pPr>");
                const auto appendRun = [&](const OString& rText, bool bAsian) {
                    aRows.append(R"xml(<w:r><w:rPr><w:rFonts w:ascii="Segoe UI" w:hAnsi="Segoe UI" w:eastAsia="SimSun" w:hint=")xml"_ostr
                        + (bAsian ? "eastAsia"_ostr : "default"_ostr)
                        + R"xml("/><w:sz w:val="24"/><w:lang w:val="en-US" w:eastAsia="zh-CN"/>)xml"_ostr);
                    if (bHeader)
                        aRows.append("<w:b/>");
                    aRows.append("</w:rPr><w:t>"_ostr + rText + "</w:t></w:r>"_ostr);
                };
                if (bHeader && i == 2)
                {
                    appendRun("必填"_ostr, true);
                    appendRun("/"_ostr, false);
                    appendRun("选填"_ostr, true);
                }
                else
                    appendRun(bHeader ? aHeaders[i] : aValues[i], bHeader || i == 2);
                aRows.append("</w:p></w:tc>");
            }
            aRows.append("</w:tr>");
        }
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="4998" w:type="pct"/><w:tblInd w:w="0" w:type="dxa"/><w:tblLayout w:type="autofit"/></w:tblPr><w:tblGrid><w:gridCol w:w="1711"/><w:gridCol w:w="977"/><w:gridCol w:w="870"/><w:gridCol w:w="2215"/><w:gridCol w:w="2920"/></w:tblGrid>)xml"_ostr
            + aRows.makeStringAndClear()
            + R"xml(</w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1800" w:right="1800"/><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    loadFromURL(aMixedFile.GetURL());
    for (int nRound = 0; nRound < 2; ++nRound)
    {
        auto pLayout = parseLayoutDump();
        CPPUNIT_ASSERT_EQUAL(4, countXPathNodes(pLayout,
            "/root/page/body/tab/row[1]/cell[3]/txt/SwParaPortion/SwLineLayout"));
        if (!nRound)
            saveAndReload(TestFilter::DOCX);
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacYaHeiGridHeight)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    for (sal_Int16 nSize : {sal_Int16(9), sal_Int16(10)})
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, true, true, false, false, false, false, false,
                           false, false, false, 0, 0, false, nSize);
        loadFromURL(aFile.GetURL());
        // A Mac Word print-PDF probe confirms 9pt YaHei centers its glyphs
        // in the full 150% grid pitch, including the first line. 10pt occupies
        // two grid rows and ignores that multiplier.
        assertXPath(parseLayoutDump(), "/root/page[1]/body/txt[1]/infos/bounds",
                    "height", nSize == 9 ? u"1404" : u"1872");
    }

    // Actual Mac Word print-PDF probes: a large proportional pitch is a
    // lower bound even when the glyphs already occupy several grid rows.
    struct GridPitchCase { const char* pFont; int nSize; int nLine; int nHeight; };
    for (const auto& rCase : {
        GridPitchCase{ "SimSun", 22, 576, 748 },
        GridPitchCase{ "SimSun", 22, 720, 936 },
        GridPitchCase{ "SimSun", 22, 360, 624 },
        GridPitchCase{ "SimHei", 16, 576, 748 },
        GridPitchCase{ "SimHei", 16, 413, 624 },
        GridPitchCase{ "Microsoft YaHei", 12, 576, 748 },
        GridPitchCase{ "Microsoft YaHei", 12, 360, 624 },
        GridPitchCase{ "Microsoft YaHei", 9, 360, 468 } })
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            const OString aFont(rCase.pFont);
            const OString aRunProperties = R"xml(<w:rFonts w:ascii=")xml"_ostr + aFont
                + R"xml(" w:hAnsi=")xml"_ostr + aFont + R"xml(" w:eastAsia=")xml"_ostr + aFont
                + R"xml(" w:hint="eastAsia"/><w:b/><w:sz w:val=")xml"_ostr
                + OString::number(rCase.nSize * 2) + R"xml("/><w:lang w:eastAsia="zh-CN"/>)xml"_ostr;
            writeWordMacPart(xWord, u"document.xml"_ustr,
                R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:snapToGrid/><w:spacing w:before="0" w:after="0" w:line=")xml"_ostr
                + OString::number(rCase.nLine) + R"xml(" w:lineRule="auto"/><w:rPr>)xml"_ostr
                + aRunProperties + R"xml(</w:rPr></w:pPr><w:r><w:rPr>)xml"_ostr
                + aRunProperties + R"xml(</w:rPr><w:t>标题行距验证</w:t></w:r></w:p><w:p><w:pPr><w:snapToGrid w:val="0"/><w:spacing w:before="0" w:after="0" w:line="312" w:lineRule="exact"/></w:pPr><w:r><w:t>Marker</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1800" w:bottom="1800" w:left="1800" w:right="1800"/><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:body></w:document>)xml"_ostr);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        loadFromURL(aFile.GetURL());
        for (int nRound = 0; nRound < 2; ++nRound)
        {
            auto pDump = parseLayoutDump();
            CPPUNIT_ASSERT_EQUAL(sal_Int32(rCase.nHeight), getXPath(pDump,
                "/root/page/body/txt[1]/infos/bounds", "height").toInt32());
            if (!nRound)
                saveAndReload(TestFilter::DOCX);
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacGridPunctuationCompression)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    for (sal_Int16 nGrid : {sal_Int16(1), sal_Int16(2)})
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true, false, false, false, false, false,
                           false, true, false, nGrid);
        loadFromURL(aFile.GetURL());
        CPPUNIT_ASSERT(getSwDocShell()->GetDoc()->getIDocumentSettingAccess().get(
            DocumentSettingId::DO_NOT_COMPRESS_WITH_GRID));
        // Mac Word compresses punctuation with a lines-only grid, including
        // with doNotUseEastAsianBreakRules and automatic line spacing.
        assertXPath(parseLayoutDump(),
            "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout", 1);
        saveAndReload(TestFilter::DOCX);
        CPPUNIT_ASSERT(getSwDocShell()->GetDoc()->getIDocumentSettingAccess().get(
            DocumentSettingId::DO_NOT_COMPRESS_WITH_GRID));
        CPPUNIT_ASSERT_EQUAL(sal_Int16(1), static_cast<sal_Int16>(
            getSwDocShell()->GetDoc()->getIDocumentSettingAccess().getCharacterCompressionType()));
        assertXPath(parseLayoutDump(),
            "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout", 1);
        if (nGrid == 1)
        {
            uno::Reference<text::XTextTablesSupplier> xSupplier(mxComponent, uno::UNO_QUERY_THROW);
            uno::Reference<container::XIndexAccess> xTables(xSupplier->getTextTables(), uno::UNO_QUERY_THROW);
            uno::Reference<text::XTextTable> xTable(xTables->getByIndex(0), uno::UNO_QUERY_THROW);
            uno::Reference<text::XText> xCell(xTable->getCellByName(u"A1"_ustr), uno::UNO_QUERY_THROW);
            uno::Reference<beans::XPropertySet> xParagraph(getParagraphOfText(1, xCell), uno::UNO_QUERY_THROW);
            style::LineSpacing aSpacing;
            aSpacing.Mode = style::LineSpacingMode::FIX;
            aSpacing.Height = 706;
            xParagraph->setPropertyValue(u"ParaLineSpacing"_ustr, uno::Any(aSpacing));
            assertXPath(parseLayoutDump(),
                "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout", 1);
            aSpacing.Mode = style::LineSpacingMode::PROP;
            aSpacing.Height = 100;
            xParagraph->setPropertyValue(u"ParaLineSpacing"_ustr, uno::Any(aSpacing));
            assertXPath(parseLayoutDump(),
                "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout", 1);
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacMixedScriptLineBudget)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr, u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    const OUString aOn = u"1"_ustr;
    for (const auto& rName : aNames)
        osl_setEnvironment(rName.pData, aOn.pData);
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true, false, false, false, false, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:r><w:t>WIDTH-240</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="4800" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4800"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="4800" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="left"/><w:snapToGrid w:val="0"/><w:autoSpaceDE/><w:autoSpaceDN/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Microsoft YaHei" w:hAnsi="Microsoft YaHei" w:eastAsia="Microsoft YaHei"/><w:sz w:val="22"/></w:rPr><w:t>ABC甲乙丙丁，戊己庚辛12345壬癸，子丑寅卯1234辰，巳午未申7890酉戌。</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:r><w:t>WIDTH-245</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="4900" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4900"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="4900" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="left"/><w:snapToGrid w:val="0"/><w:autoSpaceDE/><w:autoSpaceDN/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Microsoft YaHei" w:hAnsi="Microsoft YaHei" w:eastAsia="Microsoft YaHei"/><w:sz w:val="22"/></w:rPr><w:t>ABC甲乙丙丁，戊己庚辛12345壬癸，子丑寅卯1234辰，巳午未申7890酉戌。</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:r><w:t>WIDTH-250</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="5000" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="5000"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="5000" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="left"/><w:snapToGrid w:val="0"/><w:autoSpaceDE/><w:autoSpaceDN/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Microsoft YaHei" w:hAnsi="Microsoft YaHei" w:eastAsia="Microsoft YaHei"/><w:sz w:val="22"/></w:rPr><w:t>ABC甲乙丙丁，戊己庚辛12345壬癸，子丑寅卯1234辰，巳午未申7890酉戌。</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:r><w:t>WIDTH-255</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="5100" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="5100"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="5100" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="left"/><w:snapToGrid w:val="0"/><w:autoSpaceDE/><w:autoSpaceDN/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Microsoft YaHei" w:hAnsi="Microsoft YaHei" w:eastAsia="Microsoft YaHei"/><w:sz w:val="22"/></w:rPr><w:t>ABC甲乙丙丁，戊己庚辛12345壬癸，子丑寅卯1234辰，巳午未申7890酉戌。</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:r><w:t>WIDTH-260</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="5200" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="5200"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="5200" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="left"/><w:snapToGrid w:val="0"/><w:autoSpaceDE/><w:autoSpaceDN/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Microsoft YaHei" w:hAnsi="Microsoft YaHei" w:eastAsia="Microsoft YaHei"/><w:sz w:val="22"/></w:rPr><w:t>ABC甲乙丙丁，戊己庚辛12345壬癸，子丑寅卯1234辰，巳午未申7890酉戌。</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:r><w:t>WIDTH-265</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="5300" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="5300"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="5300" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="left"/><w:snapToGrid w:val="0"/><w:autoSpaceDE/><w:autoSpaceDN/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Microsoft YaHei" w:hAnsi="Microsoft YaHei" w:eastAsia="Microsoft YaHei"/><w:sz w:val="22"/></w:rPr><w:t>ABC甲乙丙丁，戊己庚辛12345壬癸，子丑寅卯1234辰，巳午未申7890酉戌。</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    loadFromURL(aFile.GetURL());
    auto pLayout = parseLayoutDump();
    // Six independent actual Mac Word exports: the 255pt threshold
    // requires narrower script gaps, not a wider punctuation allowance.
    for (int nCase = 1; nCase <= 6; ++nCase)
        assertXPath(pLayout, "(/root/page/body/tab)[" + OString::number(nCase)
            + "]/row/cell/txt/SwParaPortion/SwLineLayout[1]", "portion",
            nCase <= 3 ? u"ABC甲乙丙丁，戊己庚辛12345壬癸，子丑寅卯"_ustr
                       : u"ABC甲乙丙丁，戊己庚辛12345壬癸，子丑寅卯1234"_ustr);

}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacGridOverflowSetting)
{
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    for (bool bEnabled : {true, false})
    {
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            const OString aXml = R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:compat><w:doNotWrapTextWithPunct w:val=")xml"_ostr
                + (bEnabled ? "1"_ostr : "0"_ostr) + R"xml("/></w:compat></w:settings>)xml"_ostr;
            writeWordMacPart(xWord, u"settings.xml"_ustr, aXml);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        loadFromURL(aFile.GetURL());
        CPPUNIT_ASSERT_EQUAL(bEnabled, getSwDocShell()->GetDoc()->getIDocumentSettingAccess().get(
            DocumentSettingId::DO_NOT_WRAP_PUNCTUATION_WITH_GRID));
        saveAndReload(TestFilter::DOCX);
        CPPUNIT_ASSERT_EQUAL(bEnabled, getSwDocShell()->GetDoc()->getIDocumentSettingAccess().get(
            DocumentSettingId::DO_NOT_WRAP_PUNCTUATION_WITH_GRID));
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacClosingUnitBudget)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    const OUString aOn = u"1"_ustr;
    for (const auto& rName : aNames)
        osl_setEnvironment(rName.pData, aOn.pData);
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true, false, false, false, false, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="560" w:lineRule="exact"/><w:ind w:firstLine="560"/><w:jc w:val="both"/><w:autoSpaceDE/><w:autoSpaceDN/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="SimSun" w:hAnsi="SimSun" w:eastAsia="SimSun"/><w:sz w:val="28"/></w:rPr><w:t>甲乙丙丁200戊己庚辛壬癸子丑寅卯，辰巳2G午未申Word、Excel、ABC甲乙丙丁戊己庚辛壬癸子丑寅卯，辰巳午未申酉戌亥。</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1800" w:right="1800"/></w:sectPr></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    loadFromURL(aFile.GetURL());
    auto pLayout = parseLayoutDump();
    // Actual Mac Word PDF: Excel and its closing punctuation remain on
    // the first line. Script-gap contraction is independent of punctuation
    // compression; charging it twice incorrectly moves the entire unit.
    assertXPath(pLayout, "/root/page/body/txt/SwParaPortion/SwLineLayout[1]",
        "portion", u"甲乙丙丁200戊己庚辛壬癸子丑寅卯，辰巳2G午未申Word、Excel、"_ustr);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacPercentageClosingUnit)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    const OUString aOn = u"1"_ustr;
    for (const auto& rName : aNames)
        osl_setEnvironment(rName.pData, aOn.pData);
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true, false, false, false, false, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="560" w:lineRule="exact"/><w:ind w:firstLine="640"/><w:jc w:val="both"/><w:autoSpaceDE/><w:autoSpaceDN/><w:overflowPunct/><w:snapToGrid/></w:pPr><w:r><w:rPr><w:rFonts w:hint="eastAsia" w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:kern w:val="2"/><w:sz w:val="32"/></w:rPr><w:t>4、甲乙丙丁戊己庚辛壬癸子丑寅卯辰巳午（申酉7.20%）</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1800" w:right="1800"/></w:sectPr></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    loadFromURL(aFile.GetURL());
    auto pLayout = parseLayoutDump();
    // A percentage and its CJK closing parenthesis form one fitting unit.
    // Stopping the backward scan at '%' wrongly moves the entire number.
    assertXPath(pLayout, "/root/page/body/txt/SwParaPortion/SwLineLayout", 1);
    assertXPath(pLayout, "/root/page/body/txt/SwParaPortion/SwLineLayout[1]",
        "portion", u"4、甲乙丙丁戊己庚辛壬癸子丑寅卯辰巳午（申酉7.20%）"_ustr);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacLeftAlignedLineBudget)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    const OUString aOn = u"1"_ustr;
    for (const auto& rName : aNames)
        osl_setEnvironment(rName.pData, aOn.pData);
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true, false, false, false, false, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:snapToGrid w:val="0"/><w:jc w:val="left"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="32"/></w:rPr><w:t>甲乙丙丁戊己，庚辛壬癸子丑。寅卯辰巳午未申酉戌亥甲乙丙丁戊己</w:t></w:r></w:p><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:snapToGrid w:val="0"/><w:ind w:right="4306"/><w:jc w:val="left"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="32"/></w:rPr><w:t>甲乙丙丁戊己，庚辛壬癸子丑</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1800" w:right="1800"/></w:sectPr></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    for (sal_Int32 nMode : {14, 15})
    {
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            const OString aSettings = R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:characterSpacingControl w:val="compressPunctuation"/><w:compat><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val=")xml"_ostr
                + OString::number(nMode) + R"xml("/></w:compat></w:settings>)xml"_ostr;
            writeWordMacPart(xWord, u"settings.xml"_ustr, aSettings);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        loadFromURL(aFile.GetURL());
        auto pLayout = parseLayoutDump();
        // Modern left-aligned body lines cannot use the extra break unit.
        // Keep the legacy compression behavior for pre-2013 documents.
        const OUString aExpected = u"甲乙丙丁戊己，庚辛壬癸子丑。寅卯辰巳午未申酉戌亥甲"_ustr
            + (nMode < 15 ? u"乙"_ustr : OUString());
        assertXPath(pLayout, "/root/page/body/txt[1]/SwParaPortion/SwLineLayout[1]",
            "portion", aExpected);
        assertXPath(pLayout, "/root/page/body/txt[2]/SwParaPortion/SwLineLayout", 1);
    }
    // Justification probes can truncate a word's temporary portion without
    // changing its formatting. Keep wordWrap independent of those probes.
    for (bool bWordWrap : { false, true })
    {
        utl::TempFileNamed aProbe;
        aProbe.EnableKillingFile();
        makeWordMacPackage(aProbe, false, true);
        const OString aPrefix = "2030/01/01 12:00:00.000|1.0.0|I|SyntheticBatchWorker_Complete 12345"_ostr;
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aProbe.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            writeWordMacPart(xWord, u"document.xml"_ustr,
                R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:jc w:val="both"/><w:wordWrap w:val=")xml"_ostr
                + (bWordWrap ? "1"_ostr : "0"_ostr)
                + R"xml("/><w:spacing w:before="0" w:after="0" w:line="360" w:lineRule="auto"/><w:snapToGrid/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312" w:hint="eastAsia"/><w:sz w:val="24"/><w:lang w:val="zh-CN" w:eastAsia="zh"/></w:rPr><w:t>)xml"_ostr
                + aPrefix + R"xml( messages for session 0123456789abcdef0123456789abcdef</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1800" w:right="1800"/><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:body></w:document>)xml"_ostr);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        loadFromURL(aProbe.GetURL());
        for (bool bReload : { false, true })
        {
            if (bReload)
                saveAndReload(TestFilter::DOCX);
            const OUString aExpected = OUString::fromUtf8(aPrefix)
                + (bWordWrap ? OUString() : u" m"_ustr);
            CPPUNIT_ASSERT_EQUAL(aExpected, getXPath(parseLayoutDump(),
                "/root/page/body/txt[1]/SwParaPortion/SwLineLayout[1]", "portion").trim());
        }
    }

}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacPdfFallbackBold)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    for (bool bMac : { false, true })
    {
        const OUString aValue = bMac ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aName.pData, aValue.pData);
        for (bool bVertical : { false, true })
        {
            for (bool bOutline : { false, true })
            {
                utl::TempFileNamed aFile;
                aFile.EnableKillingFile();
                makeWordMacPackage(aFile, false, true);
                {
                    auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                        ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
                    auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
                    // Helvetica has a native bold face; its SimSun fallback
                    // needs synthesis for these CJK glyphs in the pinned pack.
                    writeWordMacPart(xWord, u"document.xml"_ustr,
                        R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Helvetica" w:hAnsi="Helvetica" w:eastAsia="Helvetica"/><w:sz w:val="24"/><w:b/>)xml"_ostr
                        + (bOutline ? "<w:outline/>"_ostr : OString())
                        + R"xml(</w:rPr><w:t>AB甲乙</w:t></w:r></w:p><w:sectPr>)xml"_ostr
                        + (bVertical ? R"xml(<w:textDirection w:val="tbRl"/>)xml"_ostr : OString())
                        + R"xml(</w:sectPr></w:body></w:document>)xml"_ostr);
                    uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
                    uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
                }
                loadFromURL(aFile.GetURL());
                save(TestFilter::PDF_WRITER);
                auto pPdf = parsePDFExport();
                CPPUNIT_ASSERT(pPdf);
                auto pPage = pPdf->openPage(0);
                auto pText = pPage->getTextPage();
                bool bSawLatin = false, bSawCjk = false;
                for (int i = 0; i < pPage->getObjectCount(); ++i)
                {
                    auto pObject = pPage->getObject(i);
                    const auto aText = pObject->getText(pText);
                    if (aText.indexOf(u'A') >= 0 || aText.indexOf(u'B') >= 0)
                    {
                        bSawLatin = true;
                        CPPUNIT_ASSERT_EQUAL(bOutline ? vcl::pdf::PDFTextRenderMode::FillStroke
                                                     : vcl::pdf::PDFTextRenderMode::Fill,
                            pObject->getTextRenderMode());
                    }
                    if (aText.indexOf(u'甲') >= 0 || aText.indexOf(u'乙') >= 0)
                    {
                        bSawCjk = true;
                        CPPUNIT_ASSERT_EQUAL(bMac || bOutline ? vcl::pdf::PDFTextRenderMode::FillStroke
                                                             : vcl::pdf::PDFTextRenderMode::Fill,
                            pObject->getTextRenderMode());
                    }
                }
                CPPUNIT_ASSERT(bSawLatin && bSawCjk);
            }
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacStrictGridTablePunctuation)
{
    const std::array<OUString, 4> aNames = {
        u"COCOLIAN_WORD_MAC_METRICS"_ustr, u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr, u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr };
    std::array<OUString, 4> aPrevious;
    std::array<bool, 4> aHadPrevious;
    for (size_t i = 0; i < aNames.size(); ++i)
    {
        aHadPrevious[i] = osl_getEnvironment(aNames[i].pData, &aPrevious[i].pData)
            == osl_Process_E_None;
        const OUString aOn = u"1"_ustr;
        osl_setEnvironment(aNames[i].pData, aOn.pData);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
        {
            if (aHadPrevious[i])
                osl_setEnvironment(aNames[i].pData, aPrevious[i].pData);
            else
                osl_clearEnvironment(aNames[i].pData);
        }
    });
    for (const OString& rAlignment : { "left"_ostr, "center"_ostr })
    for (bool bStrict : { false, true })
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            writeWordMacPart(xWord, u"document.xml"_ustr,
                R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="4480" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4480"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="4480" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:jc w:val="ALIGNMENT"/><w:autoSpaceDE/><w:autoSpaceDN/><w:overflowPunct/><w:snapToGrid/><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="auto"/><w:ind w:firstLine="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Segoe UI" w:hAnsi="Segoe UI" w:eastAsia="宋体"/><w:sz w:val="24"/></w:rPr><w:t>甲乙丙丁APP戊己庚辛壬癸子丑寅卯辰巳。sampleToken甲乙丙丁戊己庚辛壬癸</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:body></w:document>)xml"_ostr.replaceAll("ALIGNMENT"_ostr, rAlignment));
            writeWordMacPart(xWord, u"settings.xml"_ustr,
                R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:characterSpacingControl w:val="compressPunctuation"/><w:compat><w:adjustLineHeightInTable/><w:doNotWrapTextWithPunct w:val=")xml"_ostr
                + (bStrict ? "1"_ostr : "0"_ostr)
                + R"xml("/><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val="15"/></w:compat></w:settings>)xml"_ostr);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        loadFromURL(aFile.GetURL());
        for (bool bReload : { false, true })
        {
            if (bReload)
                saveAndReload(TestFilter::DOCX);
            assertXPath(parseLayoutDump(),
                "//tab/row/cell/txt/SwParaPortion/SwLineLayout[1]", "portion",
                bStrict ? u"甲乙丙丁APP戊己庚辛壬癸子丑寅卯辰"_ustr
                        : u"甲乙丙丁APP戊己庚辛壬癸子丑寅卯辰巳。"_ustr);
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacTableHalfHanging)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr, u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    const OUString aOn = u"1"_ustr;
    for (const auto& rName : aNames)
        osl_setEnvironment(rName.pData, aOn.pData);

    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true, false, false, false, false, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="3081" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="3081"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="3081" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:autoSpaceDE/><w:autoSpaceDN/><w:jc w:val="center"/><w:spacing w:line="400" w:lineRule="exact"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="24"/></w:rPr><w:t>ABCD、甲乙、丙丁戊、EF己庚、辛壬、甲乙丙丁戊己庚辛壬癸。</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    loadFromURL(aFile.GetURL());
    assertXPath(parseLayoutDump(),
        "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout[1]", "portion",
        u"ABCD、甲乙、丙丁戊、EF己庚、"_ustr);
    auto* pShell = getSwDocShell()->GetWrtShell();
    pShell->SttEndDoc(true);
    const auto nFirst = pShell->GetCharRect().Left();
    pShell->Right(SwCursorSkipMode::Chars, false, 16, false);
    const auto nComma = pShell->GetCharRect().Left();
    // Actual Word leaves half of the final 12pt comma outside the cell's
    // text area; preceding commas recover only the remaining interior space.
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2961, nComma - nFirst, 2);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacMultiplePunctuationBudget)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr, u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    const OUString aOn = u"1"_ustr;
    for (const auto& rName : aNames)
        osl_setEnvironment(rName.pData, aOn.pData);
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true, false, false, false, false, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="exact"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Times New Roman"/><w:sz w:val="20"/></w:rPr><w:t>F208</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="4160" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4160"/></w:tblGrid><w:tr><w:trPr><w:cantSplit/></w:trPr><w:tc><w:tcPr><w:tcW w:w="4160" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="both"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="32"/></w:rPr><w:t>甲乙，丁戊己，庚辛，癸子丑寅卯辰巳午未</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="exact"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Times New Roman"/><w:sz w:val="20"/></w:rPr><w:t>F207.6</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="4152" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4152"/></w:tblGrid><w:tr><w:trPr><w:cantSplit/></w:trPr><w:tc><w:tcPr><w:tcW w:w="4152" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="both"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="32"/></w:rPr><w:t>甲乙，丁戊己，庚辛，癸子丑寅卯辰巳午未</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="exact"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Times New Roman"/><w:sz w:val="20"/></w:rPr><w:t>F207.2</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="4144" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4144"/></w:tblGrid><w:tr><w:trPr><w:cantSplit/></w:trPr><w:tc><w:tcPr><w:tcW w:w="4144" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="both"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="32"/></w:rPr><w:t>甲乙，丁戊己，庚辛，癸子丑寅卯辰巳午未</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="exact"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Times New Roman"/><w:sz w:val="20"/></w:rPr><w:t>F206.4</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="4128" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4128"/></w:tblGrid><w:tr><w:trPr><w:cantSplit/></w:trPr><w:tc><w:tcPr><w:tcW w:w="4128" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="both"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="32"/></w:rPr><w:t>甲乙，丁戊己，庚辛，癸子丑寅卯辰巳午未</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="exact"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Times New Roman"/><w:sz w:val="20"/></w:rPr><w:t>F204</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="4080" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4080"/></w:tblGrid><w:tr><w:trPr><w:cantSplit/></w:trPr><w:tc><w:tcPr><w:tcW w:w="4080" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="both"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="32"/></w:rPr><w:t>甲乙，丁戊己，庚辛，癸子丑寅卯辰巳午未</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="exact"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Times New Roman"/><w:sz w:val="20"/></w:rPr><w:t>F200</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="4000" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4000"/></w:tblGrid><w:tr><w:trPr><w:cantSplit/></w:trPr><w:tc><w:tcPr><w:tcW w:w="4000" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="both"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="32"/></w:rPr><w:t>甲乙，丁戊己，庚辛，癸子丑寅卯辰巳午未</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="exact"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Times New Roman"/><w:sz w:val="20"/></w:rPr><w:t>F192</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="3840" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="3840"/></w:tblGrid><w:tr><w:trPr><w:cantSplit/></w:trPr><w:tc><w:tcPr><w:tcW w:w="3840" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="both"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="32"/></w:rPr><w:t>甲乙，丁戊己，庚辛，癸子丑寅卯辰巳午未</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="exact"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Times New Roman"/><w:sz w:val="20"/></w:rPr><w:t>Y208</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="4160" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4160"/></w:tblGrid><w:tr><w:trPr><w:cantSplit/></w:trPr><w:tc><w:tcPr><w:tcW w:w="4160" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="both"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Microsoft YaHei" w:hAnsi="Microsoft YaHei" w:eastAsia="Microsoft YaHei"/><w:sz w:val="32"/></w:rPr><w:t>甲乙，丁戊己，庚辛，癸子丑寅卯辰巳午未</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="exact"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Times New Roman"/><w:sz w:val="20"/></w:rPr><w:t>Y207.6</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="4152" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4152"/></w:tblGrid><w:tr><w:trPr><w:cantSplit/></w:trPr><w:tc><w:tcPr><w:tcW w:w="4152" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="both"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Microsoft YaHei" w:hAnsi="Microsoft YaHei" w:eastAsia="Microsoft YaHei"/><w:sz w:val="32"/></w:rPr><w:t>甲乙，丁戊己，庚辛，癸子丑寅卯辰巳午未</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="exact"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Times New Roman"/><w:sz w:val="20"/></w:rPr><w:t>Y207.2</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="4144" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4144"/></w:tblGrid><w:tr><w:trPr><w:cantSplit/></w:trPr><w:tc><w:tcPr><w:tcW w:w="4144" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="both"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Microsoft YaHei" w:hAnsi="Microsoft YaHei" w:eastAsia="Microsoft YaHei"/><w:sz w:val="32"/></w:rPr><w:t>甲乙，丁戊己，庚辛，癸子丑寅卯辰巳午未</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="exact"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Times New Roman"/><w:sz w:val="20"/></w:rPr><w:t>Y206.4</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="4128" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4128"/></w:tblGrid><w:tr><w:trPr><w:cantSplit/></w:trPr><w:tc><w:tcPr><w:tcW w:w="4128" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="both"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Microsoft YaHei" w:hAnsi="Microsoft YaHei" w:eastAsia="Microsoft YaHei"/><w:sz w:val="32"/></w:rPr><w:t>甲乙，丁戊己，庚辛，癸子丑寅卯辰巳午未</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="exact"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Times New Roman"/><w:sz w:val="20"/></w:rPr><w:t>Y204</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="4080" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4080"/></w:tblGrid><w:tr><w:trPr><w:cantSplit/></w:trPr><w:tc><w:tcPr><w:tcW w:w="4080" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="both"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Microsoft YaHei" w:hAnsi="Microsoft YaHei" w:eastAsia="Microsoft YaHei"/><w:sz w:val="32"/></w:rPr><w:t>甲乙，丁戊己，庚辛，癸子丑寅卯辰巳午未</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="exact"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Times New Roman"/><w:sz w:val="20"/></w:rPr><w:t>Y200</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="4000" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4000"/></w:tblGrid><w:tr><w:trPr><w:cantSplit/></w:trPr><w:tc><w:tcPr><w:tcW w:w="4000" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="both"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Microsoft YaHei" w:hAnsi="Microsoft YaHei" w:eastAsia="Microsoft YaHei"/><w:sz w:val="32"/></w:rPr><w:t>甲乙，丁戊己，庚辛，癸子丑寅卯辰巳午未</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="exact"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Times New Roman"/><w:sz w:val="20"/></w:rPr><w:t>Y192</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:w="3840" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="3840"/></w:tblGrid><w:tr><w:trPr><w:cantSplit/></w:trPr><w:tc><w:tcPr><w:tcW w:w="3840" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:jc w:val="both"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="Microsoft YaHei" w:hAnsi="Microsoft YaHei" w:eastAsia="Microsoft YaHei"/><w:sz w:val="32"/></w:rPr><w:t>甲乙，丁戊己，庚辛，癸子丑寅卯辰巳午未</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    loadFromURL(aFile.GetURL());
    auto pLayout = parseLayoutDump();
    // Actual Word PDFs cover two native fonts and seven widths. A
    // three-comma line may contract to 200pt but must not gain a glyph at 192pt.
    for (int nCase = 1; nCase <= 14; ++nCase)
        assertXPath(pLayout, "(//body/tab)[" + OString::number(nCase)
            + "]/row/cell/txt/SwParaPortion/SwLineLayout[1]", "portion",
            nCase % 7 ? u"甲乙，丁戊己，庚辛，癸子丑"_ustr
                      : u"甲乙，丁戊己，庚辛，癸子"_ustr);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacFlexibleScriptGap)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr, u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    const OUString aOn = u"1"_ustr;
    for (const auto& rName : aNames)
        osl_setEnvironment(rName.pData, aOn.pData);
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true, false, false, false, false, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="700" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="700"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="700" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:autoSpaceDE/><w:autoSpaceDN/><w:spacing w:before="0" w:after="0" w:line="360" w:lineRule="exact"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="24"/></w:rPr><w:t>甲1乙</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    loadFromURL(aFile.GetURL());
    auto pLayout = parseLayoutDump();
    assertXPath(pLayout, "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout", 1);
    auto xMetaFile = getSwDocShell()->GetPreviewMetaFile();
    MetafileXmlDump aDumper;
    auto pMeta = dumpAndParse(aDumper, *xMetaFile);
    const sal_Int32 nFirst = getXPath(pMeta, "(//textarray)[1]", "x").toInt32();
    // Word narrows both 3pt script gaps to 2.5pt to fit this 35pt cell.
    CPPUNIT_ASSERT_EQUAL(sal_Int32(290),
        getXPath(pMeta, "(//textarray)[2]", "x").toInt32() - nFirst);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(460),
        getXPath(pMeta, "(//textarray)[3]", "x").toInt32() - nFirst);

}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacAutomaticScriptSpacing)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    const OUString aFlexName = u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr;
    OUString aPreviousFlex;
    const bool bHadFlex = osl_getEnvironment(aFlexName.pData, &aPreviousFlex.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestoreFlex([&] {
        if (bHadFlex)
            osl_setEnvironment(aFlexName.pData, aPreviousFlex.pData);
        else
            osl_clearEnvironment(aFlexName.pData);
    });
    const OUString aFixedGaps = u"0"_ustr;
    osl_setEnvironment(aFlexName.pData, aFixedGaps.pData);
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true, false, false, false, false, false, false, false, true);
    const OUString aOff = u"0"_ustr;
    osl_setEnvironment(aName.pData, aOff.pData);
    loadFromURL(aFile.GetURL());
    assertXPath(parseLayoutDump(), "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout", 1);
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    loadFromURL(aFile.GetURL());
    // The base profile's fixed quarter-em gaps total 36pt and exceed this
    // 35pt cell. The flexible server mode is checked separately against Word
    // in testWordMacFlexibleScriptGap; legacy fifth-em gaps fit one line.
    assertXPath(parseLayoutDump(), "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout", 2);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacSmallTableGridProportional)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, true, true, false, true);
    const OUString aOff = u"0"_ustr;
    osl_setEnvironment(aName.pData, aOff.pData);
    loadFromURL(aFile.GetURL());
    assertXPath(parseLayoutDump(), "/root/page/body/tab/row/cell/txt[1]/infos/bounds", "height", u"936");
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    loadFromURL(aFile.GetURL());
    // Mac Word applies 150% spacing to text fitting one grid row, even in
    // tables: all three lines occupy 468 twips, including the first line.
    assertXPath(parseLayoutDump(), "/root/page/body/tab/row/cell/txt[1]/infos/bounds", "height", u"1404");
    assertXPath(parseLayoutDump(), "/root/page/body/tab/row/cell/txt[2]/infos/bounds", "height", u"468");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacSmallBodyGridProportional)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, true, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::READWRITE);
        auto xStream = xWord->openStreamElement(u"document.xml"_ustr, embed::ElementModes::READ);
        auto xInput = xStream->getInputStream();
        uno::Sequence<sal_Int8> aBytes;
        OStringBuffer aBuffer;
        for (sal_Int32 nRead; (nRead = xInput->readBytes(aBytes, 4096)) > 0;)
            aBuffer.append(reinterpret_cast<const char*>(aBytes.getConstArray()), nRead);
        xInput->closeInput();
        xInput.clear();
        const OString aDocument = aBuffer.makeStringAndClear();
        xStream.clear();
        writeWordMacPart(xWord, u"document.xml"_ustr,
            aDocument.replaceAll("w:val=\"36\""_ostr, "w:val=\"24\""_ostr));
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    for (bool bMac : { false, true })
    {
        const OUString aValue = bMac ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aName.pData, aValue.pData);
        loadFromURL(aFile.GetURL());
        for (bool bReload : { false, true })
        {
            if (bReload)
                saveAndReload(TestFilter::DOCX);
            auto pDump = parseLayoutDump();
            // One grid row with 150% leading occupies the full 468 twips,
            // including the first line, in Mac Word.
            assertXPath(pDump, "/root/page[1]/body/txt[1]/infos/bounds", "height",
                bMac ? u"1404"_ustr : u"1248"_ustr);
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordBodyGridProportional)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    const OUString aValue = u"1"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    osl_setEnvironment(aName.pData, aValue.pData);
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, true, true);
    loadFromURL(aFile.GetURL());
    auto pXmlDoc = parseLayoutDump();
    // 18pt text occupies two 312-twip grid rows. Mac Word retains
    // that 31.2pt pitch even with explicit 150% line spacing.
    assertXPath(pXmlDoc, "/root/page[1]/body/txt[1]/infos/bounds", "height", u"1872");
    assertXPath(pXmlDoc, "/root/page[1]/body/txt[2]/infos/bounds", "height", u"1872");
    // Empty paragraphs use the same grid multiple, without extra leading.
    assertXPath(pXmlDoc, "/root/page[1]/body/txt[3]/infos/bounds", "height", u"624");
    assertXPath(pXmlDoc, "/root/page[1]/body/txt[4]/infos/bounds", "height", u"624");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacMissingParagraphDefaults)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, false);
    const OUString aOff = u"0"_ustr;
    osl_setEnvironment(aName.pData, aOff.pData);
    loadFromURL(aFile.GetURL());
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(getParagraph(1), u"ParaBottomMargin"_ustr));
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    loadFromURL(aFile.GetURL());
    CPPUNIT_ASSERT_EQUAL(sal_Int32(282), getProperty<sal_Int32>(getParagraph(1), u"ParaBottomMargin"_ustr));
    utl::TempFileNamed aExplicitFile;
    aExplicitFile.EnableKillingFile();
    makeWordMacPackage(aExplicitFile, false, true);
    loadFromURL(aExplicitFile.GetURL());
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(getParagraph(1), u"ParaBottomMargin"_ustr));
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf166652)
{
    createSwDoc("line-spacing-80percent.fodt");
    auto pXmlDoc = parseLayoutDump();

    // Check that all line heights are identical
    assertXPath(pXmlDoc, "//cell[1]/txt", 11);
    assertXPath(pXmlDoc, "//cell[1]/txt[1]/infos/bounds", "height", u"552");
    // Before the fix, this was 691
    assertXPath(pXmlDoc, "//cell[1]/txt[2]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[1]/txt[3]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[1]/txt[4]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[1]/txt[5]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[1]/txt[6]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[1]/txt[7]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[1]/txt[8]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[1]/txt[9]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[1]/txt[10]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[1]/txt[11]/infos/bounds", "height", u"552");

    assertXPath(pXmlDoc, "//cell[2]/txt", 11);
    assertXPath(pXmlDoc, "//cell[2]/txt[1]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[2]/txt[2]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[2]/txt[3]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[2]/txt[4]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[2]/txt[5]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[2]/txt[6]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[2]/txt[7]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[2]/txt[8]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[2]/txt[9]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[2]/txt[10]/infos/bounds", "height", u"552");
    assertXPath(pXmlDoc, "//cell[2]/txt[11]/infos/bounds", "height", u"552");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf165094)
{
    // Given a document that:
    // 1. Has 45 leading paragraphs (more than SwLayHelper::mnMaxParaPerPage);
    // 2. Has the next paragraph hidden;
    // 2.1. That hidden paragraph has "page break before with page style";
    // 2.2. That page style is different than page style used before;
    // 2.3. That hidden paragraph has "with page number", and the number is odd (previous page is
    //      also odd) - this is actually an alternative to 2.2;
    // 3. Then immediately goes a table.
    createSwDoc("tdf165094.fodt");
    auto pXmlDoc = parseLayoutDump();

    // Check that the page break is created, with correct settings

    assertXPath(pXmlDoc, "//page", 3);

    assertXPath(pXmlDoc, "//page[1]", "formatName", u"Default Page Style");
    assertXPath(pXmlDoc, "//page[1]/page_info", "virtNum", u"1");
    assertXPath(pXmlDoc, "//page[1]/body/txt", 45);

    assertXPath(pXmlDoc, "//page[2]/page_info", "virtNum", u"2");
    assertXPath(pXmlDoc, "//page[2]/infos/bounds", "height", u"0"); // hidden
    assertXPath(pXmlDoc, "//page[2]/body", 0);

    assertXPath(pXmlDoc, "//page[3]", "formatName", u"Style1");
    assertXPath(pXmlDoc, "//page[3]/page_info", "virtNum", u"5");
    assertXPath(pXmlDoc, "//page[3]/body/txt[1]/infos/bounds", "height", u"0"); // hidden
    assertXPath(pXmlDoc, "//page[3]/body/tab", 1);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf166691)
{
    // Given a document with a table which disallows breaking across pages, and that has a cell
    // with an inner section with hide condition (evaluated to true), such as to have the cell
    // height enough to fit on page 1, when the section is hidden, but too tall when it's shown:
    createSwDoc("table-with-hidden-section.fodt");
    auto pXmlDoc = parseLayoutDump();

    // Check that there is only a single page
    assertXPath(pXmlDoc, "//page", 1);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf166871)
{
    // Given a document with 6-column-layout pages, having hundreds of small paragraphs per page
    // (normal and headings); its metadata has the number of pages, paragraphs, etc.:
    createSwDoc("many-paragraphs-per-page-in-6-columns.odt");
    auto pXmlDoc = parseLayoutDump();

    // Check the layout - i.e., the count of paragraphs per column per page.
    // The metadata in the document must not prevent the correct layout.

    assertXPath(pXmlDoc, "//page", 12);

    // The first 11 pages have the same paragraph count in each column
    for (int page = 1; page <= 11; ++page)
    {
        OString Xpath1 = "//page[" + OString::number(page) + "]/body/column";
        assertXPath(pXmlDoc, Xpath1, 6);
        for (int column = 1; column <= 6; ++column)
        {
            OString Xpath2 = Xpath1 + "[" + OString::number(column) + "]/body/txt";
            assertXPath(pXmlDoc, Xpath2, 47);
        }
    }

    // Check the last page
    assertXPath(pXmlDoc, "//page[12]/body/column", 6);
    assertXPath(pXmlDoc, "//page[12]/body/column[1]/body/txt", 14);
    assertXPath(pXmlDoc, "//page[12]/body/column[2]/body/txt", 0);
    assertXPath(pXmlDoc, "//page[12]/body/column[3]/body/txt", 0);
    assertXPath(pXmlDoc, "//page[12]/body/column[4]/body/txt", 0);
    assertXPath(pXmlDoc, "//page[12]/body/column[5]/body/txt", 0);
    assertXPath(pXmlDoc, "//page[12]/body/column[6]/body/txt", 0);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf166978)
{
    // Given a document with a table, which cell has a section:
    createSwDoc("hideSection.fodt");
    CPPUNIT_ASSERT_EQUAL(1, getPages());

    // Hide the section
    auto xTextSectionsSupplier = mxComponent.queryThrow<css::text::XTextSectionsSupplier>();
    auto xSections = xTextSectionsSupplier->getTextSections();
    CPPUNIT_ASSERT(xSections);
    auto xSection = xSections->getByName(u"Section1"_ustr).queryThrow<css::beans::XPropertySet>();
    xSection->setPropertyValue(u"IsVisible"_ustr, cpo::uno::Any(false));
    Scheduler::ProcessEventsToIdle();

    CPPUNIT_ASSERT_EQUAL(1, getPages()); // must not split the second row of the table to page 2
    // Check the layout dump; the whole table must be still on the first page
    auto pXmlDoc = parseLayoutDump();
    assertXPath(pXmlDoc, "//page", 1);
    assertXPath(pXmlDoc, "//page[1]/body/tab", 1);
    assertXPath(pXmlDoc, "//page[1]/body/tab/row", 2);
    assertXPathContent(pXmlDoc, "//page[1]/body/tab/row[2]/cell[1]",
                       u"A2: this row suddenly jumps to the next page");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf166181)
{
    // Given a document with a table with a merged cell, which gets split across pages:
    createSwDoc("tdf166181.fodt");
    CPPUNIT_ASSERT_EQUAL(2, getPages());

    // Make sure that the follow cell has the correct height
    auto pXmlDoc = parseLayoutDump();
    assertXPath(pXmlDoc, "//page", 2);
    assertXPath(pXmlDoc, "//page[2]/body/tab", 1);
    assertXPath(pXmlDoc, "//page[2]/body/tab/row", 2);
    assertXPath(pXmlDoc, "//page[2]/body/tab/row[1]/infos/bounds", "height", u"0");
    // Check that the first cell of the row is a follow (it has a precede)
    (void)getXPath(pXmlDoc, "//page[2]/body/tab/row[1]/cell[1]", "precede");
    // Check that it is merged
    assertXPath(pXmlDoc, "//page[2]/body/tab/row[1]/cell[1]", "rowspan", u"2");
    // Get cell height
    OUString height = getXPath(pXmlDoc, "//page[2]/body/tab/row[1]/cell[1]/infos/bounds", "height");
    // Without a fix, this was greater than 16000; it is expected around 400
    CPPUNIT_ASSERT_LESS(sal_Int32(500), height.toInt32());
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf167526)
{
    // Given a document with a floating table, immediately followed by a normal table:
    createSwDoc("tdf167526.docx");

    // check layout
    {
        // Make sure that the second node is a dummy node, and its line has height of 0
        auto pXmlDoc = parseLayoutDump();
        assertXPath(pXmlDoc, "//page['pass 1']", 1);
        assertXPath(pXmlDoc, "//body['pass 1']/txt[2]/anchored", 1);
        assertXPath(pXmlDoc, "//body['pass 1']/txt[2]/SwParaPortion/SwLineLayout", "height", u"0");
    }

    // DOCX roundtrip:
    saveAndReload(TestFilter::DOCX);

    // check layout
    {
        // Make sure that the second node is a dummy node, and its line has height of 0
        auto pXmlDoc = parseLayoutDump();
        assertXPath(pXmlDoc, "//page['pass 2']", 1);
        assertXPath(pXmlDoc, "//body['pass 2']/txt[2]/anchored", 1);
        // Without a fix, this would fail, because the paragraph was visible after reload;
        // there were two SwLineLayout elements, each with non-0 height.
        assertXPath(pXmlDoc, "//body['pass 2']/txt[2]/SwParaPortion/SwLineLayout", "height", u"0");
    }

    // check DOCX export
    {
        auto pXmlDoc = parseExport(u"word/document.xml"_ustr);
        assertXPathNodeName(pXmlDoc, "/w:document/w:body/*[1]", "p");
        assertXPathNodeName(pXmlDoc, "/w:document/w:body/*[2]", "tbl");
        // Without a fix, this would fail, because the third element was the wrongly emitted <w:p>
        assertXPathNodeName(pXmlDoc, "/w:document/w:body/*[3]", "tbl");
    }

    // tdf#167535: check line numbering; the dummy node must not interfere with it
    {
        // Dump the rendering of the first page as an XML file.
        SwDocShell* pShell = getSwDocShell();
        std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
        MetafileXmlDump dumper;
        xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);
        // Without a fix, this was 3
        assertXPathContent(
            pXmlDoc, "/metafile/push/push/push/textarray[@index=0 and @length=1][2]/text", u"2");
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf167540)
{
    // Given a document with a paragraph with a hard line break followed by nothing (tdf#167538);
    // and two floating tables with an empty paragraph between them (tdf#167540). The strings in
    // paragraphs all have different lengths, to allow XPath matching:
    createSwDoc("tdf167540.docx");

    // check line numbering
    auto verify_me = [this]() {
        // Dump the rendering of the first page as an XML file.
        SwDocShell* pShell = getSwDocShell();
        std::shared_ptr<GDIMetaFile> xMetaFile = pShell->GetPreviewMetaFile();
        MetafileXmlDump dumper;
        xmlDocUniquePtr pXmlDoc = dumpAndParse(dumper, *xMetaFile);

        // line 1

        assertXPathContent(pXmlDoc, "//textarray[@length=1][1]/text", u"1");
        auto ln1_y = getXPath(pXmlDoc, "//textarray[@length=1][1]", "y");

        assertXPathContent(pXmlDoc, "//textarray[@length=4]/text", u"Text\n");
        auto text1_y = getXPath(pXmlDoc, "//textarray[@length=4]", "y");

        // line numbering Y coordinate is the same as its line
        CPPUNIT_ASSERT_EQUAL(text1_y, ln1_y);

        // line 2 (empty)

        assertXPathContent(pXmlDoc, "//textarray[@length=1][2]/text", u"2");
        auto ln2_y = getXPath(pXmlDoc, "//textarray[@length=1][2]", "y");

        // line numbering for line 2 is indeed lower than for line 1
        CPPUNIT_ASSERT_GREATER(ln1_y.toInt32(), ln2_y.toInt32());

        // first floating table

        assertXPathContent(pXmlDoc, "//textarray[@length=20]/text", u"First floating table");
        auto table1_y = getXPath(pXmlDoc, "//textarray[@length=20]", "y");

        // the first floating table is indeed lower than line numbering for line 2
        CPPUNIT_ASSERT_GREATER(ln2_y.toInt32(), table1_y.toInt32());

        // line 3 (empty)

        assertXPathContent(pXmlDoc, "//textarray[@length=1][3]/text", u"3");
        auto ln3_y = getXPath(pXmlDoc, "//textarray[@length=1][3]", "y");

        // line numbering for line 3 is indeed lower than the first floating table
        CPPUNIT_ASSERT_GREATER(table1_y.toInt32(), ln3_y.toInt32());

        // second floating table

        assertXPathContent(pXmlDoc, "//textarray[@length=21]/text", u"Second floating table");
        auto table2_y = getXPath(pXmlDoc, "//textarray[@length=21]", "y");

        // the second floating table is indeed lower than line numbering for line 3
        CPPUNIT_ASSERT_GREATER(ln3_y.toInt32(), table2_y.toInt32());

        // Inline table

        assertXPathContent(pXmlDoc, "//textarray[@length=14]/text", u"A normal table");
        auto table3_y = getXPath(pXmlDoc, "//textarray[@length=14]", "y");

        // the inline table is indeed lower than second floating table
        CPPUNIT_ASSERT_GREATER(table2_y.toInt32(), table3_y.toInt32());

        // line 4

        assertXPathContent(pXmlDoc, "//textarray[@length=1][4]/text", u"4");
        auto ln4_y = getXPath(pXmlDoc, "//textarray[@length=1][4]", "y");

        assertXPathContent(pXmlDoc, "//textarray[@length=9]/text", u"More text");
        auto text4_y = getXPath(pXmlDoc, "//textarray[@length=9]", "y");

        // line numbering Y coordinate is the same as its line
        CPPUNIT_ASSERT_EQUAL(text4_y, ln4_y);
    };

    verify_me();

    // DOCX roundtrip:
    saveAndReload(TestFilter::DOCX);

    verify_me();
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf167541)
{
    // Without the fix, the following call would hang on an infinite loop
    createSwDoc("tdf167541.fodt");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf129808CompatFlagSet)
{
    // tdf#129808: This document contains an embedded font that advertises CP950 coverage. When
    // Word-compatible CJK metrics are enabled, the line height should be scaled higher.
    createSwDoc("tdf129808-flag-set.fodt");
    auto pXmlDoc = parseLayoutDump();

    auto nHeight1 = getXPath(pXmlDoc, "//txt[1]/SwParaPortion/SwLineLayout", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(1450), nHeight1);
    CPPUNIT_ASSERT_LESS(sal_Int32(1460), nHeight1);

    auto nHeight2 = getXPath(pXmlDoc, "//txt[2]/SwParaPortion/SwLineLayout", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(1450), nHeight2);
    CPPUNIT_ASSERT_LESS(sal_Int32(1460), nHeight2);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf129808CompatFlagUnset)
{
    // tdf#129808: This test is identical to the previous test, but with the compatibility flag
    // unset. This test should pass with or without the fix, but ensures the new behavior won't
    // be applied unintentionally in the future.
    createSwDoc("tdf129808-flag-unset.fodt");
    auto pXmlDoc = parseLayoutDump();

    auto nHeight1 = getXPath(pXmlDoc, "//txt[1]/SwParaPortion/SwLineLayout", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(1140), nHeight1);
    CPPUNIT_ASSERT_LESS(sal_Int32(1150), nHeight1);

    auto nHeight2 = getXPath(pXmlDoc, "//txt[2]/SwParaPortion/SwLineLayout", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(1140), nHeight2);
    CPPUNIT_ASSERT_LESS(sal_Int32(1150), nHeight2);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf168116)
{
    // Given a paragraph with a line break immediately followed by a hidden paragraph mark:
    createSwDoc("hidden-para-before-table.rtf");
    auto pXmlDoc = parseLayoutDump();

    assertXPath(pXmlDoc, "//body");
    assertXPathChildren(pXmlDoc, "//body", 4);
    // It must have a "merged" data; without the fix, this failed:
    assertXPath(pXmlDoc, "//body/txt[1]/merged");
    // It must only have one line; without the fix, there were two (the second empty):
    assertXPath(pXmlDoc, "//body/txt[1]/SwParaPortion/SwLineLayout", 1);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf72341GrowAllScripts)
{
    createSwDoc();

    auto* pWrtShell = getSwDocShell()->GetWrtShell();

    pWrtShell->Insert(u"AAAAאאאא"_ustr);
    pWrtShell->Left(SwCursorSkipMode::Chars, /*bSelect*/ false, 7, /*bBasicCall*/ false);
    pWrtShell->Right(SwCursorSkipMode::Chars, /*bSelect*/ true, 2, /*bBasicCall*/ false);

    for (size_t i = 0; i < 4; ++i)
    {
        dispatchCommand(mxComponent, u".uno:Grow"_ustr, {});
    }

    pWrtShell->Left(SwCursorSkipMode::Chars, /*bSelect*/ false, 1, /*bBasicCall*/ false);
    pWrtShell->Insert(u"אאאא"_ustr);

    auto pXmlDoc = parseLayoutDump();

    auto nWidthAlephResized
        = getXPath(pXmlDoc, "//SwLineLayout/SwMultiPortion[1]", "width").toInt32();
    auto nWidthAlephInitial
        = getXPath(pXmlDoc, "//SwLineLayout/SwMultiPortion[2]", "width").toInt32();

    CPPUNIT_ASSERT_GREATER(nWidthAlephInitial, nWidthAlephResized);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf168858)
{
    // Both test documents have no content outside of hidden sections.
    // Each test checks, that the document loads OK, sections in the document load with correct
    // conditions, the layout has nothing except a section on a single page, and the section is
    // hidden (height is 0). The test repeats after save-and-reload. FODT format is used for save,
    // because for ODT, the schema check fails for unknown "text:is-hidden" attribute in section
    // (used as cache; written in XMLSectionExport::ExportRegularSectionStart; handled on import
    // in XMLSectionImportContext::ProcessAttributes).

    // 1. A single section, with a condition that hides it
    {
        createSwDoc("tdf168858_single_hidden_section.fodt");
        Scheduler::ProcessEventsToIdle(); // Recalculate conditions
        auto xTextSections
            = mxComponent.queryThrow<text::XTextSectionsSupplier>()->getTextSections();
        CPPUNIT_ASSERT_EQUAL(sal_Int32(1), xTextSections->getElementNames().getLength());

        auto aSect = xTextSections->getByName(u"abc"_ustr);
        // Before the fix, this was "0", because the condition was set to not hide the last section:
        CPPUNIT_ASSERT_EQUAL(u"\"\" EQ \"\""_ustr, getProperty<OUString>(aSect, u"Condition"_ustr));

        auto pXmlDoc = parseLayoutDump();
        assertXPath(pXmlDoc, "//page['pass 1']", 1);
        assertXPathChildren(pXmlDoc, "//page['pass 1']/body", 2);
        assertXPathNodeName(pXmlDoc, "//page['pass 1']/body/*[1]", "infos");
        assertXPathNodeName(pXmlDoc, "//page['pass 1']/body/*[2]", "section");
        assertXPath(pXmlDoc, "//page['pass 1']/body/section", "formatName", u"abc");
        // Before the fix, this was 276 - i.e., the frame wasn't hidden:
        assertXPath(pXmlDoc, "//page['pass 1']/body/section/infos/bounds", "height", u"0");
    }
    {
        saveAndReload(TestFilter::FODT);
        Scheduler::ProcessEventsToIdle(); // Recalculate conditions
        auto xTextSections
            = mxComponent.queryThrow<text::XTextSectionsSupplier>()->getTextSections();
        CPPUNIT_ASSERT_EQUAL(sal_Int32(1), xTextSections->getElementNames().getLength());

        auto aSect = xTextSections->getByName(u"abc"_ustr);
        CPPUNIT_ASSERT_EQUAL(u"\"\" EQ \"\""_ustr, getProperty<OUString>(aSect, u"Condition"_ustr));

        auto pXmlDoc = parseLayoutDump();
        assertXPath(pXmlDoc, "//page['pass 2']", 1);
        assertXPathChildren(pXmlDoc, "//page['pass 2']/body", 2);
        assertXPathNodeName(pXmlDoc, "//page['pass 2']/body/*[1]", "infos");
        assertXPathNodeName(pXmlDoc, "//page['pass 2']/body/*[2]", "section");
        assertXPath(pXmlDoc, "//page['pass 2']/body/section", "formatName", u"abc");
        assertXPath(pXmlDoc, "//page['pass 2']/body/section/infos/bounds", "height", u"0");
    }

    // 2. A conditionally hidden section inside another hidden section
    {
        createSwDoc("tdf168858_nested_hidden_section.fodt");
        Scheduler::ProcessEventsToIdle(); // Recalculate conditions
        auto xTextSections
            = mxComponent.queryThrow<text::XTextSectionsSupplier>()->getTextSections();
        CPPUNIT_ASSERT_EQUAL(sal_Int32(2), xTextSections->getElementNames().getLength());

        auto aSect = xTextSections->getByName(u"abc"_ustr);
        CPPUNIT_ASSERT_EQUAL(u"\"\" EQ \"\""_ustr, getProperty<OUString>(aSect, u"Condition"_ustr));
        aSect = xTextSections->getByName(u"def"_ustr);
        // Before the fix, this was "0", because the condition was set to not hide the last section:
        CPPUNIT_ASSERT_EQUAL(u"\"\" EQ \"\""_ustr, getProperty<OUString>(aSect, u"Condition"_ustr));

        auto pXmlDoc = parseLayoutDump();
        assertXPath(pXmlDoc, "//page['pass 3']", 1);
        assertXPathChildren(pXmlDoc, "//page['pass 3']/body", 2);
        assertXPathNodeName(pXmlDoc, "//page['pass 3']/body/*[1]", "infos");
        assertXPathNodeName(pXmlDoc, "//page['pass 3']/body/*[2]", "section");
        // The inner section replaces the whole area of the outer section, so there's no frame for
        // the outer section:
        assertXPath(pXmlDoc, "//page['pass 3']/body/section", "formatName", u"def");
        // Even before the fix, this was hidden:
        assertXPath(pXmlDoc, "//page['pass 3']/body/section/infos/bounds", "height", u"0");
    }
    {
        saveAndReload(TestFilter::FODT);
        Scheduler::ProcessEventsToIdle(); // Recalculate conditions
        auto xTextSections
            = mxComponent.queryThrow<text::XTextSectionsSupplier>()->getTextSections();
        CPPUNIT_ASSERT_EQUAL(sal_Int32(2), xTextSections->getElementNames().getLength());

        auto aSect = xTextSections->getByName(u"abc"_ustr);
        CPPUNIT_ASSERT_EQUAL(u"\"\" EQ \"\""_ustr, getProperty<OUString>(aSect, u"Condition"_ustr));
        aSect = xTextSections->getByName(u"def"_ustr);
        CPPUNIT_ASSERT_EQUAL(u"\"\" EQ \"\""_ustr, getProperty<OUString>(aSect, u"Condition"_ustr));

        auto pXmlDoc = parseLayoutDump();
        assertXPath(pXmlDoc, "//page['pass 4']", 1);
        assertXPathChildren(pXmlDoc, "//page['pass 4']/body", 2);
        assertXPathNodeName(pXmlDoc, "//page['pass 4']/body/*[1]", "infos");
        assertXPathNodeName(pXmlDoc, "//page['pass 4']/body/*[2]", "section");
        // The inner section replaces the whole area of the outer section, so there's no frame for
        // the outer section:
        assertXPath(pXmlDoc, "//page['pass 4']/body/section", "formatName", u"def");
        assertXPath(pXmlDoc, "//page['pass 4']/body/section/infos/bounds", "height", u"0");
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf164718)
{
    // A 13-line field as the lone content of a paragraph in a two-column section:
    createSwDoc("multiline-field-in-columnar-section.fodt");
    Scheduler::ProcessEventsToIdle();
    auto pXmlDoc = parseLayoutDump();

    // The problem was, that layout refused to split the multiline field across the columns, which
    // resulted in a layout loop, ending up in a new page creation, where that section had height
    // equal to zero (invisible). After that, the split was undone by widow control, because the
    // zero-length rest lines of the field were not counted as lines.

    // Check that there is only one page, it has the leading paragraph, the section, and the
    // trailing paragraph:
    assertXPath(pXmlDoc, "//page", 1);
    assertXPathChildren(pXmlDoc, "//page/body", 4);
    assertXPathNodeName(pXmlDoc, "//page/body/*[1]", "infos");
    assertXPathNodeName(pXmlDoc, "//page/body/*[2]", "txt");
    assertXPathNodeName(pXmlDoc, "//page/body/*[3]", "section");
    assertXPathNodeName(pXmlDoc, "//page/body/*[4]", "txt");

    // Check that the field is split across both columns: lines 8-13 are in column 2, and the
    // section is about 7 lines (1932) high, not 13 lines (3588):
    assertXPath(pXmlDoc, "//page/body/section/column[1]/body/txt", 1);
    assertXPath(pXmlDoc, "//page/body/section/column[2]/body/txt", 1);
    auto height = getXPath(pXmlDoc, "//page/body/section/infos/bounds", "height").toInt32();
    CPPUNIT_ASSERT_GREATER(sal_Int32(1500), height);
    CPPUNIT_ASSERT_LESS(sal_Int32(2500), height);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf164718_lineBreaks)
{
    // A 9-line string variable (lines separated by line feeds) in a two-column section, with
    // widows and orphans of 2:
    createSwDoc("multiline-field-linebreaks.fodt");
    Scheduler::ProcessEventsToIdle();
    auto pXmlDoc = parseLayoutDump();

    // Without the fix, the whole field stayed in column 1:
    assertXPath(pXmlDoc, "//page", 1);
    assertXPath(pXmlDoc, "//page/body/section/column[1]/body/txt", 1);
    assertXPath(pXmlDoc, "//page/body/section/column[2]/body/txt", 1);
    // 5 lines in column 1, 4 in column 2:
    assertXPath(pXmlDoc, "//page/body/section/column[1]/body/txt/SwParaPortion/SwLineLayout", 5);
    assertXPath(pXmlDoc, "//page/body/section/column[2]/body/txt/SwParaPortion/SwLineLayout", 4);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf164718_mixedLines)
{
    // An 8-line string variable with several characters per line, one of them long enough to
    // wrap, in a two-column section, with widows and orphans of 2:
    createSwDoc("multiline-field-mixed-lines.fodt");
    Scheduler::ProcessEventsToIdle();
    auto pXmlDoc = parseLayoutDump();

    assertXPath(pXmlDoc, "//page", 1);
    assertXPath(pXmlDoc, "//page/body/section/column[1]/body/txt", 1);
    assertXPath(pXmlDoc, "//page/body/section/column[2]/body/txt", 1);
    // 5 lines in column 1, 4 in column 2; the column break falls inside the wrapped fifth line:
    OString aCol1 = "//page/body/section/column[1]/body/txt/SwParaPortion/SwLineLayout"_ostr;
    OString aCol2 = "//page/body/section/column[2]/body/txt/SwParaPortion/SwLineLayout"_ostr;
    assertXPath(pXmlDoc, aCol1, 5);
    assertXPath(pXmlDoc, aCol2, 4);
    assertXPath(pXmlDoc, aCol1 + "[1]/SwFieldPortion", "expand", u"Line one");
    assertXPath(pXmlDoc, aCol1 + "[5]/SwFieldPortion", "expand",
                u"This fifth line is long enough that it has to wrap ");
    assertXPath(pXmlDoc, aCol2 + "[1]/SwFieldPortion", "expand", u"inside the narrow column");
    assertXPath(pXmlDoc, aCol2 + "[4]/SwFieldPortion", "expand", u"Line eight");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf169158)
{
    // A table with vertical-align="middle" in a fly anchored at page:
    createSwDoc("cell-vertical-align-middle-in-fly-at-page.fodt");
    auto pXmlDoc = parseLayoutDump(); // Intentionally no Scheduler::ProcessEventsToIdle

    // The problem was, that layout didn't reformat objects at page, when the non-default
    // alignment invalidated cells in tables in at-page fly in SwContentNotify::ImplDestroy.

    // Check that the text in the cell is aligned correctly:
    assertXPath(pXmlDoc, "//page", 1);
    assertXPath(pXmlDoc, "//page/anchored/fly/tab/row/cell/infos/bounds", "top", u"1985");
    // Without the fix, this would fail with
    // - Expected: 2698
    // - Actual  : 2043
    assertXPath(pXmlDoc, "//page/anchored/fly/tab/row/cell/txt/infos/bounds", "top", u"2698");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testCool16036)
{
    // A nearly frame-wide rectangle, centered on its (empty) paragraph, inside a fixed-size
    // text frame anchored at the page. The rectangle cannot fit centered, so formatting the
    // paragraph did not converge and SwLayAction::InternalAction looped forever: opening the
    // document hung.
    createSwDoc("vert-centered-object-in-fly-at-page.fodt");
    // Without the fix, laying out the document (below) never returns.
    auto pXmlDoc = parseLayoutDump();

    // The at-page text frame is laid out at its fixed size, with the rectangle nested in it.
    assertXPath(pXmlDoc, "//page", 1);
    assertXPath(pXmlDoc, "//page/anchored/fly", 1);
    assertXPath(pXmlDoc, "//page/anchored/fly/infos/bounds", "height", u"2835");
    assertXPath(pXmlDoc, "//page/anchored/fly/section/txt/anchored/SwAnchoredDrawObject", 1);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf104020)
{
    // A nearly frame-wide rectangle set to centre on its anchor paragraph, which has no text
    // of its own, in a fixed-height text frame anchored at the page. The rectangle is centred
    // on the frame's content area, as centring it on the paragraph would be circular: the
    // paragraph's height comes from the rectangle's own wrap.
    createSwDoc("object-centred-in-fixed-height-fly.fodt");
    auto pXmlDoc = parseLayoutDump();

    // The paragraph keeps the whole frame; without the fix it collapsed to a single line (276).
    assertXPath(pXmlDoc, "//page/anchored/fly/infos/bounds", "top", u"567");
    assertXPath(pXmlDoc, "//page/anchored/fly/infos/bounds", "height", u"2835");
    assertXPath(pXmlDoc, "//page/anchored/fly/section/txt/infos/bounds", "height", u"2835");

    // The rectangle sits centred in the frame, (2835 - 2384) / 2 below its top; without the
    // fix it was centred on the single line instead, which put it above the frame.
    assertXPath(pXmlDoc, "//page/anchored/fly/section/txt/anchored/SwAnchoredDrawObject/bounds",
                "height", u"2384");
    assertXPath(pXmlDoc, "//page/anchored/fly/section/txt/anchored/SwAnchoredDrawObject/bounds",
                "top", u"792");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf104020FittingObject)
{
    // The same fixed-height text frame, but with a rectangle that fits in its anchor
    // paragraph: it is placed in the paragraph as asked, next to the paragraph's own line,
    // and the paragraph stays a single line. Nothing is circular here, so this must keep
    // using the paragraph as the alignment area.
    createSwDoc("object-fitting-empty-paragraph-in-fly.fodt");
    auto pXmlDoc = parseLayoutDump();

    assertXPath(pXmlDoc, "//page/anchored/fly/infos/bounds", "top", u"567");
    assertXPath(pXmlDoc, "//page/anchored/fly/section/txt/infos/bounds", "height", u"276");

    // Centred on the single line, (276 - 173) / 2 below the paragraph's top - not on the
    // whole frame, which would put it near the frame's middle instead.
    assertXPath(pXmlDoc, "//page/anchored/fly/section/txt/anchored/SwAnchoredDrawObject/bounds",
                "height", u"173");
    assertXPath(pXmlDoc, "//page/anchored/fly/section/txt/anchored/SwAnchoredDrawObject/bounds",
                "top", u"619");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf169320)
{
    // A list with one item hidden by usual hidden text property, and another having a hidden
    // paragraph mark:
    createSwDoc("tdf169320-list_item_hidden_by_para_mark.fodt");
    auto pXmlDoc = parseLayoutDump();
    assertXPath(pXmlDoc, "//page", 1);
    assertXPath(pXmlDoc, "//body/txt", 5);

    // This node has a redline
    CPPUNIT_ASSERT_GREATER(sal_Int32(200),
                           getXPath(pXmlDoc, "//body/txt[1]/infos/bounds", "height").toInt32());
    assertXPath(pXmlDoc, "//body/txt[1]/merged", 0);
    assertXPathContent(pXmlDoc, "//body/txt[1]", u"item 1 added text");
    assertXPath(pXmlDoc, "//body/txt[1]//SwFieldPortion", "expand", u"1.");

    CPPUNIT_ASSERT_GREATER(sal_Int32(200),
                           getXPath(pXmlDoc, "//body/txt[2]/infos/bounds", "height").toInt32());
    assertXPath(pXmlDoc, "//body/txt[2]/merged", 0);
    assertXPathContent(pXmlDoc, "//body/txt[2]", u"item 2");
    assertXPath(pXmlDoc, "//body/txt[2]//SwFieldPortion", "expand", u"2.");

    // Compare the layout structure of a hidden paragraph (txt[3]) to a merged one (txt[5]).

    // This is hidden:
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0),
                         getXPath(pXmlDoc, "//body/txt[3]/infos/bounds", "height").toInt32());
    assertXPath(pXmlDoc, "//body/txt[3]/merged", 0);
    assertXPathContent(pXmlDoc, "//body/txt[3]", u"item 3");
    assertXPath(pXmlDoc, "//body/txt[3]//SwFieldPortion", 0);

    CPPUNIT_ASSERT_GREATER(sal_Int32(200),
                           getXPath(pXmlDoc, "//body/txt[4]/infos/bounds", "height").toInt32());
    assertXPath(pXmlDoc, "//body/txt[4]/merged", 0);
    assertXPathContent(pXmlDoc, "//body/txt[4]", u"subitem 3.1");
    assertXPath(pXmlDoc, "//body/txt[4]//SwFieldPortion", "expand", u"3.1");

    // This is merged:
    CPPUNIT_ASSERT_GREATER(sal_Int32(200),
                           getXPath(pXmlDoc, "//body/txt[5]/infos/bounds", "height").toInt32());
    assertXPath(pXmlDoc, "//body/txt[5]/merged", 1);
    assertXPathContent(pXmlDoc, "//body/txt[5]", u"item 4subitem 4.1");
    // Without a fix, this was "3.2[4.1]":
    assertXPath(pXmlDoc, "//body/txt[5]//SwFieldPortion", "expand", u"4.1");
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testTdf33687)
{
    // A ToC in linked frames, on a First Page followed by a Standard page:
    createSwDoc("ToC-in-linked-frame-on-first-page-style.fodt");
    auto pXmlDoc = parseLayoutDump();
    assertXPath(pXmlDoc, "//page", 5);

    // Check that the ToC text flows to the second frame:

    // First frame
    assertXPath(pXmlDoc, "//page[1]/body/txt[2]/anchored/fly", "formatName", u"Frame1");
    assertXPath(pXmlDoc, "//page[1]/body/txt[2]/anchored/fly/section", 2);
    assertXPath(pXmlDoc, "//page[1]/body/txt[2]/anchored/fly/section[1]", "formatName",
                u"TOC1_Head");
    assertXPath(pXmlDoc, "//page[1]/body/txt[2]/anchored/fly/section[1]/txt", 1);
    // Before the fix, this was 60:
    assertXPath(pXmlDoc, "//page[1]/body/txt[2]/anchored/fly/section[2]/txt", 36);

    // Second frame
    assertXPath(pXmlDoc, "//page[1]/body/txt[3]/anchored/fly", "formatName", u"Frame2");
    // Before the fix, there was no section in this frame, because ToC didn't flow here:
    assertXPath(pXmlDoc, "//page[1]/body/txt[3]/anchored/fly/section", 1);
    assertXPath(pXmlDoc, "//page[1]/body/txt[3]/anchored/fly/section[1]/txt", 24);
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacEmptyRowContinuation)
{
    const OUString aEnv = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bPrevious = osl_getEnvironment(aEnv.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bPrevious)
            osl_setEnvironment(aEnv.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aEnv.pData);
    });
    for (bool bMac : {false, true})
    {
        const OUString aValue = bMac ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aEnv.pData, aValue.pData);
        for (bool bAfter : {false, true})
        {
            utl::TempFileNamed aFile;
            aFile.EnableKillingFile();
            makeWordMacPackage(aFile, false, true);
            {
                auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                    ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
                auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
                const OString aAfter = bAfter ? "480"_ostr : "0"_ostr;
                writeWordMacPart(xWord, u"document.xml"_ustr,
                    R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body>
<w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="12900" w:lineRule="exact"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:t>Spacer</w:t></w:r></w:p>
<w:tbl><w:tblPr><w:tblW w:type="dxa" w:w="8000"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="8000"/></w:tblGrid>
<w:tr><w:trPr><w:tblHeader/></w:trPr><w:tc><w:tcPr><w:tcW w:type="dxa" w:w="8000"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:t>Header</w:t></w:r></w:p></w:tc></w:tr>
<w:tr><w:tc><w:tcPr><w:tcW w:type="dxa" w:w="8000"/></w:tcPr><w:p><w:pPr><w:widowControl w:val="0"/><w:spacing w:before="0" w:after=")xml"_ostr + aAfter + R"xml(" w:line="480" w:lineRule="exact"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:t>Body</w:t></w:r></w:p></w:tc></w:tr></w:tbl>
<w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr);
                writeWordMacPart(xWord, u"settings.xml"_ustr,
                    R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:compat><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val="15"/></w:compat></w:settings>)xml"_ostr);
                uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
                uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
            }
            for (int nRound = 0; nRound < 2; ++nRound)
            {
                if (nRound == 0)
                    loadFromURL(aFile.GetURL());
                else
                    saveAndReload(TestFilter::DOCX);
                auto pLayout = parseLayoutDump();
                // A row continuation must contain actual content, not only a
                // zero-length follow frame carrying paragraph trailing space.
                assertXPath(pLayout, "/root/page[1]/body/tab", bMac && bAfter ? 0 : 1);
                assertXPath(pLayout, "/root/page[2]/body/tab", bAfter ? 1 : 0);
                if (bAfter)
                    assertXPath(pLayout, "/root/page[2]/body/tab/row[2]/cell/txt/SwParaPortion/SwLineLayout[1]",
                        "length", bMac ? u"4" : u"0");
            }
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacCjkHangingColon)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr, u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr, u"COCOLIAN_WORD_MAC_AUTOFIT"_ustr,
        u"COCOLIAN_WORD_MAC_GRID_BOTTOM"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
        const OUString aOn = u"1"_ustr;
        osl_setEnvironment(rName.pData, aOn.pData);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    for (bool bDictionaryWord : {false, true})
    for (bool bHanging : {false, true})
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            const OString aHanging = bHanging ? "1"_ostr : "0"_ostr;
            const OString aLastWord = bDictionaryWord ? "推进"_ostr : "甲甲"_ostr;
            writeWordMacPart(xWord, u"document.xml"_ustr,
                R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:line="360" w:lineRule="auto"/><w:ind w:firstLine="480"/><w:overflowPunct w:val=")xml"_ostr + aHanging + R"xml("/></w:pPr>
<w:r><w:rPr><w:rFonts w:ascii="SimSun" w:hAnsi="SimSun" w:eastAsia="SimSun"/><w:sz w:val="24"/></w:rPr><w:t>甲甲1234甲甲甲甲甲甲甲甲甲甲甲甲</w:t></w:r>
<w:r><w:rPr><w:rFonts w:hint="eastAsia"/><w:sz w:val="24"/><w:lang w:eastAsia="zh"/></w:rPr><w:t>，</w:t></w:r>
<w:r><w:rPr><w:rFonts w:ascii="SimSun" w:hAnsi="SimSun" w:eastAsia="SimSun"/><w:sz w:val="24"/></w:rPr><w:t>甲甲甲甲甲甲甲甲甲甲甲甲甲甲甲甲甲甲，甲甲甲甲甲甲甲甲甲甲甲甲，甲甲甲甲甲甲甲甲甲甲甲甲甲甲甲)xml"_ostr + aLastWord + R"xml(：</w:t></w:r>
</w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:right="1803" w:bottom="1440" w:left="1803" w:header="720" w:footer="720"/><w:docGrid w:linePitch="360" w:charSpace="0"/></w:sectPr></w:body></w:document>)xml"_ostr);
            writeWordMacPart(xWord, u"styles.xml"_ustr,
                R"xml(<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:docDefaults><w:rPrDefault><w:rPr><w:rFonts w:ascii="SimSun" w:hAnsi="SimSun" w:eastAsia="SimSun"/><w:sz w:val="24"/><w:lang w:val="en-US" w:eastAsia="en-US"/></w:rPr></w:rPrDefault><w:pPrDefault/></w:docDefaults><w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/><w:pPr><w:spacing w:after="200" w:line="360" w:lineRule="auto"/></w:pPr><w:rPr><w:rFonts w:ascii="SimSun" w:hAnsi="SimSun" w:eastAsia="SimSun"/><w:sz w:val="24"/><w:lang w:val="en-US" w:eastAsia="en-US"/></w:rPr></w:style></w:styles>)xml"_ostr);
            writeWordMacPart(xWord, u"settings.xml"_ustr,
                R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:compat><w:doNotExpandShiftReturn/><w:doNotWrapTextWithPunct/><w:doNotUseEastAsianBreakRules/><w:useFELayout/><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val="14"/></w:compat></w:settings>)xml"_ostr);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        for (int nRound = 0; nRound < 2; ++nRound)
        {
            if (nRound == 0)
                loadFromURL(aFile.GetURL());
            else
                saveAndReload(TestFilter::DOCX);
            // A closing CJK colon may hang outside the body text boundary;
            // moving its preceding dictionary word adds an unwanted third line.
            assertXPath(parseLayoutDump(), "/root/page/body/txt/SwParaPortion/SwLineLayout",
                bHanging && bDictionaryWord ? 2 : 3);
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacAutoFitMissingColumnPreferences)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_AUTOFIT"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aNames[1].pData, aOn.pData);
    for (bool bMac : {false, true})
    for (int nPreference : {0, 1, 2, 3, 4})
    {
        const OUString aMode = bMac ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aNames[0].pData, aMode.pData);
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            OString aTable = R"xml(<w:tbl><w:tblPr><w:tblW w:w="5000" w:type="pct"/></w:tblPr>)xml"_ostr;
            if (nPreference == 4)
                aTable = R"xml(<w:tbl><w:tblPr><w:tblW w:w="5000" w:type="pct"/><w:tblLayout w:type="fixed"/></w:tblPr>)xml"_ostr;
            if (nPreference == 1)
                aTable += R"xml(<w:tblGrid><w:gridCol w:w="2076"/><w:gridCol w:w="2076"/><w:gridCol w:w="2076"/><w:gridCol w:w="2076"/></w:tblGrid>)xml"_ostr;
            for (int nRow = 0; nRow < 2; ++nRow)
            {
                aTable += "<w:tr>"_ostr;
                for (int nCol = 0; nCol < 4; ++nCol)
                {
                    aTable += "<w:tc><w:tcPr>"_ostr;
                    if (nPreference == 2)
                        aTable += R"xml(<w:tcW w:w="1250" w:type="pct"/>)xml"_ostr;
                    if (nPreference == 3)
                        aTable += R"xml(<w:tcW w:w="2076" w:type="dxa"/>)xml"_ostr;
                    aTable += R"xml(</w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="SimSun" w:hAnsi="SimSun" w:eastAsia="SimSun"/><w:sz w:val="24"/></w:rPr><w:t>)xml"_ostr;
                    aTable += nRow == 1 && nCol == 0 ? "甲乙丙丁戊己庚辛壬癸子丑"_ostr : "甲乙丙丁"_ostr;
                    aTable += "</w:t></w:r></w:p></w:tc>"_ostr;
                }
                aTable += "</w:tr>"_ostr;
            }
            aTable += "</w:tbl>"_ostr;
            writeWordMacPart(xWord, u"document.xml"_ustr,
                R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body>)xml"_ostr + aTable
                + R"xml(<w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1800" w:right="1800"/></w:sectPr></w:body></w:document>)xml"_ostr);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        for (int nRound = 0; nRound < 2; ++nRound)
        {
            if (nRound == 0)
                loadFromURL(aFile.GetURL());
            else
                saveAndReload(TestFilter::DOCX);
            auto pLayout = parseLayoutDump();
            const sal_Int32 nFirst = getXPath(pLayout,
                "/root/page/body/tab/row[1]/cell[1]/infos/bounds", "width").toInt32();
            const sal_Int32 nSecond = getXPath(pLayout,
                "/root/page/body/tab/row[1]/cell[2]/infos/bounds", "width").toInt32();
            const bool bContentFit = bMac && (nPreference == 0 || nPreference == 1);
            if (bContentFit)
                CPPUNIT_ASSERT_GREATER(nSecond * 3 / 2, nFirst);
            else
                CPPUNIT_ASSERT(std::abs(nFirst - nSecond) < 5);
            assertXPath(pLayout,
                "/root/page/body/tab/row[2]/cell[1]/txt/SwParaPortion/SwLineLayout",
                bContentFit ? 1 : 2);
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacNaturalLinePitch)
{
    const OUString aProfile = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    const OUString aClassic = u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr;
    OUString aPrevious, aPreviousClassic;
    const bool bHadProfile = osl_getEnvironment(aProfile.pData, &aPrevious.pData) == osl_Process_E_None;
    const bool bHadClassic = osl_getEnvironment(aClassic.pData, &aPreviousClassic.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadProfile)
            osl_setEnvironment(aProfile.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aProfile.pData);
        if (bHadClassic)
            osl_setEnvironment(aClassic.pData, aPreviousClassic.pData);
        else
            osl_clearEnvironment(aClassic.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aProfile.pData, aOn.pData);
    osl_setEnvironment(aClassic.pData, aOn.pData);
    // Measured Mac Word long-paragraph baselines, independently of Writer's
    // font-height formula. Check accumulated pitch and DOCX round trips.
    struct PitchCase
    {
        const char* pFamily;
        sal_Int32 nHalfPoints;
        sal_Int32 nLine;
        double fWordPitch;
        double fTolerance;
    };
    const PitchCase aCases[] = {
        { "SimSun", 48, 360, 46.68, 0.04 },
        { "FangSong_GB2312", 32, 240, 20.76, 0.025 },
        { "KaiTi_GB2312", 20, 276, 14.92, 0.025 },
        { "DengXian", 28, 276, 21.8021, 0.025 },
        { "Microsoft YaHei", 24, 276, 23.68, 0.04 },
        { "Microsoft YaHei", 48, 276, 47.34, 0.025 },
    };
    for (const auto& rCase : aCases)
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            OString aRun = "<w:rPr><w:rFonts w:ascii=\"FONT\" w:hAnsi=\"FONT\" w:eastAsia=\"FONT\"/><w:sz w:val=\"SIZE\"/></w:rPr>"_ostr;
            aRun = aRun.replaceAll("FONT"_ostr, OString(rCase.pFamily))
                .replaceAll("SIZE"_ostr, OString::number(rCase.nHalfPoints));
            OString aLines;
            for (int i = 0; i < 13; ++i)
                aLines += (i ? "<w:br/>"_ostr : OString()) + "<w:t>甲</w:t>"_ostr;
            OString aXml = "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\"><w:body><w:p><w:pPr><w:snapToGrid w:val=\"0\"/><w:spacing w:before=\"0\" w:after=\"0\" w:line=\""_ostr
                + OString::number(rCase.nLine) + "\" w:lineRule=\"auto\"/>"_ostr + aRun
                + "</w:pPr><w:r>"_ostr + aRun + aLines
                + "</w:r></w:p><w:sectPr><w:pgSz w:w=\"11906\" w:h=\"16838\"/><w:pgMar w:top=\"1440\" w:bottom=\"1440\" w:left=\"1800\" w:right=\"1800\"/></w:sectPr></w:body></w:document>"_ostr;
            writeWordMacPart(xWord, u"document.xml"_ustr, aXml);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        loadFromURL(aFile.GetURL());
        for (bool bRoundTrip : { false, true })
        {
            if (bRoundTrip)
                saveAndReload(TestFilter::DOCX);
            CPPUNIT_ASSERT_EQUAL(1, getPages());
            auto xMeta = getSwDocShell()->GetPreviewMetaFile();
            MetafileXmlDump aDump;
            auto pMeta = dumpAndParse(aDump, *xMeta);
            assertXPath(pMeta, "//textarray", 13);
            const double fFirst = getXPath(pMeta, "(//textarray)[1]", "y").toDouble();
            const double fLast = getXPath(pMeta, "(//textarray)[13]", "y").toDouble();
            // Writer preview metafiles use twips for these text baselines.
            const double fPitch = (fLast - fFirst) / 240;
            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(rCase.pFamily,
                rCase.fWordPitch, fPitch, rCase.fTolerance);
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacCjkHangingParenthesis)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr, u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr, u"COCOLIAN_WORD_MAC_AUTOFIT"_ustr,
        u"COCOLIAN_WORD_MAC_GRID_BOTTOM"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
        const OUString aOn = u"1"_ustr;
        osl_setEnvironment(rName.pData, aOn.pData);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    const OUString aTexts[] = {
        u"系统提供文件检索的接口，授权给管理人员（一般由管理组人员来承担）使用，继续核对文件的访问规则和操作记录。"_ustr,
        u"甲甲甲甲甲甲甲甲甲甲甲，甲甲甲甲甲甲甲（甲甲甲甲甲甲甲甲甲甲甲）使用，继续核对文件的访问规则和操作记录。"_ustr,
        u"系统提供文件检索的接口，授权给管理人员甲一般由管理组人员来承担）使用，继续核对文件的访问规则和操作记录。"_ustr,
        u"甲甲甲甲甲甲甲甲甲甲甲，甲甲甲甲甲甲甲甲甲甲甲甲甲甲甲甲甲甲甲：使用，继续核对文件的访问规则和操作记录。"_ustr,
    };
    for (const char* pFamily : {"FangSong_GB2312", "SimSun"})
    for (const char* pAlignment : {"both", "left"})
    for (size_t nPattern = 0; nPattern < std::size(aTexts); ++nPattern)
    for (bool bHanging : {false, true})
    for (bool bKern : {false, true})
    {
        // The extra kerning / Latin-language control has its own Word PDF.
        if (bKern && (nPattern != 0 || std::strcmp(pFamily, "FangSong_GB2312") != 0))
            continue;
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            OString aRun = R"xml(<w:rPr><w:rFonts w:ascii="FONT" w:hAnsi="FONT" w:eastAsia="FONT"/><w:b w:val="0"/><w:kern w:val="KERN"/><w:sz w:val="28"/><w:lang w:val="LANG" w:eastAsia="zh-CN"/></w:rPr>)xml"_ostr;
            aRun = aRun.replaceAll("FONT"_ostr, OString(pFamily))
                .replaceAll("KERN"_ostr, bKern ? "2"_ostr : "0"_ostr)
                .replaceAll("LANG"_ostr, bKern ? "en-US"_ostr : "zh-CN"_ostr);
            OString aXml = R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:wordWrap w:val="0"/><w:autoSpaceDE/><w:autoSpaceDN/><w:adjustRightInd w:val="0"/><w:overflowPunct w:val="HANG"/><w:snapToGrid/><w:spacing w:before="0" w:after="0" w:line="276" w:lineRule="auto"/><w:ind w:firstLine="560"/><w:jc w:val="ALIGN"/>)xml"_ostr
                + aRun + "</w:pPr><w:r>"_ostr + aRun + "<w:t>"_ostr
                + OUStringToOString(aTexts[nPattern], RTL_TEXTENCODING_UTF8)
                + R"xml(</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="12240" w:h="15840"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr;
            aXml = aXml.replaceAll("HANG"_ostr, bHanging ? "1"_ostr : "0"_ostr)
                .replaceAll("ALIGN"_ostr, OString(pAlignment));
            writeWordMacPart(xWord, u"document.xml"_ustr, aXml);
            writeWordMacPart(xWord, u"styles.xml"_ustr,
                R"xml(<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/></w:style></w:styles>)xml"_ostr);
            writeWordMacPart(xWord, u"settings.xml"_ustr,
                R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"/>)xml"_ostr);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        loadFromURL(aFile.GetURL());
        for (bool bRoundTrip : {false, true})
        {
            if (bRoundTrip)
                saveAndReload(TestFilter::DOCX);
            // Actual Word keeps the closing punctuation on the first line
            // when hanging is enabled, including repeated and unpaired text.
            auto pXml = parseLayoutDump();
            assertXPath(pXml, "/root/page/body/txt/SwParaPortion/SwLineLayout", 2);
            const sal_Int32 nFirstLength = bHanging ? 32 : 30;
            assertXPath(pXml, "/root/page/body/txt/SwParaPortion/SwLineLayout[1]",
                "length", OUString::number(nFirstLength));
            assertXPath(pXml, "/root/page/body/txt/SwParaPortion/SwLineLayout[2]",
                "length", OUString::number(aTexts[nPattern].getLength() - nFirstLength));
        }
    }
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacTableWidowControl)
{
    const OUString aEnv = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bPrevious = osl_getEnvironment(aEnv.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bPrevious)
            osl_setEnvironment(aEnv.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aEnv.pData);
    });
    for (bool bMac : {false, true})
    {
        const OUString aValue = bMac ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aEnv.pData, aValue.pData);
        for (int nCompat : {14, 15})
        {
            for (bool bWidows : {false, true})
            {
                utl::TempFileNamed aFile;
                aFile.EnableKillingFile();
                makeWordMacPackage(aFile, false, true);
                {
                    auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                        ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
                    auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
                    const OString aCompat = OString::number(nCompat);
                    const OString aWidow = bWidows ? "1"_ostr : "0"_ostr;
                    writeWordMacPart(xWord, u"document.xml"_ustr,
                        R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body>
    <w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="12500" w:lineRule="exact"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:t>Spacer</w:t></w:r></w:p>
    <w:tbl><w:tblPr><w:tblW w:type="dxa" w:w="8000"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="8000"/></w:tblGrid>
    <w:tr><w:trPr><w:tblHeader/></w:trPr><w:tc><w:tcPr><w:tcW w:type="dxa" w:w="8000"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:t>Header</w:t></w:r></w:p></w:tc></w:tr>
    <w:tr><w:tc><w:tcPr><w:tcW w:type="dxa" w:w="8000"/></w:tcPr><w:p><w:pPr><w:widowControl w:val=")xml"_ostr + aWidow + R"xml("/><w:spacing w:before="0" w:after="0" w:line="480" w:lineRule="exact"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:t>First</w:t><w:br/><w:t>Second</w:t><w:br/><w:t>Third</w:t></w:r></w:p></w:tc></w:tr></w:tbl>
    <w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr);
                    writeWordMacPart(xWord, u"settings.xml"_ustr,
                        R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:compat><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val=")xml"_ostr + aCompat + R"xml("/></w:compat></w:settings>)xml"_ostr);
                    uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
                    uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
                }
                for (int nRound = 0; nRound < 2; ++nRound)
                {
                    if (nRound == 0)
                        loadFromURL(aFile.GetURL());
                    else
                        saveAndReload(TestFilter::DOCX);
                    auto pLayout = parseLayoutDump();
                    // Three lines cannot be split into two groups of at least two.
                    // Word 2013 mode keeps the row with its repeated header on page two;
                    // Word 2010 mode permits splitting even when widows are enabled.
                    assertXPath(pLayout, "/root/page[1]/body/tab", bMac && bWidows && nCompat >= 15 ? 0 : 1);
                    assertXPath(pLayout, "/root/page[2]/body/tab", 1);
                }
            }
        }
    }
}


CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacOversizedFixedTableWidth)
{
    const OUString aEnv = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bPrevious = osl_getEnvironment(aEnv.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bPrevious)
            osl_setEnvironment(aEnv.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aEnv.pData);
    });
    for (bool bMac : {false, true})
    {
        const OUString aValue = bMac ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aEnv.pData, aValue.pData);
        for (const OString& rCellType : {"pct"_ostr, "dxa"_ostr, "none"_ostr})
        {
            utl::TempFileNamed aFile;
            aFile.EnableKillingFile();
            makeWordMacPackage(aFile, false, true);
            {
                auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                    ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
                auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
                OString aCells;
                const sal_Int32 aGrid[] = {1602, 735, 1624, 2288, 3428};
                const sal_Int32 aPercents[] = {827, 379, 839, 1182, 1771};
                for (int i = 0; i < 5; ++i)
                {
                    OString aWidth;
                    if (rCellType != "none")
                        aWidth = "<w:tcW w:type=\""_ostr + rCellType + "\" w:w=\""_ostr
                            + OString::number(rCellType == "pct" ? aPercents[i] : aGrid[i])
                            + "\"/>"_ostr;
                    aCells += "<w:tc><w:tcPr>"_ostr + aWidth
                        + "</w:tcPr><w:p><w:r><w:t>Cell</w:t></w:r></w:p></w:tc>"_ostr;
                }
                writeWordMacPart(xWord, u"document.xml"_ustr,
                    R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="5564" w:type="pct"/><w:tblLayout w:type="fixed"/></w:tblPr><w:tblGrid><w:gridCol w:w="1602"/><w:gridCol w:w="735"/><w:gridCol w:w="1624"/><w:gridCol w:w="2288"/><w:gridCol w:w="3428"/></w:tblGrid><w:tr>)xml"_ostr
                    + aCells + R"xml(</w:tr></w:tbl><w:p/><w:sectPr><w:pgSz w:w="11904" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1800" w:right="1800"/></w:sectPr></w:body></w:document>)xml"_ostr);
                writeWordMacPart(xWord, u"settings.xml"_ustr,
                    R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:compat><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val="15"/></w:compat></w:settings>)xml"_ostr);
                uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
                uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
            }
            for (int nRound = 0; nRound < 2; ++nRound)
            {
                if (!nRound)
                    loadFromURL(aFile.GetURL());
                else
                    saveAndReload(TestFilter::DOCX);
                auto pDump = parseLayoutDump();
                const sal_Int32 nBody = getXPath(pDump,
                    "/root/page/body/infos/prtBounds", "width").toInt32();
                const sal_Int32 nWidth = getXPath(pDump,
                    "/root/page/body/tab/infos/prtBounds", "width").toInt32();
                const sal_Int32 nExpected = bMac ? sal_Int64(nBody) * 5564 / 5000 : nBody;
                const OString aDiagnostic = "celltype="_ostr + rCellType
                    + " mac="_ostr + OString::number(bMac)
                    + " round="_ostr + OString::number(nRound)
                    + " width="_ostr + OString::number(nWidth)
                    + " expected="_ostr + OString::number(nExpected);
                CPPUNIT_ASSERT_MESSAGE(aDiagnostic.getStr(), std::abs(nWidth - nExpected) <= 1);
                if (nRound)
                    assertXPath(parseExport(u"word/document.xml"_ustr),
                        "/w:document/w:body/w:tbl/w:tblPr/w:tblW", "w",
                        bMac ? u"5564"_ustr : u"5000"_ustr);
            }
        }
    }
}


CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacTableNoScriptGapCompression)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr, u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr, u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aNames[1].pData, aOn.pData);
    osl_setEnvironment(aNames[2].pData, aOn.pData);
    osl_setEnvironment(aNames[3].pData, aOn.pData);
    for (bool bMac : {false, true})
    {
        const OUString aValue = bMac ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aNames[0].pData, aValue.pData);
        for (const OString& rControl : {"absent"_ostr, "doNotCompress"_ostr,
                                      "compressPunctuation"_ostr})
        {
            for (bool bSong : {false, true})
            {
                utl::TempFileNamed aFile;
                aFile.EnableKillingFile();
                makeWordMacPackage(aFile, false, true);
                {
                    auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                        ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
                    auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
                    OString aDocument = R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="WIDTH" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="WIDTH"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="WIDTH" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:autoSpaceDE/><w:autoSpaceDN/><w:spacing w:before="0" w:after="0" w:line="360" w:lineRule="exact"/><w:snapToGrid w:val="0"/><w:jc w:val="left"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FONT" w:hAnsi="FONT" w:eastAsia="FONT"/><w:sz w:val="SIZE"/></w:rPr><w:t>甲1乙</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr;
                    aDocument = aDocument.replaceAll("WIDTH"_ostr, bSong ? "820"_ostr : "700"_ostr)
                        .replaceAll("FONT"_ostr, bSong ? "SimSun"_ostr : "FangSong_GB2312"_ostr)
                        .replaceAll("SIZE"_ostr, bSong ? "28"_ostr : "24"_ostr);
                    writeWordMacPart(xWord, u"document.xml"_ustr, aDocument);
                    const OString aControl = rControl == "absent" ? OString()
                        : "<w:characterSpacingControl w:val=\""_ostr + rControl + "\"/>"_ostr;
                    writeWordMacPart(xWord, u"settings.xml"_ustr,
                        R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">)xml"_ostr
                        + aControl
                        + R"xml(<w:compat><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val="15"/></w:compat></w:settings>)xml"_ostr);
                    uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
                    uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
                }
                for (int nRound = 0; nRound < 2; ++nRound)
                {
                    if (!nRound)
                        loadFromURL(aFile.GetURL());
                    else
                        saveAndReload(TestFilter::DOCX);
                    const bool bNoCompression = bMac && rControl != "compressPunctuation";
                    auto pDump = parseLayoutDump();
                    assertXPath(pDump,
                        "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout",
                        bNoCompression ? 2 : 1);
                    if (bNoCompression)
                    {
                        assertXPath(pDump,
                            "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout[1]",
                            "portion", u"甲1");
                        assertXPath(pDump,
                            "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout[2]",
                            "portion", u"乙");
                    }
                }
            }
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacHangingPunctuationPaint)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
        const OUString aOn = u"1"_ustr;
        osl_setEnvironment(rName.pData, aOn.pData);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });

    for (bool bHanging : { false, true })
    {
        for (const OString& rCompression : { "doNotCompress"_ostr, "compressPunctuation"_ostr })
        {
            utl::TempFileNamed aFile;
            aFile.EnableKillingFile();
            makeWordMacPackage(aFile, false, true);
            {
                auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                    ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
                auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
                OString aDocument = R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:wordWrap w:val="0"/><w:autoSpaceDE/><w:autoSpaceDN/><w:adjustRightInd w:val="0"/><w:overflowPunct w:val="HANG"/><w:snapToGrid/><w:spacing w:before="0" w:after="0" w:line="276" w:lineRule="auto"/><w:ind w:firstLine="0"/><w:jc w:val="both"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="DengXian" w:hAnsi="DengXian" w:eastAsia="DengXian"/><w:sz w:val="24"/><w:lang w:val="zh-CN" w:eastAsia="zh-CN"/></w:rPr><w:t>TEXT</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="12240" w:h="15840"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr;
                aDocument = aDocument.replaceAll("HANG"_ostr, bHanging ? "1"_ostr : "0"_ostr)
                    .replaceAll("TEXT"_ostr, "“甲”甲甲AI乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙乙，丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙丙。"_ostr);
                writeWordMacPart(xWord, u"document.xml"_ustr, aDocument);
                writeWordMacPart(xWord, u"settings.xml"_ustr,
                    R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:characterSpacingControl w:val=")xml"_ostr
                    + rCompression
                    + R"xml("/><w:compat><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val="15"/></w:compat></w:settings>)xml"_ostr);
                uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
                uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
            }
            for (int nRound = 0; nRound < 2; ++nRound)
            {
                if (!nRound)
                    loadFromURL(aFile.GetURL());
                else
                    saveAndReload(TestFilter::DOCX);
                save(TestFilter::PDF_WRITER);
                auto pPdf = parsePDFExport();
                CPPUNIT_ASSERT(pPdf);
                CPPUNIT_ASSERT_EQUAL(1, pPdf->getPageCount());
                auto pPage = pPdf->openPage(0);
                auto pText = pPage->getTextPage();
                OUString aRendered;
                for (int i = 0; i < pPage->getObjectCount(); ++i)
                    aRendered += pPage->getObject(i)->getText(pText);
                // The hanging comma is present in layout but used to be
                // skipped by the PDF paint loop after script-gap adjustment.
                CPPUNIT_ASSERT_MESSAGE("Hanging comma must reach PDF paint",
                    aRendered.indexOf(u'，') >= 0);
                CPPUNIT_ASSERT_MESSAGE("Final punctuation must also be painted",
                    aRendered.indexOf(u'。') >= 0);
            }
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacClosingUnitTrailingCjk)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
        const OUString aOn = u"1"_ustr;
        osl_setEnvironment(rName.pData, aOn.pData);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="1270" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="108" w:type="dxa"/><w:right w:w="108" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="1270"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="1270" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="360" w:lineRule="auto"/><w:jc w:val="left"/></w:pPr><w:r><w:rPr><w:rFonts w:hint="eastAsia" w:ascii="FangSong_GB2312" w:hAnsi="DengXian" w:eastAsia="FangSong_GB2312"/><w:b/><w:kern w:val="0"/><w:sz w:val="24"/><w:lang w:val="en-US" w:eastAsia="zh-CN"/></w:rPr><w:t>甲乙丙丁（AB）戊己</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr);
        writeWordMacPart(xWord, u"settings.xml"_ustr,
            R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:characterSpacingControl w:val="compressPunctuation"/><w:compat><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val="14"/></w:compat></w:settings>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    for (int nRound = 0; nRound < 2; ++nRound)
    {
        if (!nRound)
            loadFromURL(aFile.GetURL());
        else
            saveAndReload(TestFilter::DOCX);
        auto pDump = parseLayoutDump();
        assertXPath(pDump, "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout", 3);
        assertXPath(pDump, "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout[2]",
            "portion", u"（AB）戊");
        assertXPath(pDump, "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout[3]",
            "portion", u"己");
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacMinimumRowFragments)
{
    const OUString aEnv = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bPrevious = osl_getEnvironment(aEnv.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bPrevious)
            osl_setEnvironment(aEnv.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aEnv.pData);
    });
    for (const auto& [nCompat, bRepeat] : std::vector<std::pair<int, bool>>{
             {14, false}, {14, true}, {15, false}, {15, true}})
    {
        for (bool bMac : { false, true })
        {
            const OUString aValue = bMac ? u"1"_ustr : u"0"_ustr;
            osl_setEnvironment(aEnv.pData, aValue.pData);
            for (bool bMinimum : { false, true })
            {
                for (int nSlots : { 1, 2, 3 })
                {
                    utl::TempFileNamed aFile;
                    aFile.EnableKillingFile();
                    makeWordMacPackage(aFile, false, true);
                    {
                        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
                        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
                        const OString aSpacer = OString::number(13958 - 400 - 480 * nSlots - 50);
                        const OString aRow = bMinimum
                            ? "<w:trPr><w:trHeight w:val=\"1140\" w:hRule=\"atLeast\"/></w:trPr>"_ostr
                            : OString();
                        const OString aHeader = bRepeat
                            ? "<w:trPr><w:tblHeader/></w:trPr>"_ostr : OString();
                        writeWordMacPart(xWord, u"document.xml"_ustr,
                            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body>
    <w:p><w:pPr><w:keepNext w:val="0"/><w:widowControl w:val="0"/><w:snapToGrid w:val="0"/><w:spacing w:before="0" w:after="0" w:line=")xml"_ostr + aSpacer + R"xml(" w:lineRule="exact"/></w:pPr><w:r><w:t>Spacer</w:t></w:r></w:p>
    <w:tbl><w:tblPr><w:tblW w:type="dxa" w:w="8000"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="8000"/></w:tblGrid>
    <w:tr>)xml"_ostr + aHeader + R"xml(<w:tc><w:tcPr><w:tcW w:type="dxa" w:w="8000"/></w:tcPr><w:p><w:pPr><w:keepNext w:val="0"/><w:widowControl w:val="0"/><w:snapToGrid w:val="0"/><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/></w:pPr><w:r><w:t>Header</w:t></w:r></w:p></w:tc></w:tr>
    <w:tr>)xml"_ostr + aRow + R"xml(<w:tc><w:tcPr><w:tcW w:type="dxa" w:w="8000"/><w:vAlign w:val="center"/></w:tcPr>
    <w:p><w:pPr><w:keepNext w:val="0"/><w:keepLines w:val="0"/><w:widowControl w:val="0"/><w:snapToGrid w:val="0"/><w:spacing w:before="0" w:after="0" w:line="480" w:lineRule="exact"/></w:pPr><w:r><w:t>Heading</w:t></w:r></w:p>
    <w:p><w:pPr><w:keepNext w:val="0"/><w:keepLines w:val="0"/><w:widowControl w:val="0"/><w:snapToGrid w:val="0"/><w:spacing w:before="0" w:after="0" w:line="480" w:lineRule="exact"/></w:pPr><w:r><w:t>First</w:t><w:br/><w:t>Second</w:t><w:br/><w:t>Third</w:t></w:r></w:p>
    </w:tc></w:tr></w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr);
                        writeWordMacPart(xWord, u"settings.xml"_ustr,
                            R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:compat><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val=")xml"_ostr + OString::number(nCompat) + R"xml("/></w:compat></w:settings>)xml"_ostr);
                        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
                        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
                    }
                    for (int nRound = 0; nRound < 2; ++nRound)
                    {
                        if (!nRound)
                            loadFromURL(aFile.GetURL());
                        else
                            saveAndReload(TestFilter::DOCX);
                        auto pDump = parseLayoutDump();
                        // The first fragment must have room for the declared minimum;
                        // a variable row or a larger available area may still split.
                        const bool bMove = bMac && bMinimum && nSlots < 3;
                        if (bMove && bRepeat && nCompat >= 15)
                            assertXPath(pDump, "/root/page[1]/body/tab", 0);
                        else
                            assertXPath(pDump, "/root/page[1]/body/tab/row", bMove ? 1 : 2);
                        assertXPath(pDump, "/root/page[2]/body/tab/row", bRepeat ? 2 : 1);
                        // Word preserves the minimum on the short final fragment too.
                        if (nSlots == 3)
                        {
                            assertXPath(pDump, "/root/page[2]/body/tab/row[last()]/infos/bounds",
                                "height", bMac && bMinimum ? u"1140" : u"480");
                            if (bMac && bMinimum)
                            {
                                const sal_Int32 nRowTop = getXPath(pDump,
                                    "/root/page[2]/body/tab/row[last()]/infos/bounds", "top").toInt32();
                                const sal_Int32 nTextTop = getXPath(pDump,
                                    "/root/page[2]/body/tab/row[last()]/cell/txt[last()]/infos/bounds",
                                    "top").toInt32();
                                CPPUNIT_ASSERT_EQUAL_MESSAGE("Word top-aligns continued cell text",
                                    nRowTop, nTextTop);
                            }
                        }
                    }
                }
            }
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacTableHangingPrefix)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
        const OUString aOn = u"1"_ustr;
        osl_setEnvironment(rName.pData, aOn.pData);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    for (int nCompat : {14, 15})
    {
        for (const OString& rAlign : {"left"_ostr, "both"_ostr})
        {
            for (const OString& rPrefix : {"1"_ostr, "A"_ostr, "AB"_ostr, "10"_ostr})
            {
                utl::TempFileNamed aFile;
                aFile.EnableKillingFile();
                makeWordMacPackage(aFile, false, true);
                {
                    auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                        ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
                    auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
                    writeWordMacPart(xWord, u"document.xml"_ustr,
                        R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="4856" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4856"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="4856" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:jc w:val=")xml"_ostr + rAlign + R"xml("/><w:overflowPunct/><w:autoSpaceDE/><w:autoSpaceDN/><w:snapToGrid w:val="0"/><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="auto"/><w:ind w:left="0" w:right="0"/></w:pPr><w:r><w:rPr><w:rFonts w:hint="eastAsia" w:ascii="FangSong_GB2312" w:hAnsi="FangSong" w:eastAsia="FangSong_GB2312"/><w:sz w:val="24"/><w:lang w:eastAsia="zh"/></w:rPr><w:t>)xml"_ostr + rPrefix + R"xml(、甲乙丙丁戊己庚辛壬癸子，丑寅卯辰巳午未，申酉戌亥甲乙丙丁戊己庚辛壬癸子丑寅卯辰巳午未；</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr);
                    writeWordMacPart(xWord, u"settings.xml"_ustr,
                        R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:characterSpacingControl w:val="compressPunctuation"/><w:compat><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val=")xml"_ostr + OString::number(nCompat) + R"xml("/></w:compat></w:settings>)xml"_ostr);
                    uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
                    uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
                }
                for (int nRound = 0; nRound < 2; ++nRound)
                {
                    if (!nRound)
                        loadFromURL(aFile.GetURL());
                    else
                        saveAndReload(TestFilter::DOCX);
                    const bool bHanging = rPrefix.getLength() == 1
                        && (nCompat == 14 || rAlign == "both");
                    const OUString aExpected = OUString::fromUtf8(rPrefix)
                        + u"、甲乙丙丁戊己庚辛壬癸子，丑寅卯辰巳午"_ustr
                        + (bHanging ? u"未，"_ustr : OUString());
                    assertXPath(parseLayoutDump(),
                        "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout[1]",
                        "portion", aExpected);
                }
            }
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacLegacyPairedParentheses)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
        const OUString aOn = u"1"_ustr;
        osl_setEnvironment(rName.pData, aOn.pData);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    for (int nCompat : {14, 15})
    {
        for (const OString& rAlign : {"left"_ostr, "center"_ostr})
        {
            for (int nWidth : {960, 1095, 1200})
            {
                if (nCompat == 15 && nWidth != 1095)
                    continue;
                utl::TempFileNamed aFile;
                aFile.EnableKillingFile();
                makeWordMacPackage(aFile, false, true);
                {
                    auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                        ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
                    auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
                    writeWordMacPart(xWord, u"document.xml"_ustr,
                        R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w=")xml"_ostr + OString::number(nWidth) + R"xml(" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w=")xml"_ostr + OString::number(nWidth) + R"xml("/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w=")xml"_ostr + OString::number(nWidth) + R"xml(" w:type="dxa"/></w:tcPr><w:p><w:pPr><w:jc w:val=")xml"_ostr + rAlign + R"xml("/><w:overflowPunct/><w:autoSpaceDE/><w:autoSpaceDN/><w:snapToGrid w:val="0"/><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="auto"/><w:ind w:left="0" w:right="0"/></w:pPr><w:r><w:rPr><w:rFonts w:hint="eastAsia" w:ascii="FangSong_GB2312" w:hAnsi="FangSong" w:eastAsia="FangSong_GB2312"/><w:b/><w:sz w:val="24"/><w:lang w:eastAsia="zh"/></w:rPr><w:t>甲乙丙丁戊（己庚）</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr);
                    writeWordMacPart(xWord, u"settings.xml"_ustr,
                        R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:characterSpacingControl w:val="compressPunctuation"/><w:compat><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val=")xml"_ostr + OString::number(nCompat) + R"xml("/></w:compat></w:settings>)xml"_ostr);
                    uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
                    uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
                }
                for (int nRound = 0; nRound < 2; ++nRound)
                {
                    if (!nRound)
                        loadFromURL(aFile.GetURL());
                    else
                        saveAndReload(TestFilter::DOCX);
                    std::vector<OUString> aExpected;
                    if (nWidth == 960)
                        aExpected = nCompat == 14
                            ? std::vector<OUString>{u"甲乙丙"_ustr, u"丁戊（己"_ustr, u"庚）"_ustr}
                            : std::vector<OUString>{u"甲乙丙"_ustr, u"丁戊"_ustr, u"（己"_ustr, u"庚）"_ustr};
                    else
                        aExpected = nCompat == 14
                            ? std::vector<OUString>{u"甲乙丙丁"_ustr, u"戊（己庚）"_ustr}
                            : std::vector<OUString>{u"甲乙丙丁"_ustr, u"戊（己"_ustr, u"庚）"_ustr};
                    auto pDump = parseLayoutDump();
                    const OString aPath("/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout");
                    assertXPath(pDump, aPath, static_cast<sal_Int32>(aExpected.size()));
                    for (size_t i = 0; i < aExpected.size(); ++i)
                        assertXPath(pDump, aPath + "[" + OString::number(i + 1) + "]",
                            "portion", aExpected[i]);
                }
            }
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacFixedPercentColumns)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
        const OUString aOn = u"1"_ustr;
        osl_setEnvironment(rName.pData, aOn.pData);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    for (bool bMac : {false, true})
    {
        for (bool bPercent : {false, true})
        {
            const OUString aValue = bMac ? u"1"_ustr : u"0"_ustr;
            osl_setEnvironment(aNames.front().pData, aValue.pData);
            for (int nGridWidth : (bPercent ? std::vector<int>{8701, 8702, 8705, 10000}
                                           : std::vector<int>{8701}))
            {
                for (int nTablePercent : {5105, 5120})
                {
                    utl::TempFileNamed aFile;
                    aFile.EnableKillingFile();
                    makeWordMacPackage(aFile, false, true);
                    {
                        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
                        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
                        const std::vector<int> aGrid{1463, 4115, 951, 981, 1191};
                        const std::vector<int> aPercents{840, 2364, 546, 563, 684};
                        OString aCells;
                        for (size_t i = 0; i < aGrid.size(); ++i)
                        {
                            aCells += R"xml(<w:tc><w:tcPr><w:tcW w:type=")xml"_ostr
                                + (bPercent ? "pct"_ostr : "dxa"_ostr)
                                + R"xml(" w:w=")xml"_ostr
                                + OString::number(bPercent ? aPercents[i] : aGrid[i])
                                + R"xml("/><w:tcBorders><w:top w:val="single" w:sz="4"/><w:left w:val="single" w:sz="4"/><w:bottom w:val="single" w:sz="4"/><w:right w:val="single" w:sz="4"/></w:tcBorders></w:tcPr><w:p><w:pPr><w:jc w:val="center"/><w:snapToGrid w:val="0"/><w:spacing w:before="0" w:after="0" w:line="360" w:lineRule="auto"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="24"/><w:b/></w:rPr><w:t>甲乙丙丁戊己</w:t></w:r></w:p></w:tc>)xml"_ostr;
                        }
                        writeWordMacPart(xWord, u"document.xml"_ustr,
                            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w=")xml"_ostr + OString::number(nTablePercent) + R"xml(" w:type="pct"/><w:tblInd w:w="0" w:type="dxa"/><w:tblLayout w:type="fixed"/><w:tblBorders><w:top w:val="nil"/><w:left w:val="nil"/><w:bottom w:val="nil"/><w:right w:val="nil"/><w:insideH w:val="nil"/><w:insideV w:val="nil"/></w:tblBorders><w:tblCellMar><w:left w:w="108" w:type="dxa"/><w:right w:w="108" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="1463"/><w:gridCol w:w="4115"/><w:gridCol w:w="951"/><w:gridCol w:w="981"/><w:gridCol w:w=")xml"_ostr + OString::number(nGridWidth - 7510) + R"xml("/></w:tblGrid><w:tr>)xml"_ostr
                            + aCells + R"xml(</w:tr></w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1800" w:right="1800"/></w:sectPr></w:body></w:document>)xml"_ostr);
                        writeWordMacPart(xWord, u"settings.xml"_ustr,
                            R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:compat><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val="14"/></w:compat></w:settings>)xml"_ostr);
                        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
                        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
                    }
                    for (int nRound = 0; nRound < 2; ++nRound)
                    {
                        if (!nRound)
                        {
                            loadFromURL(aFile.GetURL());
                            if (bMac && bPercent && nGridWidth == 10000)
                                saveAndReload(TestFilter::DOCX);
                        }
                        else
                            saveAndReload(TestFilter::DOCX);
                        if (bMac && bPercent && nRound)
                        {
                            auto pXml = parseExport(u"word/document.xml"_ustr);
                            assertXPath(pXml, "/w:document/w:body/w:tbl/w:tblPr/w:tblW",
                                "w", OUString::number(nTablePercent));
                            assertXPath(pXml, "/w:document/w:body/w:tbl/w:tr/w:tc[3]/w:tcPr/w:tcW",
                                "type", u"pct");
                        }
                        auto pDump = parseLayoutDump();
                        const OString aContext = "mac=" + OString::number(int(bMac)) + " pct=" + OString::number(int(bPercent))
                            + " grid=" + OString::number(nGridWidth)
                            + " percent=" + OString::number(nTablePercent)
                            + " round=" + OString::number(nRound)
                            + " cell=" + getXPath(pDump,
                                "/root/page/body/tab/row/cell[3]/infos/bounds", "width").toUtf8()
                            + " print=" + getXPath(pDump,
                                "/root/page/body/tab/row/cell[3]/infos/prtBounds", "width").toUtf8()
                            + " table=" + getXPath(pDump,
                                "/root/page/body/tab/infos/prtBounds", "width").toUtf8();
                        CPPUNIT_ASSERT_EQUAL_MESSAGE(aContext.getStr(),
                            bMac && (!bPercent || nTablePercent == 5120) ? 2 : 3,
                            countXPathNodes(pDump,
                                "/root/page/body/tab/row/cell[3]/txt/SwParaPortion/SwLineLayout"));
                        if (bMac && bPercent)
                        {
                            CPPUNIT_ASSERT_EQUAL_MESSAGE(aContext.getStr(),
                                nTablePercent == 5105 ? u"甲乙"_ustr : u"甲乙丙"_ustr,
                                getXPath(pDump,
                                    "/root/page/body/tab/row/cell[3]/txt/SwParaPortion/SwLineLayout[1]",
                                    "portion"));
                        }
                    }
                }
            }
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacBodyPairedDelimiters)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
        const OUString aOn = u"1"_ustr;
        osl_setEnvironment(rName.pData, aOn.pData);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    const std::vector<std::pair<OUString, OUString>> aPairs{
        {u"《"_ustr, u"》"_ustr}, {u"（"_ustr, u"）"_ustr},
        {u"“"_ustr, u"”"_ustr}, {u"〈"_ustr, u"〉"_ustr}};
    for (bool bCompress : {false, true})
    for (int nCompat : {14, 15})
    for (const auto& rPair : aPairs)
    for (const OString& rAlign : {"left"_ostr, "both"_ostr})
    for (bool bGrid : {false, true})
    {
        const OUString aPrefix = u"甲乙丙丁戊己庚辛壬癸；"_ustr + rPair.first
            + u"子丑寅卯辰巳午未申酉戌亥甲乙"_ustr + rPair.second;
        const OUString aText = aPrefix + u"丙丁戊己庚辛壬癸子丑寅卯辰巳午未申酉戌亥，继续核对文字排版及段落换行情况。"_ustr;
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            writeWordMacPart(xWord, u"document.xml"_ustr,
                R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:keepNext w:val="0"/><w:keepLines w:val="0"/><w:widowControl w:val="0"/><w:wordWrap/><w:overflowPunct/><w:autoSpaceDE/><w:autoSpaceDN/><w:snapToGrid w:val=")xml"_ostr
                + OString::number(int(bGrid)) + R"xml("/><w:spacing w:before="0" w:after="0" w:line="560" w:lineRule="exact"/><w:ind w:left="0" w:right="0" w:firstLine="0"/><w:jc w:val=")xml"_ostr
                + rAlign + R"xml("/></w:pPr><w:r><w:rPr><w:rFonts w:hint="eastAsia" w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="32"/><w:lang w:eastAsia="zh"/></w:rPr><w:t>)xml"_ostr
                + aText.toUtf8() + R"xml(</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1800" w:right="1800"/><w:docGrid w:type="lines" w:linePitch="560"/></w:sectPr></w:body></w:document>)xml"_ostr);
            writeWordMacPart(xWord, u"settings.xml"_ustr,
                R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:characterSpacingControl w:val=")xml"_ostr
                + (bCompress ? "compressPunctuation"_ostr : "doNotCompress"_ostr)
                + R"xml("/><w:compat><w:doNotWrapTextWithPunct/><w:doNotUseEastAsianBreakRules/><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val=")xml"_ostr
                + OString::number(nCompat) + R"xml("/></w:compat></w:settings>)xml"_ostr);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        for (int nRound = 0; nRound < 2; ++nRound)
        {
            if (!nRound)
                loadFromURL(aFile.GetURL());
            else
                saveAndReload(TestFilter::DOCX);
            const OUString aExpected = !bCompress || (nCompat == 15 && rAlign == "left")
                ? aPrefix.copy(0, 25) : aPrefix;
            auto pDump = parseLayoutDump();
            assertXPath(pDump, "/root/page/body/txt/SwParaPortion/SwLineLayout[1]",
                "portion", aExpected);
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacInlineImageExactGrid)
{
    const OUString aName = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bHadPrevious = osl_getEnvironment(aName.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bHadPrevious)
            osl_setEnvironment(aName.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aName.pData);
    });
    const OUString aOn = u"1"_ustr;
    osl_setEnvironment(aName.pData, aOn.pData);
    for (int nCompat : {14, 15})
    for (bool bTable : {false, true})
    for (int nImageHeight : {3118, 3120, 3180})
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        writeWordMacPart(xRoot, u"[Content_Types].xml"_ustr,
            R"xml(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="xml" ContentType="application/xml"/><Default Extension="png" ContentType="image/png"/><Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/><Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/><Override PartName="/word/settings.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"/></Types>)xml"_ostr);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        auto xRels = xWord->openStorageElement(u"_rels"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xRels, u"document.xml.rels"_ustr,
            R"xml(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/><Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings" Target="settings.xml"/><Relationship Id="image" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" Target="media/probe.png"/></Relationships>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xRels, uno::UNO_QUERY_THROW)->commit();
        auto xMedia = xWord->openStorageElement(u"media"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xMedia, u"probe.png"_ustr,
            OString("\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52\x00\x00\x00\x01\x00\x00\x00\x01\x08\x02\x00\x00\x00\x90\x77\x53\xde\x00\x00\x00\x0c\x49\x44\x41\x54\x78\x9c\x63\x60\x60\x60\x00\x00\x00\x04\x00\x01\xf6\x17\x38\x55\x00\x00\x00\x00\x49\x45\x4e\x44\xae\x42\x60\x82", 69));
        uno::Reference<embed::XTransactedObject>(xMedia, uno::UNO_QUERY_THROW)->commit();
        const OString aCy = OString::number(nImageHeight * 635);
        const OString aParagraph =
            R"xml(<w:p><w:pPr><w:snapToGrid/><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="auto"/><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="24"/></w:rPr></w:pPr><w:r><w:rPr><w:rFonts w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="24"/></w:rPr><w:drawing><wp:inline><wp:extent cx="1270000" cy=")xml"_ostr + aCy + R"xml("/><wp:effectExtent l="0" t="0" r="0" b="0"/><wp:docPr id="1" name="Synthetic"/><a:graphic><a:graphicData uri="http://schemas.openxmlformats.org/drawingml/2006/picture"><pic:pic><pic:nvPicPr><pic:cNvPr id="1" name="probe.png"/><pic:cNvPicPr/></pic:nvPicPr><pic:blipFill><a:blip r:embed="image"/><a:stretch><a:fillRect/></a:stretch></pic:blipFill><pic:spPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="1270000" cy=")xml"_ostr + aCy + R"xml("/></a:xfrm><a:prstGeom prst="rect"><a:avLst/></a:prstGeom></pic:spPr></pic:pic></a:graphicData></a:graphic></wp:inline></w:drawing></w:r></w:p>)xml"_ostr;
        const OString aContents = bTable
            ? R"xml(<w:tbl><w:tblPr><w:tblW w:type="dxa" w:w="4000"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4000"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:type="dxa" w:w="4000"/></w:tcPr>)xml"_ostr
                + aParagraph + "</w:tc></w:tr></w:tbl>"_ostr
            : aParagraph;
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main" xmlns:wp="http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing" xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:pic="http://schemas.openxmlformats.org/drawingml/2006/picture" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"><w:body>)xml"_ostr
            + aContents + R"xml(<w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:body></w:document>)xml"_ostr);
        writeWordMacPart(xWord, u"settings.xml"_ustr,
            R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:compat><w:adjustLineHeightInTable/><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val=")xml"_ostr
            + OString::number(nCompat) + R"xml("/></w:compat></w:settings>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        for (int nRound = 0; nRound < 2; ++nRound)
        {
            if (!nRound)
                loadFromURL(aFile.GetURL());
            else
                saveAndReload(TestFilter::DOCX);
            for (int nPass = 0; nPass < 3; ++nPass)
            {
                calcLayout();
                auto pDump = parseLayoutDump();
                const OString aPath = bTable ? "/root/page[1]/body/tab/row/cell/txt[1]"_ostr
                                             : "/root/page[1]/body/txt[1]"_ostr;
                const auto nTop = getXPath(pDump, aPath + "/infos/bounds", "top").toInt32();
                const auto nImageTop = getXPath(pDump, aPath + "/anchored/fly/infos/bounds", "top").toInt32();
                const auto nHeight = getXPath(pDump, aPath + "/SwParaPortion/SwLineLayout", "height").toInt32();
                // True Word PDF: 155.9pt uses156pt; exactly156pt and159pt
                // use171.6pt. An exact image must advance to the next grid row.
                CPPUNIT_ASSERT_EQUAL(sal_Int32(nImageHeight == 3118 ? 3120 : 3432), nHeight);
                const sal_Int32 nExpectedOffset = nImageHeight == 3118 ? 1
                    : nImageHeight == 3120 ? 156 : 126;
                CPPUNIT_ASSERT(std::abs(nImageTop - nTop - nExpectedOffset) <= 2);
                getSwDocShell()->GetWrtShell()->Reformat();
            }
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacCoveredRowMinimum)
{
    const OUString aEnv = u"COCOLIAN_WORD_MAC_METRICS"_ustr;
    OUString aPrevious;
    const bool bPrevious = osl_getEnvironment(aEnv.pData, &aPrevious.pData) == osl_Process_E_None;
    comphelper::ScopeGuard aRestore([&] {
        if (bPrevious)
            osl_setEnvironment(aEnv.pData, aPrevious.pData);
        else
            osl_clearEnvironment(aEnv.pData);
    });
    for (bool bMac : {false, true})
    for (int nCompat : {14, 15})
    for (bool bMerged : {false, true})
    for (int nMinimum : {460, 1140})
    {
        const OUString aValue = bMac ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aEnv.pData, aValue.pData);
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            const OString aStart = bMerged ? "<w:vMerge w:val=\"restart\"/>"_ostr : OString();
            const OString aCover = bMerged ? "<w:vMerge/>"_ostr : OString();
            writeWordMacPart(xWord, u"document.xml"_ustr,
                R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body>
<w:p><w:pPr><w:keepNext w:val="0"/><w:keepLines w:val="0"/><w:widowControl w:val="0"/><w:snapToGrid w:val="0"/><w:spacing w:before="0" w:after="0" w:line="11508" w:lineRule="exact"/></w:pPr><w:r><w:t>Spacer</w:t></w:r></w:p>
<w:tbl><w:tblPr><w:tblW w:type="dxa" w:w="8000"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/><w:left w:w="0" w:type="dxa"/><w:right w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="1000"/><w:gridCol w:w="7000"/></w:tblGrid>
<w:tr><w:tc><w:tcPr><w:tcW w:type="dxa" w:w="1000"/>)xml"_ostr + aStart + R"xml(</w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="exact"/></w:pPr><w:r><w:t>Left</w:t></w:r></w:p></w:tc><w:tc><w:tcPr><w:tcW w:type="dxa" w:w="7000"/></w:tcPr><w:p><w:pPr><w:keepNext w:val="0"/><w:spacing w:before="0" w:after="0" w:line="400" w:lineRule="exact"/></w:pPr><w:r><w:t>Header</w:t></w:r></w:p></w:tc></w:tr>
<w:tr><w:trPr><w:trHeight w:val=")xml"_ostr + OString::number(nMinimum) + R"xml(" w:hRule="atLeast"/></w:trPr><w:tc><w:tcPr><w:tcW w:type="dxa" w:w="1000"/>)xml"_ostr + aCover + R"xml(</w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="312" w:lineRule="exact"/></w:pPr></w:p></w:tc><w:tc><w:tcPr><w:tcW w:type="dxa" w:w="7000"/><w:vAlign w:val="center"/></w:tcPr>
<w:p><w:pPr><w:keepNext w:val="0"/><w:keepLines w:val="0"/><w:widowControl w:val="0"/><w:snapToGrid w:val="0"/><w:spacing w:before="0" w:after="0" w:line="480" w:lineRule="exact"/></w:pPr><w:r><w:t>First</w:t><w:br/><w:t>Second</w:t><w:br/><w:t>Third</w:t><w:br/><w:t>Fourth</w:t></w:r></w:p>
<w:p><w:pPr><w:keepNext w:val="0"/><w:keepLines w:val="0"/><w:widowControl w:val="0"/><w:snapToGrid w:val="0"/><w:spacing w:before="0" w:after="0" w:line="312" w:lineRule="exact"/></w:pPr></w:p>
</w:tc></w:tr></w:tbl><w:p><w:r><w:t>After</w:t></w:r></w:p><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr);
            writeWordMacPart(xWord, u"settings.xml"_ustr,
                R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:compat><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val=")xml"_ostr
                + OString::number(nCompat) + R"xml("/></w:compat></w:settings>)xml"_ostr);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        for (int nRound = 0; nRound < 2; ++nRound)
        {
            if (!nRound)
                loadFromURL(aFile.GetURL());
            else
                saveAndReload(TestFilter::DOCX);
            auto pDump = parseLayoutDump();
            assertXPath(pDump, "/root/page", 2);
            const auto nHeight = getXPath(pDump,
                "/root/page[2]/body/tab/row[last()]/infos/bounds", "height").toInt32();
            // Word retains the minimum on the final empty fragment even
            // when another cell is covered by a vertical merge from above.
            if (bMac)
                CPPUNIT_ASSERT_GREATEREQUAL(sal_Int32(nMinimum), nHeight);
            else
                CPPUNIT_ASSERT(nHeight < nMinimum);
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacClosingUnitPunctuationRun)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
        const OUString aOn = u"1"_ustr;
        osl_setEnvironment(rName.pData, aOn.pData);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    for (int nCompat : {14, 15})
    for (const OString& rAlign : {"left"_ostr, "center"_ostr, "both"_ostr})
    for (bool bGrid : {false, true})
    for (const OUString& rToken : {u"CRM"_ustr, u"API"_ustr})
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        const OUString aPrefix = u"甲乙丙丁、戊己庚辛（"_ustr + rToken + u"）、"_ustr;
        const OUString aText = aPrefix + u"壬癸子丑寅卯（辰巳午未）；申酉戌亥甲乙丙丁。"_ustr;
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            writeWordMacPart(xWord, u"document.xml"_ustr,
                R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:type="dxa" w:w="3262"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/><w:left w:w="108" w:type="dxa"/><w:right w:w="108" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="3262"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:type="dxa" w:w="3262"/></w:tcPr><w:p><w:pPr><w:keepNext w:val="0"/><w:keepLines w:val="0"/><w:widowControl w:val="0"/><w:wordWrap/><w:overflowPunct/><w:autoSpaceDE/><w:autoSpaceDN/><w:snapToGrid w:val=")xml"_ostr
                + OString::number(int(bGrid)) + R"xml("/><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="auto"/><w:jc w:val=")xml"_ostr
                + rAlign + R"xml("/></w:pPr><w:r><w:rPr><w:rFonts w:hint="eastAsia" w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="24"/><w:lang w:val="en-US" w:eastAsia="zh-CN"/></w:rPr><w:t>)xml"_ostr
                + aText.toUtf8() + R"xml(</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:body></w:document>)xml"_ostr);
            writeWordMacPart(xWord, u"settings.xml"_ustr,
                R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:characterSpacingControl w:val="compressPunctuation"/><w:compat><w:adjustLineHeightInTable/><w:doNotWrapTextWithPunct/><w:doNotUseEastAsianBreakRules/><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val=")xml"_ostr
                + OString::number(nCompat) + R"xml("/></w:compat></w:settings>)xml"_ostr);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        for (int nRound = 0; nRound < 2; ++nRound)
        {
            if (!nRound)
                loadFromURL(aFile.GetURL());
            else
                saveAndReload(TestFilter::DOCX);
            auto pDump = parseLayoutDump();
            const OUString aExpected = nCompat == 15 && rAlign != "both"
                ? u"甲乙丙丁、戊己庚辛"_ustr : aPrefix;
            // The closing parentheses and comma stay with their ASCII unit;
            // Chinese letters following that unit cannot borrow its budget.
            assertXPath(pDump, "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout[1]",
                "portion", aExpected);
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacCellOpeningPunctuationBudget)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
        const OUString aOn = u"1"_ustr;
        osl_setEnvironment(rName.pData, aOn.pData);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    for (int nCompat : {14, 15})
    for (const OString& rAlign : {"left"_ostr, "center"_ostr, "both"_ostr})
    for (bool bGrid : {false, true})
    for (int nToken : {0, 1, 2})
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        const OUString aPrefix = nToken == 2
            ? u"甲乙丙丁戊己庚辛壬癸子丑寅子丑"_ustr
            : u"甲乙：丙丁戊己庚辛，壬癸子丑寅"_ustr;
        const OUString aOpen = nToken == 1 ? u"（"_ustr : u"“"_ustr;
        const OUString aClose = nToken == 1 ? u"）"_ustr : u"”"_ustr;
        const OUString aText = aPrefix + aOpen + u"卯辰"_ustr + aClose
            + u"巳午未申酉戌亥甲乙丙丁。"_ustr;
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            writeWordMacPart(xWord, u"document.xml"_ustr,
                R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:type="dxa" w:w="4140"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/><w:left w:w="108" w:type="dxa"/><w:right w:w="108" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4140"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:type="dxa" w:w="4140"/></w:tcPr><w:p><w:pPr><w:keepNext w:val="0"/><w:keepLines w:val="0"/><w:widowControl w:val="0"/><w:wordWrap/><w:overflowPunct/><w:autoSpaceDE/><w:autoSpaceDN/><w:snapToGrid w:val=")xml"_ostr
                + OString::number(int(bGrid)) + R"xml("/><w:spacing w:before="0" w:after="0" w:line="360" w:lineRule="auto"/><w:jc w:val=")xml"_ostr
                + rAlign + R"xml("/></w:pPr><w:r><w:rPr><w:rFonts w:hint="eastAsia" w:ascii="FangSong_GB2312" w:hAnsi="FangSong_GB2312" w:eastAsia="FangSong_GB2312"/><w:sz w:val="24"/><w:lang w:val="en-US" w:eastAsia="zh-CN"/></w:rPr><w:t>)xml"_ostr
                + aText.toUtf8() + R"xml(</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/><w:docGrid w:type="lines" w:linePitch="312"/></w:sectPr></w:body></w:document>)xml"_ostr);
            writeWordMacPart(xWord, u"settings.xml"_ustr,
                R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:characterSpacingControl w:val="compressPunctuation"/><w:compat><w:adjustLineHeightInTable/><w:doNotWrapTextWithPunct/><w:doNotUseEastAsianBreakRules/><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val=")xml"_ostr
                + OString::number(nCompat) + R"xml("/></w:compat></w:settings>)xml"_ostr);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        for (int nRound = 0; nRound < 2; ++nRound)
        {
            if (!nRound)
                loadFromURL(aFile.GetURL());
            else
                saveAndReload(TestFilter::DOCX);
            auto pDump = parseLayoutDump();
            // Measured Mac Word 16.114: a prior colon and comma can
            // contract together with an opening delimiter and its first Han.
            // Modern left/center and the single-delimiter control stay bounded.
            const OUString aExpected = nToken != 2 && (nCompat == 14 || rAlign == "both")
                ? aPrefix + aOpen + u"卯"_ustr : aPrefix;
            assertXPath(pDump, "/root/page/body/tab/row/cell/txt/SwParaPortion/SwLineLayout[1]",
                "portion", aExpected);
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacTableLastLineGap)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
        const OUString aOn = u"1"_ustr;
        osl_setEnvironment(rName.pData, aOn.pData);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    for (int nCompat : {14, 15})
    for (const OString& rFamily : {"SimSun"_ostr, "FangSong_GB2312"_ostr})
    for (int nMark : {0, 20, 24})
    for (bool bBold : {false, true})
    for (int nProp : {100, 125, 150})
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        const auto aFont = [](const OString& rName, int nSize, bool bWeight) -> OString {
            return R"xml(<w:rFonts w:ascii=")xml"_ostr + rName + R"xml(" w:hAnsi=")xml"_ostr
                + rName + R"xml(" w:eastAsia=")xml"_ostr + rName + R"xml("/><w:sz w:val=")xml"_ostr
                + OString::number(nSize) + "\"/>"_ostr + (bWeight ? "<w:b/>"_ostr : OString());
        };
        const OString aBorders = R"xml(<w:top w:val="single" w:sz="4"/><w:left w:val="single" w:sz="4"/><w:bottom w:val="single" w:sz="4"/><w:right w:val="single" w:sz="4"/><w:insideH w:val="single" w:sz="4"/><w:insideV w:val="single" w:sz="4"/>)xml"_ostr;
        OString aRows;
        for (int nLines : {1, 2, 3})
        {
            OString aText;
            for (int i = 0; i < nLines; ++i)
                aText += (i ? "<w:br/>"_ostr : OString()) + u"<w:t>甲乙丙丁戊己</w:t>"_ustr.toUtf8();
            const OString aMark = nMark
                ? OString("<w:rPr>"_ostr + aFont(rFamily, nMark, bBold) + "</w:rPr>"_ostr) : OString();
            aRows += R"xml(<w:tr><w:tc><w:tcPr><w:tcW w:type="dxa" w:w="2400"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line=")xml"_ostr
                + OString::number(nProp * 240 / 100) + R"xml(" w:lineRule="auto"/><w:snapToGrid w:val="0"/>)xml"_ostr
                + aMark + "</w:pPr><w:r><w:rPr>"_ostr + aFont(rFamily, 20, bBold)
                + "</w:rPr>"_ostr + aText + "</w:r></w:p></w:tc></w:tr>"_ostr;
        }
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            writeWordMacPart(xWord, u"document.xml"_ustr,
                R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:type="dxa" w:w="2400"/><w:tblLayout w:type="fixed"/><w:tblBorders>)xml"_ostr
                + aBorders + R"xml(</w:tblBorders><w:tblCellMar><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/><w:left w:w="108" w:type="dxa"/><w:right w:w="108" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="2400"/></w:tblGrid>)xml"_ostr
                + aRows + R"xml(</w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr);
            writeWordMacPart(xWord, u"styles.xml"_ustr,
                R"xml(<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/><w:rPr>)xml"_ostr
                + aFont("SimSun"_ostr, 24, false) + R"xml(</w:rPr></w:style></w:styles>)xml"_ostr);
            writeWordMacPart(xWord, u"settings.xml"_ustr,
                R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:compat><w:doNotWrapTextWithPunct/><w:doNotUseEastAsianBreakRules/><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val=")xml"_ostr
                + OString::number(nCompat) + R"xml("/></w:compat></w:settings>)xml"_ostr);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        const double aWordHeights[3][3] = {
            {13.44, 26.40, 39.60}, {16.80, 32.88, 49.20}, {19.92, 39.36, 59.04}
        };
        for (int nRound = 0; nRound < 2; ++nRound)
        {
            if (!nRound)
                loadFromURL(aFile.GetURL());
            else
                saveAndReload(TestFilter::DOCX);
            auto pDump = parseLayoutDump();
            for (int i = 0; i < 3; ++i)
            {
                const OString aPath = "/root/page/body/tab/row["_ostr + OString::number(i + 1) + "]/infos/bounds"_ostr;
                const double fHeight = getXPath(pDump, aPath, "height").toInt32() / 20.0;
                // 72 independent repaginated Mac Word controls: frame edges
                // differ from painted border centers by at most a few twips.
                CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("Mac Word cell row height",
                    aWordHeights[(nProp - 100) / 25][i], fHeight, 0.20);
            }
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacTableProportionalPageEnd)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
        const OUString aOn = u"1"_ustr;
        osl_setEnvironment(rName.pData, aOn.pData);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    for (const OString& rFamily : {"SimSun"_ostr, "FangSong_GB2312"_ostr})
    for (int nRemaining : {1540, 1570, 1580, 1600, 1620, 1640, 1660, 1800})
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        OString aText;
        for (int i = 0; i < 6; ++i)
            aText += (i ? "<w:br/>"_ostr : OString()) + u"<w:t>甲乙丙丁戊己</w:t>"_ustr.toUtf8();
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            writeWordMacPart(xWord, u"document.xml"_ustr,
                R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line=")xml"_ostr
                + OString::number(13958 - nRemaining)
                + R"xml(" w:lineRule="exact"/></w:pPr><w:r><w:t>Page-end control</w:t></w:r></w:p><w:tbl><w:tblPr><w:tblW w:type="dxa" w:w="4800"/><w:tblLayout w:type="fixed"/><w:tblBorders><w:top w:val="single" w:sz="4"/><w:left w:val="single" w:sz="4"/><w:bottom w:val="single" w:sz="4"/><w:right w:val="single" w:sz="4"/></w:tblBorders><w:tblCellMar><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="4800"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:type="dxa" w:w="4800"/></w:tcPr><w:p><w:pPr><w:widowControl w:val="0"/><w:spacing w:before="0" w:after="0" w:line="300" w:lineRule="auto"/><w:snapToGrid w:val="0"/></w:pPr><w:r><w:rPr><w:rFonts w:ascii=")xml"_ostr
                + rFamily + R"xml(" w:hAnsi=")xml"_ostr + rFamily
                + R"xml(" w:eastAsia=")xml"_ostr + rFamily
                + R"xml("/><w:sz w:val="20"/></w:rPr>)xml"_ostr + aText
                + R"xml(</w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr);
            writeWordMacPart(xWord, u"styles.xml"_ustr,
                R"xml(<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/><w:rPr><w:rFonts w:ascii="SimSun" w:hAnsi="SimSun" w:eastAsia="SimSun"/><w:sz w:val="24"/></w:rPr></w:style></w:styles>)xml"_ostr);
            writeWordMacPart(xWord, u"settings.xml"_ustr,
                R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:compat><w:doNotWrapTextWithPunct/><w:doNotUseEastAsianBreakRules/><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val="15"/></w:compat></w:settings>)xml"_ostr);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        for (int nRound = 0; nRound < 2; ++nRound)
        {
            if (!nRound)
                loadFromURL(aFile.GetURL());
            else
                saveAndReload(TestFilter::DOCX);
            auto pDump = parseLayoutDump();
            // Independent Mac Word exports retain the full proportional
            // pitch at the page end: a fifth line needs at least 83 pt.
            assertXPath(pDump, "/root/page", 2);
            assertXPath(pDump,
                "/root/page[1]/body/tab/row/cell/txt/SwParaPortion/SwLineLayout",
                nRemaining < 1660 ? 4 : 5);
        }
    }
}

CPPUNIT_TEST_FIXTURE(SwLayoutWriter5, testWordMacTableWrappedLatinPitch)
{
    const std::vector<OUString> aNames{u"COCOLIAN_WORD_MAC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_CLASSIC_METRICS"_ustr,
        u"COCOLIAN_WORD_MAC_FLEX_SCRIPT_GAPS"_ustr,
        u"COCOLIAN_WORD_MAC_LINE_BUDGET"_ustr};
    std::vector<std::pair<bool, OUString>> aPrevious;
    for (const auto& rName : aNames)
    {
        OUString aValue;
        const bool bSet = osl_getEnvironment(rName.pData, &aValue.pData) == osl_Process_E_None;
        aPrevious.emplace_back(bSet, aValue);
        const OUString aOn = u"1"_ustr;
        osl_setEnvironment(rName.pData, aOn.pData);
    }
    comphelper::ScopeGuard aRestore([&] {
        for (size_t i = 0; i < aNames.size(); ++i)
            if (aPrevious[i].first)
                osl_setEnvironment(aNames[i].pData, aPrevious[i].second.pData);
            else
                osl_clearEnvironment(aNames[i].pData);
    });
    for (int nCompat : {14, 15})
    for (const OString& rFont : {"SimSun"_ostr, "FangSong_GB2312"_ostr})
    for (bool bManual : {false, true})
    {
        utl::TempFileNamed aFile;
        aFile.EnableKillingFile();
        makeWordMacPackage(aFile, false, true);
        {
            auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
                ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
            auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
            OString aRuns = bManual
                ? u"<w:t>甲乙丙丁戊己</w:t><w:br/><w:t>example_software</w:t><w:br/><w:t>release_2026</w:t>"_ustr.toUtf8()
                : u"<w:t>样例内容版本至example_software_build_v1.0.20260101</w:t>"_ustr.toUtf8();
            writeWordMacPart(xWord, u"document.xml"_ustr,
                R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:type="dxa" w:w="2400"/><w:tblLayout w:type="fixed"/><w:tblCellMar><w:top w:w="0" w:type="dxa"/><w:bottom w:w="0" w:type="dxa"/></w:tblCellMar></w:tblPr><w:tblGrid><w:gridCol w:w="2400"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:type="dxa" w:w="2400"/></w:tcPr><w:p><w:pPr><w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="auto"/><w:jc w:val="center"/></w:pPr><w:r><w:rPr><w:rFonts w:hint="eastAsia" w:ascii=")xml"_ostr + rFont
                + R"xml(" w:hAnsi=")xml"_ostr + rFont + R"xml(" w:eastAsia=")xml"_ostr + rFont
                + R"xml("/><w:sz w:val="24"/><w:szCs w:val="24"/></w:rPr>)xml"_ostr + aRuns
                + R"xml(</w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr><w:pgSz w:w="11906" w:h="16838"/><w:pgMar w:top="1440" w:bottom="1440" w:left="1440" w:right="1440"/></w:sectPr></w:body></w:document>)xml"_ostr);
            writeWordMacPart(xWord, u"styles.xml"_ustr,
                R"xml(<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/><w:rPr><w:sz w:val="21"/></w:rPr></w:style></w:styles>)xml"_ostr);
            writeWordMacPart(xWord, u"settings.xml"_ustr,
                R"xml(<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:compat><w:useFELayout/><w:compatSetting w:name="compatibilityMode" w:uri="http://schemas.microsoft.com/office/word" w:val=")xml"_ostr + OString::number(nCompat) + R"xml("/></w:compat></w:settings>)xml"_ostr);
            uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
            uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
        }
        for (int nRound = 0; nRound < 2; ++nRound)
        {
            if (!nRound)
                loadFromURL(aFile.GetURL());
            else
                saveAndReload(TestFilter::DOCX);
            auto xMeta = getSwDocShell()->GetPreviewMetaFile();
            MetafileXmlDump aDumper;
            auto pMeta = dumpAndParse(aDumper, *xMeta);
            const int nFirst = getXPath(pMeta, "(//textarray)[1]", "y").toInt32();
            const int nMiddle = getXPath(pMeta, "(//textarray)[2]", "y").toInt32();
            const int nLast = getXPath(pMeta, "(//textarray)[last()]", "y").toInt32();
            CPPUNIT_ASSERT_MESSAGE("Word table first-to-second line pitch",
                std::abs(nMiddle - nFirst - 312) <= 8);
            CPPUNIT_ASSERT_MESSAGE("Word table second-to-third line pitch",
                std::abs(nLast - nMiddle - 312) <= 8);
            // Word's actual mixed-script table PDFs retain 15.6pt between
            // successive 12pt lines, including automatic ASCII wraps.
            CPPUNIT_ASSERT_MESSAGE("Word table CJK/Latin three-line pitch",
                std::abs(nLast - nFirst - 624) <= 8);
        }
    }
}
