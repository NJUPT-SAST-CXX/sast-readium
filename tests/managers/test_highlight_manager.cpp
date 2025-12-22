#include <QColor>
#include <QSignalSpy>
#include <QTest>
#include "../../app/managers/HighlightManager.h"
#include "../TestUtilities.h"

class TestHighlightManager : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() { m_manager = new HighlightManager(); }

    void cleanup() {
        delete m_manager;
        m_manager = nullptr;
    }

    void testConstruction() { QVERIFY(m_manager != nullptr); }

    void testDefaultColors() {
        QList<QColor> colors = m_manager->availableColors();
        QVERIFY(!colors.isEmpty());
    }

    void testCurrentColor() {
        QColor color = m_manager->currentColor();
        QVERIFY(color.isValid());

        m_manager->setCurrentColor(Qt::red);
        QCOMPARE(m_manager->currentColor(), QColor(Qt::red));

        m_manager->setCurrentColor(Qt::blue);
        QCOMPARE(m_manager->currentColor(), QColor(Qt::blue));
    }

    void testAddHighlight() {
        QSignalSpy spy(m_manager, &HighlightManager::highlightAdded);

        bool result =
            m_manager->addHighlight(1, QRectF(10, 10, 100, 20), Qt::yellow);
        QVERIFY(result);
        QCOMPARE(spy.count(), 1);
    }

    void testRemoveHighlight() {
        m_manager->addHighlight(1, QRectF(10, 10, 100, 20), Qt::yellow);

        QSignalSpy spy(m_manager, &HighlightManager::highlightRemoved);

        QList<HighlightManager::Highlight> highlights =
            m_manager->getHighlightsForPage(1);
        if (!highlights.isEmpty()) {
            bool result = m_manager->removeHighlight(highlights.first().id);
            QVERIFY(result);
            QCOMPARE(spy.count(), 1);
        }
    }

    void testGetHighlightsForPage() {
        m_manager->addHighlight(1, QRectF(10, 10, 100, 20), Qt::yellow);
        m_manager->addHighlight(1, QRectF(10, 50, 100, 20), Qt::green);
        m_manager->addHighlight(2, QRectF(10, 10, 100, 20), Qt::blue);

        QList<HighlightManager::Highlight> page1Highlights =
            m_manager->getHighlightsForPage(1);
        QList<HighlightManager::Highlight> page2Highlights =
            m_manager->getHighlightsForPage(2);

        QCOMPARE(page1Highlights.size(), 2);
        QCOMPARE(page2Highlights.size(), 1);
    }

    void testClearHighlights() {
        m_manager->addHighlight(1, QRectF(10, 10, 100, 20), Qt::yellow);
        m_manager->addHighlight(2, QRectF(10, 10, 100, 20), Qt::yellow);
        m_manager->addHighlight(3, QRectF(10, 10, 100, 20), Qt::yellow);

        QSignalSpy spy(m_manager, &HighlightManager::highlightsCleared);

        m_manager->clearAllHighlights();

        QCOMPARE(spy.count(), 1);
        QCOMPARE(m_manager->getHighlightsForPage(1).size(), 0);
        QCOMPARE(m_manager->getHighlightsForPage(2).size(), 0);
        QCOMPARE(m_manager->getHighlightsForPage(3).size(), 0);
    }

    void testClearHighlightsForPage() {
        m_manager->addHighlight(1, QRectF(10, 10, 100, 20), Qt::yellow);
        m_manager->addHighlight(1, QRectF(10, 50, 100, 20), Qt::green);
        m_manager->addHighlight(2, QRectF(10, 10, 100, 20), Qt::blue);

        m_manager->clearHighlightsForPage(1);

        QCOMPARE(m_manager->getHighlightsForPage(1).size(), 0);
        QCOMPARE(m_manager->getHighlightsForPage(2).size(), 1);
    }

    void testUpdateHighlightColor() {
        m_manager->addHighlight(1, QRectF(10, 10, 100, 20), Qt::yellow);

        QList<HighlightManager::Highlight> highlights =
            m_manager->getHighlightsForPage(1);
        if (!highlights.isEmpty()) {
            QString id = highlights.first().id;
            bool result = m_manager->updateHighlightColor(id, Qt::red);
            QVERIFY(result);

            highlights = m_manager->getHighlightsForPage(1);
            QCOMPARE(highlights.first().color, QColor(Qt::red));
        }
    }

    void testHighlightAtPoint() {
        m_manager->addHighlight(1, QRectF(10, 10, 100, 20), Qt::yellow);

        HighlightManager::Highlight highlight =
            m_manager->getHighlightAtPoint(1, QPointF(50, 20));
        QVERIFY(!highlight.id.isEmpty());

        HighlightManager::Highlight noHighlight =
            m_manager->getHighlightAtPoint(1, QPointF(500, 500));
        QVERIFY(noHighlight.id.isEmpty());
    }

    void testHighlightCount() {
        QCOMPARE(m_manager->getTotalHighlightCount(), 0);

        m_manager->addHighlight(1, QRectF(10, 10, 100, 20), Qt::yellow);
        QCOMPARE(m_manager->getTotalHighlightCount(), 1);

        m_manager->addHighlight(2, QRectF(10, 10, 100, 20), Qt::blue);
        QCOMPARE(m_manager->getTotalHighlightCount(), 2);

        m_manager->addHighlight(1, QRectF(10, 50, 100, 20), Qt::green);
        QCOMPARE(m_manager->getTotalHighlightCount(), 3);
    }

    void testColorChangedSignal() {
        QSignalSpy spy(m_manager, &HighlightManager::currentColorChanged);

        m_manager->setCurrentColor(Qt::magenta);

        QCOMPARE(spy.count(), 1);
        QList<QVariant> args = spy.first();
        QCOMPARE(args.at(0).value<QColor>(), QColor(Qt::magenta));
    }

    void testHighlightStruct() {
        HighlightManager::Highlight highlight;
        QVERIFY(highlight.id.isEmpty());
        QCOMPARE(highlight.pageNumber, -1);
        QVERIFY(!highlight.rect.isValid());
    }

    void testSaveAndLoad() {
        m_manager->addHighlight(1, QRectF(10, 10, 100, 20), Qt::yellow);
        m_manager->addHighlight(2, QRectF(20, 20, 150, 30), Qt::blue);

        QString tempPath = QDir::tempPath() + "/test_highlights.json";

        bool saved = m_manager->saveHighlights(tempPath);
        QVERIFY(saved);

        m_manager->clearAllHighlights();
        QCOMPARE(m_manager->getTotalHighlightCount(), 0);

        bool loaded = m_manager->loadHighlights(tempPath);
        QVERIFY(loaded);
        QCOMPARE(m_manager->getTotalHighlightCount(), 2);

        QFile::remove(tempPath);
    }

private:
    HighlightManager* m_manager = nullptr;
};

QTEST_MAIN(TestHighlightManager)
#include "test_highlight_manager.moc"
