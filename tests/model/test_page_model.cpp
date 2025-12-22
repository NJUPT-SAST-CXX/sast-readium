#include <QSignalSpy>
#include <QTest>
#include "../../app/model/PageModel.h"
#include "../TestUtilities.h"

class TestPageModel : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() { m_model = new PageModel(10); }

    void cleanup() {
        delete m_model;
        m_model = nullptr;
    }

    void testConstruction() {
        QVERIFY(m_model != nullptr);
        QCOMPARE(m_model->currentPage(), 1);
        QCOMPARE(m_model->totalPages(), 10);
    }

    void testConstructionWithDifferentPageCount() {
        PageModel model1(1);
        QCOMPARE(model1.totalPages(), 1);

        PageModel model2(100);
        QCOMPARE(model2.totalPages(), 100);

        PageModel model3(0);
        QCOMPARE(model3.totalPages(), 0);
    }

    void testSetCurrentPage() {
        QSignalSpy spy(m_model, &PageModel::pageUpdate);

        m_model->setCurrentPage(5);
        QCOMPARE(m_model->currentPage(), 5);
        QCOMPARE(spy.count(), 1);

        m_model->setCurrentPage(1);
        QCOMPARE(m_model->currentPage(), 1);
    }

    void testSetCurrentPageBoundary() {
        m_model->setCurrentPage(0);
        QVERIFY(m_model->currentPage() >= 1);

        m_model->setCurrentPage(100);
        QVERIFY(m_model->currentPage() <= m_model->totalPages());
    }

    void testNextPage() {
        m_model->setCurrentPage(1);
        m_model->nextPage();
        QCOMPARE(m_model->currentPage(), 2);

        m_model->nextPage();
        QCOMPARE(m_model->currentPage(), 3);
    }

    void testNextPageAtEnd() {
        m_model->setCurrentPage(10);
        int pageBefore = m_model->currentPage();
        m_model->nextPage();
        QCOMPARE(m_model->currentPage(), pageBefore);
    }

    void testPrevPage() {
        m_model->setCurrentPage(5);
        m_model->prevPage();
        QCOMPARE(m_model->currentPage(), 4);

        m_model->prevPage();
        QCOMPARE(m_model->currentPage(), 3);
    }

    void testPrevPageAtStart() {
        m_model->setCurrentPage(1);
        m_model->prevPage();
        QCOMPARE(m_model->currentPage(), 1);
    }

    void testGoToPage() {
        QSignalSpy spy(m_model, &PageModel::pageChanged);

        bool result = m_model->goToPage(7);
        QVERIFY(result);
        QCOMPARE(m_model->currentPage(), 7);
        QCOMPARE(spy.count(), 1);
    }

    void testGoToInvalidPage() {
        bool result1 = m_model->goToPage(0);
        QVERIFY(!result1);

        bool result2 = m_model->goToPage(100);
        QVERIFY(!result2);

        bool result3 = m_model->goToPage(-5);
        QVERIFY(!result3);
    }

    void testGoToFirstPage() {
        m_model->setCurrentPage(5);
        bool result = m_model->goToFirstPage();
        QVERIFY(result);
        QCOMPARE(m_model->currentPage(), 1);
    }

    void testGoToLastPage() {
        m_model->setCurrentPage(1);
        bool result = m_model->goToLastPage();
        QVERIFY(result);
        QCOMPARE(m_model->currentPage(), 10);
    }

    void testValidatePage() {
        PageValidationResult result1 = m_model->validatePage(5);
        QCOMPARE(result1, PageValidationResult::Valid);

        PageValidationResult result2 = m_model->validatePage(0);
        QCOMPARE(result2, PageValidationResult::InvalidPageNumber);

        PageValidationResult result3 = m_model->validatePage(100);
        QCOMPARE(result3, PageValidationResult::InvalidPageNumber);
    }

    void testIsValidPage() {
        QVERIFY(m_model->isValidPage(1));
        QVERIFY(m_model->isValidPage(5));
        QVERIFY(m_model->isValidPage(10));
        QVERIFY(!m_model->isValidPage(0));
        QVERIFY(!m_model->isValidPage(11));
        QVERIFY(!m_model->isValidPage(-1));
    }

    void testGetValidationErrorMessage() {
        QString validMsg =
            m_model->getValidationErrorMessage(PageValidationResult::Valid);
        QVERIFY(validMsg.isEmpty() ||
                validMsg.contains("valid", Qt::CaseInsensitive));

        QString invalidMsg = m_model->getValidationErrorMessage(
            PageValidationResult::InvalidPageNumber);
        QVERIFY(!invalidMsg.isEmpty());
    }

    void testPageMetadata() {
        PageMetadata metadata = m_model->getPageMetadata(1);
        QCOMPARE(metadata.pageNumber, 1);
    }

    void testGetPageSize() { QSizeF size = m_model->getPageSize(1); }

    void testGetPageRotation() {
        double rotation = m_model->getPageRotation(1);
        QVERIFY(rotation >= 0.0 || rotation == 0.0);
    }

    void testIsPageLoaded() { bool loaded = m_model->isPageLoaded(1); }

    void testPreloadSettings() {
        m_model->setPreloadEnabled(true);
        QVERIFY(m_model->isPreloadEnabled());

        m_model->setPreloadEnabled(false);
        QVERIFY(!m_model->isPreloadEnabled());

        m_model->setPreloadRadius(3);
        QCOMPARE(m_model->preloadRadius(), 3);

        m_model->setPreloadRadius(5);
        QCOMPARE(m_model->preloadRadius(), 5);
    }

    void testPreloadPage() {
        m_model->setPreloadEnabled(true);
        m_model->preloadPage(5);
    }

    void testPreloadPages() {
        m_model->setPreloadEnabled(true);
        m_model->preloadPages({1, 2, 3, 4, 5});
    }

    void testPreloadAdjacentPages() {
        m_model->setPreloadEnabled(true);
        m_model->preloadAdjacentPages(5, 2);
    }

    void testClearPageCache() {
        m_model->clearPageCache();
        m_model->clearPageFromCache(1);
    }

    void testRenderModel() {
        QVERIFY(!m_model->hasRenderModel());
        QVERIFY(m_model->getRenderModel() == nullptr);

        m_model->setRenderModel(nullptr);
        QVERIFY(!m_model->hasRenderModel());
    }

    void testDocumentState() {
        QVERIFY(!m_model->hasDocument());
        QString error = m_model->getLastError();
    }

    void testStatistics() {
        int cacheSize = m_model->getCacheSize();
        QVERIFY(cacheSize >= 0);

        int preloadedCount = m_model->getPreloadedPagesCount();
        QVERIFY(preloadedCount >= 0);

        QList<int> preloadedPages = m_model->getPreloadedPages();

        double avgLoadTime = m_model->getAveragePageLoadTime();
        QVERIFY(avgLoadTime >= 0.0);
    }

    void testPageChangedSignal() {
        QSignalSpy spy(m_model, &PageModel::pageChanged);

        m_model->setCurrentPage(1);
        m_model->goToPage(5);

        QVERIFY(spy.count() >= 1);
        QList<QVariant> args = spy.last();
        QCOMPARE(args.at(0).toInt(), 5);
    }

    void testPageUpdateSignal() {
        QSignalSpy spy(m_model, &PageModel::pageUpdate);

        m_model->setCurrentPage(3);

        QCOMPARE(spy.count(), 1);
        QList<QVariant> args = spy.first();
        QCOMPARE(args.at(0).toInt(), 3);
        QCOMPARE(args.at(1).toInt(), 10);
    }

    void testPageMetadataStruct() {
        PageMetadata metadata1;
        QCOMPARE(metadata1.pageNumber, 0);
        QCOMPARE(metadata1.rotation, 0.0);
        QVERIFY(!metadata1.isLoaded);

        PageMetadata metadata2(5, QSizeF(612, 792), 90.0);
        QCOMPARE(metadata2.pageNumber, 5);
        QCOMPARE(metadata2.pageSize, QSizeF(612, 792));
        QCOMPARE(metadata2.rotation, 90.0);
    }

    void testNavigationSequence() {
        m_model->setCurrentPage(1);
        QCOMPARE(m_model->currentPage(), 1);

        m_model->nextPage();
        QCOMPARE(m_model->currentPage(), 2);

        m_model->nextPage();
        m_model->nextPage();
        QCOMPARE(m_model->currentPage(), 4);

        m_model->prevPage();
        QCOMPARE(m_model->currentPage(), 3);

        m_model->goToLastPage();
        QCOMPARE(m_model->currentPage(), 10);

        m_model->goToFirstPage();
        QCOMPARE(m_model->currentPage(), 1);
    }

private:
    PageModel* m_model = nullptr;
};

QTEST_MAIN(TestPageModel)
#include "test_page_model.moc"
