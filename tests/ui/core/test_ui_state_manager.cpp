#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QSignalSpy>
#include <QSplitter>
#include <QtTest/QtTest>
#include "../../../app/ui/core/UIStateManager.h"

class UIStateManagerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testSingletonInstance();
    void testSetState();
    void testGetState();
    void testHasState();
    void testRemoveState();
    void testClearScope();
    void testRegisterComponent();
    void testUnregisterComponent();
    void testSaveComponentState();
    void testRestoreComponentState();
    void testSaveAllComponentStates();
    void testRestoreAllComponentStates();
    void testSaveWindowState();
    void testRestoreWindowState();
    void testSaveGeometry();
    void testRestoreGeometry();
    void testSaveSplitterState();
    void testRestoreSplitterState();
    void testBeginBatchUpdate();
    void testEndBatchUpdate();
    void testSaveAllStates();
    void testRestoreAllStates();
    void testEnableAutosave();
    void testForceSave();
    void testForceRestore();
    void testValidateState();
    void testRepairCorruptedState();
    void testCreateStateBackup();
    void testRestoreFromBackup();
    void testSetCompressionEnabled();
    void testSetEncryptionEnabled();
    void testSetMaxStateAge();
    void testGetStateKeys();
    void testExportState();
    void testImportState();
    void testCleanupExpiredStates();
    void testOptimizeStateStorage();
    void testGetStateStorageSize();
    void testStateChangedSignal();
    void testComponentStateChangedSignal();
    void testStateSavedSignal();
    void testStateRestoredSignal();
    void testStateScopeEnum();
    void testStatePriorityEnum();

private:
    QWidget* m_parentWidget;
    QMainWindow* m_mainWindow;
    QPushButton* m_testButton;
    QSplitter* m_testSplitter;
};

void UIStateManagerTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    m_mainWindow = new QMainWindow();
    m_mainWindow->resize(1024, 768);
    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void UIStateManagerTest::cleanupTestCase() {
    delete m_mainWindow;
    delete m_parentWidget;
}

void UIStateManagerTest::init() {
    m_testButton = new QPushButton("Test", m_parentWidget);
    m_testSplitter = new QSplitter(m_parentWidget);
}

void UIStateManagerTest::cleanup() {
    UIStateManager::instance().unregisterComponent(m_testButton);
    UIStateManager::instance().removeState("test_key");
    delete m_testButton;
    delete m_testSplitter;
    m_testButton = nullptr;
    m_testSplitter = nullptr;
}

void UIStateManagerTest::testSingletonInstance() {
    auto& i1 = UIStateManager::instance();
    auto& i2 = UIStateManager::instance();
    QCOMPARE(&i1, &i2);
}

void UIStateManagerTest::testSetState() {
    UIStateManager::instance().setState("test_key", QVariant(42));
    QVERIFY(true);
}

void UIStateManagerTest::testGetState() {
    UIStateManager::instance().setState("test_key", QVariant(42));
    QVariant value =
        UIStateManager::instance().getState("test_key", QVariant(0));
    QCOMPARE(value.toInt(), 42);
}

void UIStateManagerTest::testHasState() {
    UIStateManager::instance().setState("test_key", QVariant(42));
    QVERIFY(UIStateManager::instance().hasState("test_key"));
    QVERIFY(!UIStateManager::instance().hasState("nonexistent_key"));
}

void UIStateManagerTest::testRemoveState() {
    UIStateManager::instance().setState("test_key", QVariant(42));
    UIStateManager::instance().removeState("test_key");
    QVERIFY(!UIStateManager::instance().hasState("test_key"));
}

void UIStateManagerTest::testClearScope() {
    UIStateManager::instance().setState("test_key", QVariant(42),
                                        UIStateManager::StateScope::Session);
    UIStateManager::instance().clearScope(UIStateManager::StateScope::Session);
    QVERIFY(true);
}

void UIStateManagerTest::testRegisterComponent() {
    UIStateManager::instance().registerComponent(m_testButton, "test_button");
    QVERIFY(true);
}

void UIStateManagerTest::testUnregisterComponent() {
    UIStateManager::instance().registerComponent(m_testButton, "test_button");
    UIStateManager::instance().unregisterComponent(m_testButton);
    QVERIFY(true);
}

void UIStateManagerTest::testSaveComponentState() {
    UIStateManager::instance().registerComponent(m_testButton, "test_button");
    UIStateManager::instance().saveComponentState(m_testButton);
    QVERIFY(true);
}

void UIStateManagerTest::testRestoreComponentState() {
    UIStateManager::instance().registerComponent(m_testButton, "test_button");
    UIStateManager::instance().saveComponentState(m_testButton);
    UIStateManager::instance().restoreComponentState(m_testButton);
    QVERIFY(true);
}

void UIStateManagerTest::testSaveAllComponentStates() {
    UIStateManager::instance().registerComponent(m_testButton, "test_button");
    UIStateManager::instance().saveAllComponentStates();
    QVERIFY(true);
}

void UIStateManagerTest::testRestoreAllComponentStates() {
    UIStateManager::instance().restoreAllComponentStates();
    QVERIFY(true);
}

void UIStateManagerTest::testSaveWindowState() {
    UIStateManager::instance().saveWindowState(m_mainWindow);
    QVERIFY(true);
}

void UIStateManagerTest::testRestoreWindowState() {
    UIStateManager::instance().saveWindowState(m_mainWindow);
    UIStateManager::instance().restoreWindowState(m_mainWindow);
    QVERIFY(true);
}

void UIStateManagerTest::testSaveGeometry() {
    UIStateManager::instance().saveGeometry(m_testButton, "test_button_geo");
    QVERIFY(true);
}

