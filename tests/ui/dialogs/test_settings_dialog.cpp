#include <QApplication>
#include <QComboBox>
#include <QRadioButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QtTest/QtTest>
#include "../../../app/ui/dialogs/SettingsDialog.h"

class SettingsDialogTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testConstruction();
    void testDestruction();
    void testSettingsAppliedSignal();
    void testThemeChangedSignal();
    void testLanguageChangedSignal();
    void testDialogVisibility();
    void testDialogSize();

private:
    QWidget* m_parentWidget;
    SettingsDialog* m_dialog;
};

void SettingsDialogTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void SettingsDialogTest::cleanupTestCase() { delete m_parentWidget; }

void SettingsDialogTest::init() {
    m_dialog = new SettingsDialog(m_parentWidget);
}

void SettingsDialogTest::cleanup() {
    delete m_dialog;
    m_dialog = nullptr;
}

void SettingsDialogTest::testConstruction() { QVERIFY(m_dialog != nullptr); }

void SettingsDialogTest::testDestruction() {
    auto* dialog = new SettingsDialog(m_parentWidget);
    delete dialog;
    QVERIFY(true);
}

void SettingsDialogTest::testSettingsAppliedSignal() {
    QSignalSpy spy(m_dialog, &SettingsDialog::settingsApplied);
    QVERIFY(spy.isValid());
}

void SettingsDialogTest::testThemeChangedSignal() {
    QSignalSpy spy(m_dialog, &SettingsDialog::themeChanged);
    QVERIFY(spy.isValid());
}

void SettingsDialogTest::testLanguageChangedSignal() {
    QSignalSpy spy(m_dialog, &SettingsDialog::languageChanged);
    QVERIFY(spy.isValid());
}

void SettingsDialogTest::testDialogVisibility() {
    m_dialog->show();
    if (QGuiApplication::platformName() != "offscreen") {
        QVERIFY(QTest::qWaitForWindowExposed(m_dialog));
    }
    QVERIFY(m_dialog->isVisible());
    m_dialog->hide();
    QVERIFY(!m_dialog->isVisible());
}

void SettingsDialogTest::testDialogSize() {
    m_dialog->show();
    QVERIFY(m_dialog->width() > 0);
    QVERIFY(m_dialog->height() > 0);
}

QTEST_MAIN(SettingsDialogTest)
#include "test_settings_dialog.moc"
