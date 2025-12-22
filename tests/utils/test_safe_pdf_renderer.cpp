#include <QImage>
#include <QTest>
#include "../../app/utils/SafePDFRenderer.h"
#include "../TestUtilities.h"

class TestSafePDFRenderer : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void testSingleton() {
        SafePDFRenderer& instance1 = SafePDFRenderer::instance();
        SafePDFRenderer& instance2 = SafePDFRenderer::instance();
        QVERIFY(&instance1 == &instance2);
    }

    void testRenderPageWithNull() {
        SafePDFRenderer& renderer = SafePDFRenderer::instance();

        QImage image = renderer.renderPage(nullptr, 100, 100, 1.0);
        QVERIFY(image.isNull());
    }

    void testRenderPageWithInvalidSize() {
        SafePDFRenderer& renderer = SafePDFRenderer::instance();

        QImage image1 = renderer.renderPage(nullptr, 0, 100, 1.0);
        QVERIFY(image1.isNull());

        QImage image2 = renderer.renderPage(nullptr, 100, 0, 1.0);
        QVERIFY(image2.isNull());

        QImage image3 = renderer.renderPage(nullptr, -100, -100, 1.0);
        QVERIFY(image3.isNull());
    }

    void testRenderPageWithZeroScale() {
        SafePDFRenderer& renderer = SafePDFRenderer::instance();

        QImage image = renderer.renderPage(nullptr, 100, 100, 0.0);
        QVERIFY(image.isNull());
    }

    void testRenderWithTestPdf() {
        auto* doc = TestDataGenerator::createTestPdfWithoutText(1);
        if (!doc) {
            QSKIP("Could not create test PDF");
            return;
        }

        std::unique_ptr<Poppler::Page> page(doc->page(0));
        if (!page) {
            delete doc;
            QSKIP("Could not get page from test PDF");
            return;
        }

        SafePDFRenderer& renderer = SafePDFRenderer::instance();

        QImage image = renderer.renderPage(page.get(), 200, 300, 1.0);

        delete doc;
    }

    void testRenderWithDifferentScales() {
        auto* doc = TestDataGenerator::createTestPdfWithoutText(1);
        if (!doc) {
            QSKIP("Could not create test PDF");
            return;
        }

        std::unique_ptr<Poppler::Page> page(doc->page(0));
        if (!page) {
            delete doc;
            QSKIP("Could not get page from test PDF");
            return;
        }

        SafePDFRenderer& renderer = SafePDFRenderer::instance();

        renderer.renderPage(page.get(), 100, 150, 0.5);
        renderer.renderPage(page.get(), 200, 300, 1.0);
        renderer.renderPage(page.get(), 400, 600, 2.0);

        delete doc;
    }

    void testRenderTimeout() {
        SafePDFRenderer& renderer = SafePDFRenderer::instance();

        renderer.setTimeout(5000);
        QCOMPARE(renderer.getTimeout(), 5000);

        renderer.setTimeout(10000);
        QCOMPARE(renderer.getTimeout(), 10000);
    }

    void testRenderMultiplePages() {
        auto* doc = TestDataGenerator::createTestPdfWithoutText(3);
        if (!doc) {
            QSKIP("Could not create test PDF");
            return;
        }

        SafePDFRenderer& renderer = SafePDFRenderer::instance();

        for (int i = 0; i < doc->numPages(); ++i) {
            std::unique_ptr<Poppler::Page> page(doc->page(i));
            if (page) {
                renderer.renderPage(page.get(), 200, 300, 1.0);
            }
        }

        delete doc;
    }

    void testConcurrentRendering() {
        auto* doc = TestDataGenerator::createTestPdfWithoutText(1);
        if (!doc) {
            QSKIP("Could not create test PDF");
            return;
        }

        std::unique_ptr<Poppler::Page> page(doc->page(0));
        if (!page) {
            delete doc;
            QSKIP("Could not get page from test PDF");
            return;
        }

        SafePDFRenderer& renderer = SafePDFRenderer::instance();

        for (int i = 0; i < 10; ++i) {
            renderer.renderPage(page.get(), 100, 150, 1.0);
        }

        delete doc;
    }

    void testIsRendering() {
        SafePDFRenderer& renderer = SafePDFRenderer::instance();
        bool rendering = renderer.isRendering();
    }

    void testCancelRendering() {
        SafePDFRenderer& renderer = SafePDFRenderer::instance();
        renderer.cancelRendering();
    }
};

QTEST_MAIN(TestSafePDFRenderer)
#include "test_safe_pdf_renderer.moc"
