#include <QSignalSpy>
#include <QTest>
#include "../../app/adapters/SearchAdapter.h"
#include "../TestUtilities.h"

class TestSearchAdapter : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() { m_adapter = new SearchAdapter(); }

    void cleanup() {
        delete m_adapter;
        m_adapter = nullptr;
    }

    void testConstruction() { QVERIFY(m_adapter != nullptr); }

    void testSetSearchEngine() { m_adapter->setSearchEngine(nullptr); }

    void testSetPDFViewerPage() { m_adapter->setPDFViewerPage(nullptr); }

    void testSearchWithoutEngine() {
        m_adapter->search("test query", false, false, false);
    }

    void testSearchCaseSensitive() {
        m_adapter->search("Test Query", true, false, false);
    }

    void testSearchWholeWords() {
        m_adapter->search("word", false, true, false);
    }

    void testSearchRegex() {
        m_adapter->search("test.*pattern", false, false, true);
    }

    void testSearchAllOptions() {
        m_adapter->search("Test", true, true, false);
    }

    void testStopSearchWithoutEngine() { m_adapter->stopSearch(); }

    void testClearResultsWithoutEngine() { m_adapter->clearResults(); }

    void testGoToNextResultWithoutEngine() { m_adapter->goToNextResult(); }

    void testGoToPreviousResultWithoutEngine() {
        m_adapter->goToPreviousResult();
    }

    void testGoToResultWithoutEngine() {
        m_adapter->goToResult(0);
        m_adapter->goToResult(5);
        m_adapter->goToResult(-1);
    }

    void testSearchStartedSignal() {
        QSignalSpy spy(m_adapter, &SearchAdapter::searchStarted);
        QVERIFY(spy.isValid());
    }

    void testSearchFinishedSignal() {
        QSignalSpy spy(m_adapter, &SearchAdapter::searchFinished);
        QVERIFY(spy.isValid());
    }

    void testSearchProgressSignal() {
        QSignalSpy spy(m_adapter, &SearchAdapter::searchProgress);
        QVERIFY(spy.isValid());
    }

    void testResultFoundSignal() {
        QSignalSpy spy(m_adapter, &SearchAdapter::resultFound);
        QVERIFY(spy.isValid());
    }

    void testCurrentResultChangedSignal() {
        QSignalSpy spy(m_adapter, &SearchAdapter::currentResultChanged);
        QVERIFY(spy.isValid());
    }

    void testErrorOccurredSignal() {
        QSignalSpy spy(m_adapter, &SearchAdapter::errorOccurred);
        QVERIFY(spy.isValid());
    }

    void testSearchWithEmptyQuery() {
        m_adapter->search(QString(), false, false, false);
    }

    void testSearchWithWhitespaceQuery() {
        m_adapter->search("   ", false, false, false);
    }

    void testSearchWithSpecialCharacters() {
        m_adapter->search("test@#$%^&*()", false, false, false);
    }

    void testSearchWithUnicode() {
        m_adapter->search("测试文本", false, false, false);
        m_adapter->search("日本語テスト", false, false, false);
        m_adapter->search("тест", false, false, false);
    }

    void testSearchWithLongQuery() {
        QString longQuery = QString(10000, 'a');
        m_adapter->search(longQuery, false, false, false);
    }

    void testNavigationSequence() {
        m_adapter->search("test", false, false, false);
        m_adapter->goToNextResult();
        m_adapter->goToNextResult();
        m_adapter->goToPreviousResult();
        m_adapter->goToResult(0);
        m_adapter->clearResults();
    }

    void testMultipleSearches() {
        m_adapter->search("first", false, false, false);
        m_adapter->search("second", false, false, false);
        m_adapter->stopSearch();
        m_adapter->search("third", true, true, false);
        m_adapter->clearResults();
    }

    void testSearchStopClearSequence() {
        m_adapter->search("test", false, false, false);
        m_adapter->stopSearch();
        m_adapter->clearResults();
        m_adapter->search("another", false, false, false);
        m_adapter->clearResults();
    }

private:
    SearchAdapter* m_adapter = nullptr;
};

QTEST_MAIN(TestSearchAdapter)
#include "test_search_adapter.moc"
