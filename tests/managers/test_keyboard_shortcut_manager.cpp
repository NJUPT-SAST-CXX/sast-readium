#include <QKeySequence>
#include <QSignalSpy>
#include <QTest>
#include "../../app/managers/KeyboardShortcutManager.h"
#include "../TestUtilities.h"

class TestKeyboardShortcutManager : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void testSingleton() {
        KeyboardShortcutManager& instance1 =
            KeyboardShortcutManager::instance();
        KeyboardShortcutManager& instance2 =
            KeyboardShortcutManager::instance();
        QVERIFY(&instance1 == &instance2);
    }

    void testShortcutContext() {
        QCOMPARE(
            static_cast<int>(KeyboardShortcutManager::ShortcutContext::Global),
            0);
        QCOMPARE(static_cast<int>(
                     KeyboardShortcutManager::ShortcutContext::DocumentView),
                 1);
        QCOMPARE(
            static_cast<int>(KeyboardShortcutManager::ShortcutContext::MenuBar),
            2);
    }

    void testShortcutPriority() {
        QCOMPARE(
            static_cast<int>(KeyboardShortcutManager::ShortcutPriority::Low),
            0);
        QCOMPARE(
            static_cast<int>(KeyboardShortcutManager::ShortcutPriority::Normal),
            1);
        QCOMPARE(
            static_cast<int>(KeyboardShortcutManager::ShortcutPriority::High),
            2);
        QCOMPARE(static_cast<int>(
                     KeyboardShortcutManager::ShortcutPriority::Critical),
                 3);
    }

    void testShortcutInfoConstruction() {
        KeyboardShortcutManager::ShortcutInfo info;
        QVERIFY(info.keySequence.isEmpty());
        QVERIFY(info.description.isEmpty());
        QVERIFY(info.enabled);
        QVERIFY(info.contextWidget == nullptr);
    }

    void testShortcutInfoWithParameters() {
        QKeySequence seq(Qt::CTRL | Qt::Key_S);
        KeyboardShortcutManager::ShortcutInfo info(
            seq, ActionMap::FileSave,
            KeyboardShortcutManager::ShortcutContext::Global,
            KeyboardShortcutManager::ShortcutPriority::High, "Save file");

        QCOMPARE(info.keySequence, seq);
        QCOMPARE(info.action, ActionMap::FileSave);
        QCOMPARE(info.context,
                 KeyboardShortcutManager::ShortcutContext::Global);
        QCOMPARE(info.priority,
                 KeyboardShortcutManager::ShortcutPriority::High);
        QCOMPARE(info.description, QString("Save file"));
        QVERIFY(info.enabled);
    }

    void testKeySequenceCreation() {
        QKeySequence seq1(Qt::CTRL | Qt::Key_O);
        QVERIFY(!seq1.isEmpty());

        QKeySequence seq2(Qt::CTRL | Qt::SHIFT | Qt::Key_S);
        QVERIFY(!seq2.isEmpty());

        QKeySequence seq3(Qt::ALT | Qt::Key_F4);
        QVERIFY(!seq3.isEmpty());
    }

    void testKeySequenceComparison() {
        QKeySequence seq1(Qt::CTRL | Qt::Key_C);
        QKeySequence seq2(Qt::CTRL | Qt::Key_C);
        QKeySequence seq3(Qt::CTRL | Qt::Key_V);

        QCOMPARE(seq1, seq2);
        QVERIFY(seq1 != seq3);
    }
};

QTEST_MAIN(TestKeyboardShortcutManager)
#include "test_keyboard_shortcut_manager.moc"
