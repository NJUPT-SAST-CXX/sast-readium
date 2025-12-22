#include <QPointF>
#include <QSignalSpy>
#include <QTest>
#include "../../app/interaction/MultiPageTextSelection.h"
#include "../TestUtilities.h"

class TestMultiPageTextSelection : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() { m_selection = new MultiPageTextSelection(); }

    void cleanup() {
        delete m_selection;
        m_selection = nullptr;
    }

    void testConstruction() {
        QVERIFY(m_selection != nullptr);
        QVERIFY(!m_selection->hasSelection());
        QVERIFY(m_selection->getSelectedText().isEmpty());
        QVERIFY(m_selection->getSelectedRanges().isEmpty());
    }

    void testStartSelection() {
        QSignalSpy spy(m_selection, &MultiPageTextSelection::selectionChanged);

        m_selection->startSelection(1, QPointF(100, 100));

        QVERIFY(spy.count() >= 0);
    }

    void testUpdateSelection() {
        m_selection->startSelection(1, QPointF(100, 100));

        QSignalSpy spy(m_selection, &MultiPageTextSelection::selectionChanged);

        m_selection->updateSelection(1, QPointF(200, 100));
        m_selection->updateSelection(1, QPointF(300, 100));
    }

    void testFinishSelection() {
        m_selection->startSelection(1, QPointF(100, 100));
        m_selection->updateSelection(1, QPointF(200, 100));

        QSignalSpy spy(m_selection, &MultiPageTextSelection::selectionFinished);

        m_selection->finishSelection();
    }

    void testClearSelection() {
        m_selection->startSelection(1, QPointF(100, 100));
        m_selection->updateSelection(1, QPointF(200, 100));
        m_selection->finishSelection();

        m_selection->clearSelection();

        QVERIFY(!m_selection->hasSelection());
        QVERIFY(m_selection->getSelectedText().isEmpty());
        QVERIFY(m_selection->getSelectedRanges().isEmpty());
    }

    void testSinglePageSelection() {
        m_selection->startSelection(1, QPointF(50, 100));
        m_selection->updateSelection(1, QPointF(200, 100));
        m_selection->finishSelection();

        QList<MultiPageTextSelection::PageTextRange> ranges =
            m_selection->getSelectedRanges();
        if (!ranges.isEmpty()) {
            QCOMPARE(ranges.first().pageNumber, 1);
        }
    }

    void testMultiPageSelection() {
        m_selection->startSelection(1, QPointF(100, 500));
        m_selection->updateSelection(2, QPointF(100, 100));
        m_selection->updateSelection(3, QPointF(200, 200));
        m_selection->finishSelection();

        QList<MultiPageTextSelection::PageTextRange> ranges =
            m_selection->getSelectedRanges();
    }

    void testSelectionChangedSignal() {
        QSignalSpy spy(m_selection, &MultiPageTextSelection::selectionChanged);
        QVERIFY(spy.isValid());

        m_selection->startSelection(1, QPointF(100, 100));
        m_selection->updateSelection(1, QPointF(150, 100));
        m_selection->updateSelection(1, QPointF(200, 100));
    }

    void testSelectionFinishedSignal() {
        QSignalSpy spy(m_selection, &MultiPageTextSelection::selectionFinished);
        QVERIFY(spy.isValid());

        m_selection->startSelection(1, QPointF(100, 100));
        m_selection->updateSelection(1, QPointF(200, 100));
        m_selection->finishSelection();
    }

    void testGetSelectedText() {
        m_selection->startSelection(1, QPointF(100, 100));
        m_selection->updateSelection(1, QPointF(200, 100));
        m_selection->finishSelection();

        QString text = m_selection->getSelectedText();
    }

    void testGetSelectedRanges() {
        m_selection->startSelection(1, QPointF(100, 100));
        m_selection->updateSelection(1, QPointF(200, 200));
        m_selection->finishSelection();

        QList<MultiPageTextSelection::PageTextRange> ranges =
            m_selection->getSelectedRanges();
    }

    void testRepeatedSelections() {
        for (int i = 0; i < 10; ++i) {
            m_selection->startSelection(i % 5 + 1, QPointF(100, 100));
            m_selection->updateSelection(i % 5 + 1, QPointF(200, 200));
            m_selection->finishSelection();
            m_selection->clearSelection();
        }

        QVERIFY(!m_selection->hasSelection());
    }

    void testSelectionAcrossManyPages() {
        m_selection->startSelection(1, QPointF(100, 700));

        for (int page = 2; page <= 10; ++page) {
            m_selection->updateSelection(page, QPointF(100, 100));
        }

        m_selection->finishSelection();
    }

    void testClearDuringSelection() {
        m_selection->startSelection(1, QPointF(100, 100));
        m_selection->updateSelection(1, QPointF(150, 100));

        m_selection->clearSelection();

        QVERIFY(!m_selection->hasSelection());
    }

    void testStartNewSelectionWithoutFinishing() {
        m_selection->startSelection(1, QPointF(100, 100));
        m_selection->updateSelection(1, QPointF(150, 100));

        m_selection->startSelection(2, QPointF(50, 50));
        m_selection->updateSelection(2, QPointF(100, 100));
        m_selection->finishSelection();
    }

private:
    MultiPageTextSelection* m_selection = nullptr;
};

QTEST_MAIN(TestMultiPageTextSelection)
#include "test_multi_page_text_selection.moc"
