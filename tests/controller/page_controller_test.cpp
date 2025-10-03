#include <QApplication>
#include <QSignalSpy>
#include <QTest>
#include "../../app/controller/PageController.h"
#include "../../app/model/PageModel.h"
#include "../TestUtilities.h"

// Mock PageModel for testing
class MockPageModel : public PageModel {
    Q_OBJECT
public:
    explicit MockPageModel(QObject* parent = nullptr) : PageModel(10, parent) {}

    // Override validation methods to bypass render model requirements for
    // testing
    PageValidationResult validatePage(int pageNum) const override {
        if (pageNum < 1 || pageNum > totalPages()) {
            return PageValidationResult::InvalidPageNumber;
        }
        return PageValidationResult::Valid;
    }

    bool hasDocument() const override { return totalPages() > 0; }

    bool isDocumentValid() const override { return hasDocument(); }

    // Additional method for testing
    void setTotalPages(int total) {
        // This is a test-specific method to change total pages
        // In real PageModel, total pages are set through document loading
        if (total > 0) {
            _totalPages = total;
            // Ensure current page is within bounds
            if (_currentPage > _totalPages) {
                _currentPage = _totalPages;
            }
            emit pageUpdate(_currentPage, _totalPages);
        }
    }
};

class PageControllerTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override {
        // Ensure QApplication exists for widget testing
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            new QApplication(argc, argv);
        }
    }

    void init() override {
        // Create fresh mock objects for each test
        m_mockPageModel = new MockPageModel(this);
        m_pageController = new PageController(m_mockPageModel, this);
    }

    void cleanup() override {
        // Objects will be cleaned up by Qt parent-child relationship
        m_pageController = nullptr;
        m_mockPageModel = nullptr;
    }

    // Constructor tests
    void testConstructorWithValidModel() {
        QVERIFY(m_pageController != nullptr);
        QVERIFY(m_pageController->getModel() == m_mockPageModel);
        QCOMPARE(m_pageController->getCurrentPage(), 1);
        QCOMPARE(m_pageController->getTotalPages(), 10);
    }

    void testConstructorWithNullModel() {
        PageController* controller = new PageController(nullptr, this);
        QVERIFY(controller != nullptr);
        QVERIFY(controller->getModel() == nullptr);

        // Operations with null model should handle gracefully
        QCOMPARE(controller->getCurrentPage(), 0);
        QCOMPARE(controller->getTotalPages(), 0);
    }

    // Basic navigation tests
    void testGoToNextPage() {
        QSignalSpy pageChangedSpy(m_pageController,
                                  &PageController::pageChanged);

        int initialPage = m_pageController->getCurrentPage();
        m_pageController->goToNextPage();

        // Should advance to next page
        QCOMPARE(m_pageController->getCurrentPage(), initialPage + 1);
        QCOMPARE(pageChangedSpy.count(), 1);
    }

    void testGoToPrevPage() {
        // First go to page 5
        m_pageController->goToPage(5);

        QSignalSpy pageChangedSpy(m_pageController,
                                  &PageController::pageChanged);

        int initialPage = m_pageController->getCurrentPage();
        m_pageController->goToPrevPage();

        // Should go to previous page
        QCOMPARE(m_pageController->getCurrentPage(), initialPage - 1);
        QCOMPARE(pageChangedSpy.count(), 1);
    }

    void testGoToNextPageAtEnd() {
        // Go to last page first
        m_pageController->goToLastPage();

        QSignalSpy pageChangedSpy(m_pageController,
                                  &PageController::pageChanged);

        m_pageController->goToNextPage();

        // Should wrap to first page
        QCOMPARE(m_pageController->getCurrentPage(), 1);
        QCOMPARE(pageChangedSpy.count(), 1);
    }

    void testGoToPrevPageAtBeginning() {
        // Start at first page
        m_pageController->goToFirstPage();

        QSignalSpy pageChangedSpy(m_pageController,
                                  &PageController::pageChanged);

        m_pageController->goToPrevPage();

        // Should wrap to last page
        QCOMPARE(m_pageController->getCurrentPage(), 10);
        QCOMPARE(pageChangedSpy.count(), 1);
    }

    void testGoToPage() {
        QSignalSpy pageChangedSpy(m_pageController,
                                  &PageController::pageChanged);

        m_pageController->goToPage(5);

        QCOMPARE(m_pageController->getCurrentPage(), 5);
        QCOMPARE(pageChangedSpy.count(), 1);
    }

    void testGoToInvalidPage() {
        QSignalSpy errorSpy(m_pageController, &PageController::errorOccurred);

        int initialPage = m_pageController->getCurrentPage();

        // Try to go to invalid pages
        m_pageController->goToPage(0);
        m_pageController->goToPage(-1);
        m_pageController->goToPage(100);

        // Page should not change
        QCOMPARE(m_pageController->getCurrentPage(), initialPage);

        // Should emit error signals
        QVERIFY(errorSpy.count() >= 3);
    }

    void testGoToFirstPage() {
        // Start at middle page
        m_pageController->goToPage(5);

        QSignalSpy pageChangedSpy(m_pageController,
                                  &PageController::pageChanged);

        m_pageController->goToFirstPage();

        QCOMPARE(m_pageController->getCurrentPage(), 1);
        QCOMPARE(pageChangedSpy.count(), 1);
    }

    void testGoToLastPage() {
        QSignalSpy pageChangedSpy(m_pageController,
                                  &PageController::pageChanged);

        m_pageController->goToLastPage();

        QCOMPARE(m_pageController->getCurrentPage(), 10);
        QCOMPARE(pageChangedSpy.count(), 1);
    }

    // Page validation tests
    void testIsValidPage() {
        QVERIFY(m_pageController->isValidPage(1));
        QVERIFY(m_pageController->isValidPage(5));
        QVERIFY(m_pageController->isValidPage(10));

        QVERIFY(!m_pageController->isValidPage(0));
        QVERIFY(!m_pageController->isValidPage(-1));
        QVERIFY(!m_pageController->isValidPage(11));
        QVERIFY(!m_pageController->isValidPage(100));
    }

    // History management tests
    void testNavigationHistory() {
        // Navigate to build history
        m_pageController->goToPage(3);
        m_pageController->goToPage(7);
        m_pageController->goToPage(5);

        // Should be able to go back
        QVERIFY(m_pageController->canGoBack());
        QVERIFY(!m_pageController->canGoForward());

        QSignalSpy navStateSpy(m_pageController,
                               &PageController::navigationStateChanged);

        // Go back
        m_pageController->goBack();
        QCOMPARE(m_pageController->getCurrentPage(), 7);
        QVERIFY(m_pageController->canGoBack());
        QVERIFY(m_pageController->canGoForward());

        // Go back again
        m_pageController->goBack();
        QCOMPARE(m_pageController->getCurrentPage(), 3);

        // Go forward
        m_pageController->goForward();
        QCOMPARE(m_pageController->getCurrentPage(), 7);

        // Navigation state should have changed
        QVERIFY(navStateSpy.count() > 0);
    }

    void testClearHistory() {
        // Build some history
        m_pageController->goToPage(3);
        m_pageController->goToPage(7);

        QVERIFY(m_pageController->canGoBack());

        QSignalSpy navStateSpy(m_pageController,
                               &PageController::navigationStateChanged);

        m_pageController->clearHistory();

        QVERIFY(!m_pageController->canGoBack());
        QVERIFY(!m_pageController->canGoForward());
        QCOMPARE(navStateSpy.count(), 1);
    }

    // Bookmark tests
    void testAddBookmark() {
        QSignalSpy bookmarkAddedSpy(m_pageController,
                                    &PageController::bookmarkAdded);
        QSignalSpy bookmarksChangedSpy(m_pageController,
                                       &PageController::bookmarksChanged);

        m_pageController->goToPage(5);
        m_pageController->addBookmark("Test Bookmark", "Test Description");

        QCOMPARE(m_pageController->getBookmarkCount(), 1);
        QVERIFY(m_pageController->hasBookmarkAtPage(5));
        QCOMPARE(bookmarkAddedSpy.count(), 1);
        QCOMPARE(bookmarksChangedSpy.count(), 1);

        // Check signal parameters
        QList<QVariant> args = bookmarkAddedSpy.first();
        QCOMPARE(args.at(0).toInt(), 5);
        QCOMPARE(args.at(1).toString(), QString("Test Bookmark"));
    }

    void testAddBookmarkAtPage() {
        QSignalSpy bookmarkAddedSpy(m_pageController,
                                    &PageController::bookmarkAdded);

        m_pageController->addBookmarkAtPage(3, "Page 3 Bookmark");

        QCOMPARE(m_pageController->getBookmarkCount(), 1);
        QVERIFY(m_pageController->hasBookmarkAtPage(3));
        QCOMPARE(bookmarkAddedSpy.count(), 1);
    }

    void testRemoveBookmark() {
        // Add a bookmark first
        m_pageController->addBookmarkAtPage(5, "Test Bookmark");
        QCOMPARE(m_pageController->getBookmarkCount(), 1);

        QSignalSpy bookmarkRemovedSpy(m_pageController,
                                      &PageController::bookmarkRemoved);
        QSignalSpy bookmarksChangedSpy(m_pageController,
                                       &PageController::bookmarksChanged);

        m_pageController->removeBookmark(0);

        QCOMPARE(m_pageController->getBookmarkCount(), 0);
        QVERIFY(!m_pageController->hasBookmarkAtPage(5));
        QCOMPARE(bookmarkRemovedSpy.count(), 1);
        QCOMPARE(bookmarksChangedSpy.count(), 1);
    }

    void testRemoveBookmarkAtPage() {
        // Add a bookmark first
        m_pageController->addBookmarkAtPage(7, "Test Bookmark");

        QSignalSpy bookmarkRemovedSpy(m_pageController,
                                      &PageController::bookmarkRemoved);

        m_pageController->removeBookmarkAtPage(7);

        QCOMPARE(m_pageController->getBookmarkCount(), 0);
        QVERIFY(!m_pageController->hasBookmarkAtPage(7));
        QCOMPARE(bookmarkRemovedSpy.count(), 1);
    }

    // Zoom and rotation tests
    void testSetZoomLevel() {
        QSignalSpy zoomChangedSpy(m_pageController,
                                  &PageController::zoomChanged);

        m_pageController->setZoomLevel(1.5);

        QCOMPARE(m_pageController->getCurrentZoomLevel(), 1.5);
        QCOMPARE(zoomChangedSpy.count(), 1);

        QList<QVariant> args = zoomChangedSpy.first();
        QCOMPARE(args.at(0).toDouble(), 1.5);
    }

    void testSetInvalidZoomLevel() {
        QSignalSpy errorSpy(m_pageController, &PageController::errorOccurred);

        double initialZoom = m_pageController->getCurrentZoomLevel();

        m_pageController->setZoomLevel(0.0);
        m_pageController->setZoomLevel(-1.0);

        // Zoom should not change
        QCOMPARE(m_pageController->getCurrentZoomLevel(), initialZoom);

        // Should emit error signals
        QVERIFY(errorSpy.count() >= 2);
    }

    void testSetRotation() {
        QSignalSpy rotationChangedSpy(m_pageController,
                                      &PageController::rotationChanged);

        m_pageController->setRotation(90);

        QCOMPARE(m_pageController->getCurrentRotation(), 90);
        QCOMPARE(rotationChangedSpy.count(), 1);

        // Test rotation normalization
        m_pageController->setRotation(450);  // Should normalize to 90
        QCOMPARE(m_pageController->getCurrentRotation(), 90);

        m_pageController->setRotation(-90);  // Should normalize to 270
        QCOMPARE(m_pageController->getCurrentRotation(), 270);
    }

    void testResetView() {
        // Change zoom and rotation
        m_pageController->setZoomLevel(2.0);
        m_pageController->setRotation(180);

        QSignalSpy zoomChangedSpy(m_pageController,
                                  &PageController::zoomChanged);
        QSignalSpy rotationChangedSpy(m_pageController,
                                      &PageController::rotationChanged);

        m_pageController->resetView();

        QCOMPARE(m_pageController->getCurrentZoomLevel(), 1.0);
        QCOMPARE(m_pageController->getCurrentRotation(), 0);
        QCOMPARE(zoomChangedSpy.count(), 1);
        QCOMPARE(rotationChangedSpy.count(), 1);
    }

    // Model management tests
    void testSetModel() {
        MockPageModel* newModel = new MockPageModel(this);
        newModel->setTotalPages(20);

        QSignalSpy bookmarksChangedSpy(m_pageController,
                                       &PageController::bookmarksChanged);

        // Add some bookmarks first
        m_pageController->addBookmarkAtPage(5, "Test");
        QCOMPARE(m_pageController->getBookmarkCount(), 1);

        // Set new model
        m_pageController->setModel(newModel);

        QVERIFY(m_pageController->getModel() == newModel);
        QCOMPARE(m_pageController->getTotalPages(), 20);

        // Bookmarks should be cleared when model changes
        QCOMPARE(m_pageController->getBookmarkCount(), 0);
        QCOMPARE(bookmarksChangedSpy.count(), 2);  // One for add, one for clear
    }

private:
    PageController* m_pageController = nullptr;
    MockPageModel* m_mockPageModel = nullptr;
};

QTEST_MAIN(PageControllerTest)
#include "page_controller_test.moc"