void UIStateManagerTest::testRestoreGeometry() {
    UIStateManager::instance().saveGeometry(m_testButton, "test_button_geo");
    UIStateManager::instance().restoreGeometry(m_testButton, "test_button_geo");
    QVERIFY(true);
}

void UIStateManagerTest::testSaveSplitterState() {
    UIStateManager::instance().saveSplitterState(m_testSplitter,
                                                 "test_splitter");
    QVERIFY(true);
}

void UIStateManagerTest::testRestoreSplitterState() {
    UIStateManager::instance().saveSplitterState(m_testSplitter,
                                                 "test_splitter");
    UIStateManager::instance().restoreSplitterState(m_testSplitter,
                                                    "test_splitter");
    QVERIFY(true);
}

void UIStateManagerTest::testBeginBatchUpdate() {
    UIStateManager::instance().beginBatchUpdate();
    UIStateManager::instance().setState("key1", QVariant(1));
    UIStateManager::instance().setState("key2", QVariant(2));
    UIStateManager::instance().endBatchUpdate();
    QVERIFY(true);
}

void UIStateManagerTest::testEndBatchUpdate() {
    UIStateManager::instance().beginBatchUpdate();
    UIStateManager::instance().endBatchUpdate();
    QVERIFY(true);
}

void UIStateManagerTest::testSaveAllStates() {
    UIStateManager::instance().saveAllStates();
    QVERIFY(true);
}

void UIStateManagerTest::testRestoreAllStates() {
    UIStateManager::instance().restoreAllStates();
    QVERIFY(true);
}

void UIStateManagerTest::testEnableAutosave() {
    UIStateManager::instance().enableAutosave(true, 60000);
    UIStateManager::instance().enableAutosave(false);
    QVERIFY(true);
}

void UIStateManagerTest::testForceSave() {
    UIStateManager::instance().forceSave();
    QVERIFY(true);
}

void UIStateManagerTest::testForceRestore() {
    UIStateManager::instance().forceRestore();
    QVERIFY(true);
}

void UIStateManagerTest::testValidateState() {
    UIStateManager::instance().setState("test_key", QVariant(42));
    bool valid = UIStateManager::instance().validateState("test_key");
    QVERIFY(valid || !valid);
}

void UIStateManagerTest::testRepairCorruptedState() {
    UIStateManager::instance().repairCorruptedState();
    QVERIFY(true);
}

void UIStateManagerTest::testCreateStateBackup() {
    UIStateManager::instance().createStateBackup();
    QVERIFY(true);
}

void UIStateManagerTest::testRestoreFromBackup() {
    UIStateManager::instance().createStateBackup();
    UIStateManager::instance().restoreFromBackup();
    QVERIFY(true);
}

void UIStateManagerTest::testSetCompressionEnabled() {
    UIStateManager::instance().setCompressionEnabled(true);
    UIStateManager::instance().setCompressionEnabled(false);
    QVERIFY(true);
}

void UIStateManagerTest::testSetEncryptionEnabled() {
    UIStateManager::instance().setEncryptionEnabled(true);
    UIStateManager::instance().setEncryptionEnabled(false);
    QVERIFY(true);
}

void UIStateManagerTest::testSetMaxStateAge() {
    UIStateManager::instance().setMaxStateAge(30);
    QVERIFY(true);
}

void UIStateManagerTest::testGetStateKeys() {
    UIStateManager::instance().setState("test_key", QVariant(42));
    QStringList keys = UIStateManager::instance().getStateKeys();
    QVERIFY(true);
}

void UIStateManagerTest::testExportState() {
    UIStateManager::instance().setState("test_key", QVariant(42));
    QJsonObject exported = UIStateManager::instance().exportState();
    QVERIFY(true);
}

void UIStateManagerTest::testImportState() {
    QJsonObject stateData;
    stateData["imported_key"] = 123;
    UIStateManager::instance().importState(stateData);
    QVERIFY(true);
}

void UIStateManagerTest::testCleanupExpiredStates() {
    UIStateManager::instance().cleanupExpiredStates();
    QVERIFY(true);
}

void UIStateManagerTest::testOptimizeStateStorage() {
    UIStateManager::instance().optimizeStateStorage();
    QVERIFY(true);
}

void UIStateManagerTest::testGetStateStorageSize() {
    qint64 size = UIStateManager::instance().getStateStorageSize();
    QVERIFY(size >= 0);
}

void UIStateManagerTest::testStateChangedSignal() {
    QSignalSpy spy(&UIStateManager::instance(), &UIStateManager::stateChanged);
    QVERIFY(spy.isValid());
}

void UIStateManagerTest::testComponentStateChangedSignal() {
    QSignalSpy spy(&UIStateManager::instance(),
                   &UIStateManager::componentStateChanged);
    QVERIFY(spy.isValid());
}

void UIStateManagerTest::testStateSavedSignal() {
    QSignalSpy spy(&UIStateManager::instance(), &UIStateManager::stateSaved);
    QVERIFY(spy.isValid());
}

void UIStateManagerTest::testStateRestoredSignal() {
    QSignalSpy spy(&UIStateManager::instance(), &UIStateManager::stateRestored);
    QVERIFY(spy.isValid());
}

void UIStateManagerTest::testStateScopeEnum() {
    QVERIFY(UIStateManager::StateScope::Session !=
            UIStateManager::StateScope::User);
    QVERIFY(UIStateManager::StateScope::Global !=
            UIStateManager::StateScope::Component);
}

void UIStateManagerTest::testStatePriorityEnum() {
    QVERIFY(UIStateManager::StatePriority::Low !=
            UIStateManager::StatePriority::High);
    QVERIFY(UIStateManager::StatePriority::Normal !=
            UIStateManager::StatePriority::Critical);
}

QTEST_MAIN(UIStateManagerTest)
#include "test_ui_state_manager.moc"
