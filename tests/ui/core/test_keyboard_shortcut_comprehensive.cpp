// SPDX-License-Identifier: MIT

#include <QtTest/QtTest>

#include <QApplication>
#include <QKeySequence>
#include <QSignalSpy>
#include <QWidget>

#include "../../app/controller/EventBus.h"
#include "../../app/controller/ServiceLocator.h"
#include "../../app/controller/StateManager.h"
#include "../../app/managers/KeyboardShortcutManager.h"

#include "../../TestUtilities.h"

class TestKeyboardShortcutComprehensive : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override {
        TestBase::initTestCase();
        setupServices();

        m_mainWindow = std::make_unique<QWidget>();
        m_mainWindow->setWindowTitle("ShortcutTestWindow");
        m_mainWindow->resize(800, 600);
        m_mainWindow->show();
        QApplication::processEvents();

        KeyboardShortcutManager::instance().initialize(m_mainWindow.get());
    }

    void cleanupTestCase() override {
        unregisterTrackedShortcuts();
        QApplication::processEvents();
        m_mainWindow.reset();
        teardownServices();
    }

    void init() override {
        ServiceLocator::instance().clearServices();
        EventBus::instance().clearEventQueue();
    }

    void cleanup() override { unregisterTrackedShortcuts(); }

    void testRegisterShortcut();
    void testRegisterShortcutConflict();
    void testShortcutActivation();
    void testContextSpecificShortcut();

private:
    using ShortcutContext = KeyboardShortcutManager::ShortcutContext;

    void setupServices() {
        ServiceLocator::instance().clearServices();
        StateManager::instance().reset();
        EventBus::instance().clearEventQueue();
    }

    void teardownServices() {
        ServiceLocator::instance().clearServices();
        StateManager::instance().reset();
        EventBus::instance().clearEventQueue();
    }

    void registerShortcutAndTrack(
        const KeyboardShortcutManager::ShortcutInfo& info) {
        auto& manager = KeyboardShortcutManager::instance();
        QVERIFY2(manager.registerShortcut(info),
                 "Shortcut registration failed");
        m_registeredShortcuts.append({info.keySequence, info.context});
    }

    void unregisterTrackedShortcuts() {
        auto& manager = KeyboardShortcutManager::instance();
        for (const auto& entry : std::as_const(m_registeredShortcuts)) {
            manager.unregisterShortcut(entry.first, entry.second);
        }
        m_registeredShortcuts.clear();
    }

    std::unique_ptr<QWidget> m_mainWindow;
    QList<QPair<QKeySequence, ShortcutContext>> m_registeredShortcuts;
};

void TestKeyboardShortcutComprehensive::testRegisterShortcut() {
    auto& manager = KeyboardShortcutManager::instance();

    KeyboardShortcutManager::ShortcutInfo info(
        QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_F12), ActionMap::toggleTheme,
        ShortcutContext::Global,
        KeyboardShortcutManager::ShortcutPriority::High, "Toggle theme",
        m_mainWindow.get());

    registerShortcutAndTrack(info);

    const auto shortcuts = manager.getShortcuts(ShortcutContext::Global);
    bool found = false;
    for (const auto& shortcut : shortcuts) {
        if (shortcut.keySequence == info.keySequence &&
            shortcut.action == info.action) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
}

void TestKeyboardShortcutComprehensive::testRegisterShortcutConflict() {
    KeyboardShortcutManager::ShortcutInfo first(
        QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_7), ActionMap::showHelp,
        ShortcutContext::Global,
        KeyboardShortcutManager::ShortcutPriority::Normal, "Show help",
        m_mainWindow.get());

    registerShortcutAndTrack(first);

    KeyboardShortcutManager::ShortcutInfo conflicting(
        QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_7), ActionMap::showSettings,
        ShortcutContext::Global,
        KeyboardShortcutManager::ShortcutPriority::High, "Show settings",
        m_mainWindow.get());

    auto& manager = KeyboardShortcutManager::instance();
    QVERIFY(!manager.registerShortcut(conflicting));
}

void TestKeyboardShortcutComprehensive::testShortcutActivation() {
    auto& manager = KeyboardShortcutManager::instance();
    QSignalSpy activationSpy(&manager,
                             &KeyboardShortcutManager::shortcutActivated);

    KeyboardShortcutManager::ShortcutInfo info(
        QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_N),
        ActionMap::newTab, ShortcutContext::Global,
        KeyboardShortcutManager::ShortcutPriority::High, "New tab",
        m_mainWindow.get());

    registerShortcutAndTrack(info);

    QTest::keyClick(m_mainWindow.get(), Qt::Key_N,
                    Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier);

    QVERIFY(activationSpy.wait(200));
    const auto arguments = activationSpy.takeFirst();
    QCOMPARE(arguments.at(0).value<ActionMap>(), ActionMap::newTab);
    QCOMPARE(static_cast<int>(arguments.at(1).value<ShortcutContext>()),
             static_cast<int>(ShortcutContext::Global));
}

void TestKeyboardShortcutComprehensive::testContextSpecificShortcut() {
    auto& manager = KeyboardShortcutManager::instance();
    QSignalSpy activationSpy(&manager,
                             &KeyboardShortcutManager::shortcutActivated);

    auto documentView = std::make_unique<QWidget>(m_mainWindow.get());
    documentView->setFocusPolicy(Qt::StrongFocus);
    documentView->setObjectName("DocumentViewTestWidget");
    documentView->resize(400, 300);
    documentView->show();
    QApplication::processEvents();

    manager.setContextWidget(ShortcutContext::DocumentView, documentView.get());

    KeyboardShortcutManager::ShortcutInfo info(
        QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Right), ActionMap::nextPage,
        ShortcutContext::DocumentView,
        KeyboardShortcutManager::ShortcutPriority::High, "Next page",
        documentView.get());

    registerShortcutAndTrack(info);

    // Without focus the shortcut should not fire
    QTest::keyClick(m_mainWindow.get(), Qt::Key_Right,
                    Qt::ControlModifier | Qt::AltModifier);
    QVERIFY(activationSpy.count() == 0);

    // Focus the document view and trigger again
    documentView->setFocus(Qt::TabFocusReason);
    QApplication::processEvents();

    QTest::keyClick(documentView.get(), Qt::Key_Right,
                    Qt::ControlModifier | Qt::AltModifier);

    QVERIFY(activationSpy.wait(200));
    const auto arguments = activationSpy.takeLast();
    QCOMPARE(arguments.at(0).value<ActionMap>(), ActionMap::nextPage);
    QCOMPARE(static_cast<int>(arguments.at(1).value<ShortcutContext>()),
             static_cast<int>(ShortcutContext::DocumentView));
}

QTEST_MAIN(TestKeyboardShortcutComprehensive)
#include "test_keyboard_shortcut_comprehensive.moc"
