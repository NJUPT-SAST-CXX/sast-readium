#include <QColor>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QSignalSpy>
#include <QTest>
#include "../../app/interaction/TextSelectionManager.h"
#include "../TestUtilities.h"

class TestTextSelectionManager : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() { m_manager = new TextSelectionManager(); }

    void cleanup() {
        m_manager->clearPage();
        delete m_manager;
        m_manager = nullptr;
    }

    void testConstruction() {
        QVERIFY(m_manager != nullptr);
        QVERIFY(!m_manager->hasPage());
        QVERIFY(!m_manager->hasSelection());
        QVERIFY(m_manager->getSelectedText().isEmpty());
    }

    void testSingleton() {
        TextSelectionManager& instance1 = TextSelectionManager::instance();
        TextSelectionManager& instance2 = TextSelectionManager::instance();
        QVERIFY(&instance1 == &instance2);
    }

    void testClearPage() {
        m_manager->clearPage();
        QVERIFY(!m_manager->hasPage());
    }

    void testSelectionColor() {
        QColor defaultColor = m_manager->getSelectionColor();

        QColor newColor(0, 120, 215, 100);
        m_manager->setSelectionColor(newColor);
        QCOMPARE(m_manager->getSelectionColor(), newColor);

        QColor anotherColor(255, 200, 0, 150);
        m_manager->setSelectionColor(anotherColor);
        QCOMPARE(m_manager->getSelectionColor(), anotherColor);
    }

    void testScaleFactor() {
        QCOMPARE(m_manager->getScaleFactor(), 1.0);

        m_manager->setScaleFactor(1.5);
        QCOMPARE(m_manager->getScaleFactor(), 1.5);

        m_manager->setScaleFactor(2.0);
        QCOMPARE(m_manager->getScaleFactor(), 2.0);

        m_manager->setScaleFactor(0.5);
        QCOMPARE(m_manager->getScaleFactor(), 0.5);
    }

    void testClearSelection() {
        m_manager->clearSelection();
        QVERIFY(!m_manager->hasSelection());
        QVERIFY(m_manager->getSelectedText().isEmpty());
        QVERIFY(m_manager->getSelectionRects().isEmpty());
    }

    void testCanCopy() { QVERIFY(!m_manager->canCopy()); }

    void testGetSelection() {
        TextSelection selection = m_manager->getSelection();
        QVERIFY(selection.isEmpty());
    }

    void testGetTextBoxes() {
        QList<TextBox> boxes = m_manager->getTextBoxes();
        QVERIFY(boxes.isEmpty());
    }

    void testGetPageText() {
        QString text = m_manager->getPageText();
        QVERIFY(text.isEmpty());
    }

    void testSelectionChangedSignal() {
        QSignalSpy spy(m_manager, &TextSelectionManager::selectionChanged);
        QVERIFY(spy.isValid());
    }

    void testSelectionClearedSignal() {
        QSignalSpy spy(m_manager, &TextSelectionManager::selectionCleared);
        QVERIFY(spy.isValid());

        m_manager->clearSelection();
    }

    void testTextCopiedSignal() {
        QSignalSpy spy(m_manager, &TextSelectionManager::textCopied);
        QVERIFY(spy.isValid());
    }

    void testSelectionErrorSignal() {
        QSignalSpy spy(m_manager, &TextSelectionManager::selectionError);
        QVERIFY(spy.isValid());
    }

    void testStartSelectionWithoutPage() {
        m_manager->startSelection(QPointF(100, 100));
        QVERIFY(!m_manager->hasSelection());
    }

    void testUpdateSelectionWithoutPage() {
        m_manager->updateSelection(QPointF(200, 200));
        QVERIFY(!m_manager->hasSelection());
    }

    void testEndSelectionWithoutPage() {
        m_manager->endSelection();
        QVERIFY(!m_manager->hasSelection());
    }

    void testSelectWordAtWithoutPage() {
        m_manager->selectWordAt(QPointF(100, 100));
        QVERIFY(!m_manager->hasSelection());
    }

    void testSelectLineAtWithoutPage() {
        m_manager->selectLineAt(QPointF(100, 100));
        QVERIFY(!m_manager->hasSelection());
    }

    void testSelectAllWithoutPage() {
        m_manager->selectAll();
        QVERIFY(!m_manager->hasSelection());
    }

    void testFindCharacterAtPointWithoutPage() {
        int charIndex = m_manager->findCharacterAtPoint(QPointF(100, 100));
        QCOMPARE(charIndex, -1);
    }

    void testFindTextBoxAtPointWithoutPage() {
        TextBox box = m_manager->findTextBoxAtPoint(QPointF(100, 100));
        QCOMPARE(box.charIndex, -1);
        QVERIFY(box.page == nullptr);
    }

    void testRenderSelectionWithoutPage() {
        QPixmap pixmap(200, 200);
        pixmap.fill(Qt::white);
        QPainter painter(&pixmap);

        m_manager->renderSelection(painter, 1.0);
        painter.end();

        QVERIFY(!pixmap.isNull());
    }

    void testCopySelectionToClipboardWithoutSelection() {
        m_manager->copySelectionToClipboard();
    }

    void testExtractTextBoxesWithoutPage() {
        bool result = m_manager->extractTextBoxes();
        QVERIFY(!result);
    }

    void testTextBoxStruct() {
        TextBox box1;
        QCOMPARE(box1.charIndex, -1);
        QVERIFY(box1.page == nullptr);
        QVERIFY(box1.text.isEmpty());

        TextBox box2(QRectF(10, 10, 100, 20), "test", 5, nullptr);
        QCOMPARE(box2.charIndex, 5);
        QCOMPARE(box2.text, QString("test"));
        QCOMPARE(box2.rect, QRectF(10, 10, 100, 20));

        QVERIFY(box2.contains(QPointF(50, 20)));
        QVERIFY(!box2.contains(QPointF(200, 200)));
    }

    void testTextSelectionStruct() {
        TextSelection selection;
        QVERIFY(selection.isEmpty());
        QCOMPARE(selection.startCharIndex, -1);
        QCOMPARE(selection.endCharIndex, -1);
        QCOMPARE(selection.pageNumber, -1);

        selection.startCharIndex = 10;
        selection.endCharIndex = 5;
        selection.normalize();
        QCOMPARE(selection.startCharIndex, 5);
        QCOMPARE(selection.endCharIndex, 10);

        selection.startCharIndex = 0;
        selection.endCharIndex = 20;
        selection.text = "test selection";
        selection.pageNumber = 1;
        QVERIFY(!selection.isEmpty());

        selection.clear();
        QVERIFY(selection.isEmpty());
        QVERIFY(selection.text.isEmpty());
        QVERIFY(selection.rects.isEmpty());
    }

    void testWithRealPdf() {
        auto* doc = TestDataGenerator::createTestPdfWithoutText(3);
        if (!doc) {
            QSKIP("Could not create test PDF");
            return;
        }

        std::unique_ptr<Poppler::Page> page(doc->page(0));
        if (page) {
            m_manager->setPage(page.release(), 0);
            QVERIFY(m_manager->hasPage());

            m_manager->extractTextBoxes();

            m_manager->startSelection(QPointF(50, 50));
            m_manager->updateSelection(QPointF(150, 50));
            m_manager->endSelection();

            m_manager->clearSelection();
            QVERIFY(!m_manager->hasSelection());

            m_manager->clearPage();
        }

        delete doc;
    }

private:
    TextSelectionManager* m_manager = nullptr;
};

QTEST_MAIN(TestTextSelectionManager)
#include "test_text_selection_manager.moc"
