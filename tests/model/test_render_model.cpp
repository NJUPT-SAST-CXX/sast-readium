#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "../../app/model/RenderModel.h"
#include "../TestUtilities.h"

#include <poppler/qt6/poppler-qt6.h>

class RenderModelTest : public TestBase {
    Q_OBJECT

private slots:
    void test_dpi_management_and_effective();
    void test_render_quality_and_signals();
    void test_document_validation_and_pages();
    void test_cache_management();
    void test_async_rendering_lifecycle();
};

void RenderModelTest::test_dpi_management_and_effective() {
    RenderModel rm;
    QSignalSpy dpiSpy(&rm, &RenderModel::dpiChanged);

    rm.setDpi(96, 96);
    QCOMPARE(rm.getDpiX(), 96.0);
    QCOMPARE(rm.getDpiY(), 96.0);
    QVERIFY(dpiSpy.count() >= 1);

    // Ensure quality multiplier is 1.0 to make effective == base
    rm.setRenderQuality(RenderModel::RenderQuality::Normal);
    QCOMPARE(rm.getEffectiveDpiX(), rm.getDpiX());
    QCOMPARE(rm.getEffectiveDpiY(), rm.getDpiY());
}

void RenderModelTest::test_render_quality_and_signals() {
    RenderModel rm;
    QSignalSpy qSpy(&rm, &RenderModel::renderQualityChanged);
    rm.setRenderQuality(
        RenderModel::RenderQuality::Draft);  // change from default High
    qSpy.clear();
    rm.setRenderQuality(RenderModel::RenderQuality::High);
    QCOMPARE(rm.getRenderQuality(), RenderModel::RenderQuality::High);
    QVERIFY(qSpy.count() == 1);
}

void RenderModelTest::test_document_validation_and_pages() {
    // Create a small test PDF
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString pdf = tmp.filePath("doc.pdf");
    TestDataGenerator::createTestPdfWithoutText(3, pdf);

    std::unique_ptr<Poppler::Document> doc(Poppler::Document::load(pdf));
    QVERIFY(doc != nullptr);

    RenderModel rm;
    QSignalSpy docChanged(&rm, &RenderModel::documentChanged);
    rm.setDocument(doc.get());
    QVERIFY(docChanged.count() == 1);

    QVERIFY(rm.isDocumentValid());
    QCOMPARE(rm.getPageCount(), 3);
    QVERIFY(rm.hasPage(0));
    QVERIFY(!rm.hasPage(999));
    QVERIFY(!rm.getLastError().isEmpty());  // last error set by failed hasPage
    QVERIFY(rm.hasPage(0));  // successful call should clear lastError
    QVERIFY(rm.getLastError().isEmpty());
}

void RenderModelTest::test_cache_management() {
    QTemporaryDir tmp;
    const QString pdf = tmp.filePath("doc.pdf");
    TestDataGenerator::createTestPdfWithoutText(2, pdf);
    std::unique_ptr<Poppler::Document> doc(Poppler::Document::load(pdf));

    RenderModel rm;
    rm.setDocument(doc.get());

    rm.setMaxCacheSize(1);
    QCOMPARE(rm.getMaxCacheSize(), 1);
    // Increase cache size to ensure rendered page fits into cache on typical A4
    // at 72 DPI
    rm.setMaxCacheSize(100);

    QVERIFY(!rm.isPageCached(0));
    auto img = rm.renderPage(0, 72.0, 72.0);
    QVERIFY(!img.isNull());
    QVERIFY(rm.isPageCached(0));

    rm.clearPageFromCache(0);
    QVERIFY(!rm.isPageCached(0));

    rm.renderPage(0);
    QVERIFY(rm.isPageCached(0));
    rm.clearCache();
    QVERIFY(!rm.isPageCached(0));
}

void RenderModelTest::test_async_rendering_lifecycle() {
    QTemporaryDir tmp;
    const QString pdf = tmp.filePath("doc.pdf");
    TestDataGenerator::createTestPdfWithoutText(2, pdf);
    std::unique_ptr<Poppler::Document> doc(Poppler::Document::load(pdf));

    RenderModel rm;
    rm.setDocument(doc.get());

    QSignalSpy doneSpy(&rm, &RenderModel::asyncRenderCompleted);
    rm.renderPageAsync(0);
    QVERIFY(doneSpy.wait(2000));
    // Expect a tuple (pageNum, image)
    const auto args = doneSpy.takeFirst();
    QCOMPARE(args.at(0).toInt(), 0);
    QVERIFY(args.at(1).value<QImage>().isNull() == false);
}

QTEST_MAIN(RenderModelTest)
#include "test_render_model.moc"
