/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the Collabora Office project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <swmodeltestbase.hxx>
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
    utl::TempFileNamed aFile;
    aFile.EnableKillingFile();
    makeWordMacPackage(aFile, false, true);
    {
        auto xRoot = comphelper::OStorageHelper::GetStorageOfFormatFromURL(
            ZIP_STORAGE_FORMAT_STRING, aFile.GetURL(), embed::ElementModes::READWRITE);
        auto xWord = xRoot->openStorageElement(u"word"_ustr, embed::ElementModes::WRITE);
        writeWordMacPart(xWord, u"document.xml"_ustr,
            R"xml(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:tbl><w:tblPr><w:tblW w:w="4998" w:type="pct"/><w:tblLayout w:type="fixed"/></w:tblPr><w:tblGrid><w:gridCol w:w="4500"/></w:tblGrid><w:tr><w:tc><w:tcPr><w:tcW w:w="4500" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>Width</w:t></w:r></w:p></w:tc></w:tr></w:tbl><w:p/><w:sectPr/></w:body></w:document>)xml"_ostr);
        uno::Reference<embed::XTransactedObject>(xWord, uno::UNO_QUERY_THROW)->commit();
        uno::Reference<embed::XTransactedObject>(xRoot, uno::UNO_QUERY_THROW)->commit();
    }
    for (bool bMac : {false, true})
    {
        const OUString aValue = bMac ? u"1"_ustr : u"0"_ustr;
        osl_setEnvironment(aName.pData, aValue.pData);
        loadFromURL(aFile.GetURL());
        uno::Reference<text::XTextTablesSupplier> xSupplier(mxComponent, uno::UNO_QUERY_THROW);
        uno::Reference<container::XIndexAccess> xTables(xSupplier->getTextTables(), uno::UNO_QUERY_THROW);
        uno::Reference<text::XTextTable> xTable(xTables->getByIndex(0), uno::UNO_QUERY_THROW);
        CPPUNIT_ASSERT_EQUAL(sal_Int16(bMac ? 100 : 99),
                             getProperty<sal_Int16>(xTable, u"RelativeWidth"_ustr));
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
        // Word's 9pt YaHei fits one grid row and retains 150% leading after
        // the first line. 10pt occupies two rows and ignores that multiplier.
        assertXPath(parseLayoutDump(), "/root/page[1]/body/txt[1]/infos/bounds",
                    "height", nSize == 9 ? u"1248" : u"1872");
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

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
