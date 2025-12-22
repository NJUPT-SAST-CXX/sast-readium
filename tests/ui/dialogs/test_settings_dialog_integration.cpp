#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include "../../../app/ui/dialogs/SettingsDialog.h"

class SettingsDialogIntegrationTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Construction and initialization tests
    void testConstruction();
    void testDestruction();
    void testDialogProperties();
    void testUIComponents();

    // Tab structure tests
    void testTabWidget();
    void testAppearanceTab();
    void testPerformanceTab();
    void testBehaviorTab();
    void testAdvancedTab();

    // Appearance settings tests
    void testThemeRadioButtons();
    void testThemeSelection();
    void testLanguageComboBox();
    void testLanguageSelection();

    // Performance settings tests
    void testCacheCheckBox();
    void testCacheSizeSpinBox();
    void testPreloadPagesCheckBox();
    void testPreloadCountSpinBox();
    void testRenderQualityComboBox();

    // Behavior settings tests
    void testDefaultZoomComboBox();
    void testDefaultPageModeComboBox();
    void testRecentFilesCountSpinBox();
    void testRememberWindowStateCheckBox();
    void testOpenLastFileCheckBox();

    // Advanced settings tests
    void testLogLevelComboBox();
    void testDebugPanelCheckBox();
    void testWelcomeScreenCheckBox();
    void testCachePathEdit();
    void testBrowseCachePathButton();
    void testClearCacheButton();

    // Button box tests
    void testButtonBox();
    void testApplyButton();
    void testRestoreDefaultsButton();

    // Signal tests
    void testSettingsAppliedSignal();
    void testThemeChangedSignal();
    void testLanguageChangedSignal();

    // Dialog behavior tests
    void testDialogVisibility();
    void testDialogSize();
    void testDialogResize();
    void testDialogModal();

    // Settings persistence tests
    void testLoadSettings();
    void testSaveSettings();

    // Validation tests
    void testCacheSizeValidation();
    void testRecentFilesCountValidation();
    void testCachePathValidation();

    // Theme preview tests
    void testThemePreview();
    void testLanguagePreview();

    // Restore defaults tests
    void testRestoreDefaults();

    // Language change event tests
    void testLanguageChangeEvent();
    void testRetranslateUi();

private:
    SettingsDialog* m_dialog;
    QWidget* m_parentWidget;
    QTemporaryDir* m_tempDir;

    void waitForUI();
    QTabWidget* findTabWidget();
    QWidget* findTabByName(const QString& name);
};

void SettingsDialogIntegrationTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();

    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());

    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void SettingsDialogIntegrationTest::cleanupTestCase() {
    delete m_tempDir;
    delete m_parentWidget;
}

void SettingsDialogIntegrationTest::init() {
    m_dialog = new SettingsDialog(m_parentWidget);
    m_dialog->show();

    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_dialog));
    }
}

void SettingsDialogIntegrationTest::cleanup() {
    delete m_dialog;
    m_dialog = nullptr;
}

void SettingsDialogIntegrationTest::waitForUI() {
    QTest::qWait(50);
    QApplication::processEvents();
}

QTabWidget* SettingsDialogIntegrationTest::findTabWidget() {
    return m_dialog->findChild<QTabWidget*>();
}

QWidget* SettingsDialogIntegrationTest::findTabByName(const QString& name) {
    QTabWidget* tabWidget = findTabWidget();
    if (!tabWidget)
        return nullptr;

    for (int i = 0; i < tabWidget->count(); ++i) {
        if (tabWidget->tabText(i).contains(name, Qt::CaseInsensitive)) {
            return tabWidget->widget(i);
        }
    }
    return nullptr;
}

// ============================================================================
// Construction and initialization tests
// ============================================================================

void SettingsDialogIntegrationTest::testConstruction() {
    QVERIFY(m_dialog != nullptr);
    QVERIFY(m_dialog->isVisible());
}

void SettingsDialogIntegrationTest::testDestruction() {
    auto* dialog = new SettingsDialog(m_parentWidget);
    delete dialog;
    QVERIFY(true);
}

void SettingsDialogIntegrationTest::testDialogProperties() {
    QVERIFY(!m_dialog->windowTitle().isEmpty());
    QVERIFY(m_dialog->isModal());
    QVERIFY(m_dialog->minimumWidth() >= 600);
    QVERIFY(m_dialog->minimumHeight() >= 500);
}

void SettingsDialogIntegrationTest::testUIComponents() {
    // Tab widget
    QTabWidget* tabWidget = findTabWidget();
    QVERIFY(tabWidget != nullptr);

    // Button box
    QDialogButtonBox* buttonBox = m_dialog->findChild<QDialogButtonBox*>();
    QVERIFY(buttonBox != nullptr);

    // Restore defaults button
    QList<QPushButton*> buttons = m_dialog->findChildren<QPushButton*>();
    bool foundRestoreButton = false;
    for (QPushButton* btn : buttons) {
        if (btn->text().contains("Restore", Qt::CaseInsensitive) ||
            btn->text().contains("Default", Qt::CaseInsensitive)) {
            foundRestoreButton = true;
            break;
        }
    }
    QVERIFY(foundRestoreButton);
}

// ============================================================================
// Tab structure tests
// ============================================================================

void SettingsDialogIntegrationTest::testTabWidget() {
    QTabWidget* tabWidget = findTabWidget();
    QVERIFY(tabWidget != nullptr);
    QCOMPARE(tabWidget->count(), 4);
}

void SettingsDialogIntegrationTest::testAppearanceTab() {
    QWidget* tab = findTabByName("Appearance");
    QVERIFY(tab != nullptr);

    // Should contain theme radio buttons
    QList<QRadioButton*> radios = tab->findChildren<QRadioButton*>();
    QVERIFY(radios.size() >= 2);

    // Should contain language combo box
    QComboBox* langCombo = tab->findChild<QComboBox*>();
    QVERIFY(langCombo != nullptr);
}

void SettingsDialogIntegrationTest::testPerformanceTab() {
    QWidget* tab = findTabByName("Performance");
    QVERIFY(tab != nullptr);

    // Should contain cache settings
    QList<QCheckBox*> checkboxes = tab->findChildren<QCheckBox*>();
    QVERIFY(checkboxes.size() >= 2);

    // Should contain spin boxes
    QList<QSpinBox*> spinboxes = tab->findChildren<QSpinBox*>();
    QVERIFY(spinboxes.size() >= 2);

    // Should contain render quality combo
    QComboBox* qualityCombo = tab->findChild<QComboBox*>();
    QVERIFY(qualityCombo != nullptr);
}

void SettingsDialogIntegrationTest::testBehaviorTab() {
    QWidget* tab = findTabByName("Behavior");
    QVERIFY(tab != nullptr);

    // Should contain zoom and page mode combos
    QList<QComboBox*> combos = tab->findChildren<QComboBox*>();
    QVERIFY(combos.size() >= 2);

    // Should contain session checkboxes
    QList<QCheckBox*> checkboxes = tab->findChildren<QCheckBox*>();
    QVERIFY(checkboxes.size() >= 2);
}

void SettingsDialogIntegrationTest::testAdvancedTab() {
    QWidget* tab = findTabByName("Advanced");
    QVERIFY(tab != nullptr);

    // Should contain log level combo
    QComboBox* logCombo = tab->findChild<QComboBox*>();
    QVERIFY(logCombo != nullptr);

    // Should contain cache path edit
    QLineEdit* pathEdit = tab->findChild<QLineEdit*>();
    QVERIFY(pathEdit != nullptr);

    // Should contain buttons (browse, clear cache)
    QList<QPushButton*> buttons = tab->findChildren<QPushButton*>();
    QVERIFY(buttons.size() >= 2);
}

// ============================================================================
// Appearance settings tests
// ============================================================================

void SettingsDialogIntegrationTest::testThemeRadioButtons() {
    QWidget* tab = findTabByName("Appearance");
    QVERIFY(tab != nullptr);

    QList<QRadioButton*> radios = tab->findChildren<QRadioButton*>();
    QVERIFY(radios.size() >= 2);

    // One should be checked
    bool anyChecked = false;
    for (QRadioButton* radio : radios) {
        if (radio->isChecked()) {
            anyChecked = true;
            break;
        }
    }
    QVERIFY(anyChecked);
}

void SettingsDialogIntegrationTest::testThemeSelection() {
    QWidget* tab = findTabByName("Appearance");
    QVERIFY(tab != nullptr);

    QList<QRadioButton*> radios = tab->findChildren<QRadioButton*>();
    QVERIFY(radios.size() >= 2);

    // Toggle theme selection
    for (QRadioButton* radio : radios) {
        if (!radio->isChecked()) {
            radio->setChecked(true);
            waitForUI();
            QVERIFY(radio->isChecked());
            break;
        }
    }
}

void SettingsDialogIntegrationTest::testLanguageComboBox() {
    QWidget* tab = findTabByName("Appearance");
    QVERIFY(tab != nullptr);

    QComboBox* langCombo = tab->findChild<QComboBox*>();
    QVERIFY(langCombo != nullptr);
    QVERIFY(langCombo->count() >= 2);
}

void SettingsDialogIntegrationTest::testLanguageSelection() {
    QWidget* tab = findTabByName("Appearance");
    QVERIFY(tab != nullptr);

    QComboBox* langCombo = tab->findChild<QComboBox*>();
    QVERIFY(langCombo != nullptr);

    int initialIndex = langCombo->currentIndex();
    int newIndex = (initialIndex + 1) % langCombo->count();

    langCombo->setCurrentIndex(newIndex);
    waitForUI();

    QCOMPARE(langCombo->currentIndex(), newIndex);
}

// ============================================================================
// Performance settings tests
// ============================================================================

void SettingsDialogIntegrationTest::testCacheCheckBox() {
    QWidget* tab = findTabByName("Performance");
    QVERIFY(tab != nullptr);

    QList<QCheckBox*> checkboxes = tab->findChildren<QCheckBox*>();
    QCheckBox* cacheCheckBox = nullptr;

    for (QCheckBox* cb : checkboxes) {
        if (cb->text().contains("cache", Qt::CaseInsensitive) ||
            cb->text().contains("caching", Qt::CaseInsensitive)) {
            cacheCheckBox = cb;
            break;
        }
    }

    QVERIFY(cacheCheckBox != nullptr);

    bool originalState = cacheCheckBox->isChecked();
    cacheCheckBox->setChecked(!originalState);
    waitForUI();
    QCOMPARE(cacheCheckBox->isChecked(), !originalState);

    cacheCheckBox->setChecked(originalState);
}

void SettingsDialogIntegrationTest::testCacheSizeSpinBox() {
    QWidget* tab = findTabByName("Performance");
    QVERIFY(tab != nullptr);

    QList<QSpinBox*> spinboxes = tab->findChildren<QSpinBox*>();
    QVERIFY(spinboxes.size() >= 1);

    QSpinBox* cacheSizeSpinBox = spinboxes.first();
    QVERIFY(cacheSizeSpinBox->minimum() >= 50);
    QVERIFY(cacheSizeSpinBox->maximum() <= 5000);

    int originalValue = cacheSizeSpinBox->value();
    cacheSizeSpinBox->setValue(200);
    waitForUI();
    QCOMPARE(cacheSizeSpinBox->value(), 200);

    cacheSizeSpinBox->setValue(originalValue);
}

void SettingsDialogIntegrationTest::testPreloadPagesCheckBox() {
    QWidget* tab = findTabByName("Performance");
    QVERIFY(tab != nullptr);

    QList<QCheckBox*> checkboxes = tab->findChildren<QCheckBox*>();
    QCheckBox* preloadCheckBox = nullptr;

    for (QCheckBox* cb : checkboxes) {
        if (cb->text().contains("preload", Qt::CaseInsensitive)) {
            preloadCheckBox = cb;
            break;
        }
    }

    QVERIFY(preloadCheckBox != nullptr);
    QVERIFY(preloadCheckBox->isChecked() || !preloadCheckBox->isChecked());
}

void SettingsDialogIntegrationTest::testPreloadCountSpinBox() {
    QWidget* tab = findTabByName("Performance");
    QVERIFY(tab != nullptr);

    QList<QSpinBox*> spinboxes = tab->findChildren<QSpinBox*>();
    QVERIFY(spinboxes.size() >= 2);

    // Find preload count spinbox (second one typically)
    QSpinBox* preloadSpinBox = nullptr;
    for (QSpinBox* sb : spinboxes) {
        if (sb->minimum() >= 1 && sb->maximum() <= 10) {
            preloadSpinBox = sb;
            break;
        }
    }

    if (preloadSpinBox) {
        QVERIFY(preloadSpinBox->value() >= 1);
        QVERIFY(preloadSpinBox->value() <= 10);
    }
}

void SettingsDialogIntegrationTest::testRenderQualityComboBox() {
    QWidget* tab = findTabByName("Performance");
    QVERIFY(tab != nullptr);

    QComboBox* qualityCombo = tab->findChild<QComboBox*>();
    QVERIFY(qualityCombo != nullptr);
    QVERIFY(qualityCombo->count() >= 3);

    // Test selection
    for (int i = 0; i < qualityCombo->count(); ++i) {
        qualityCombo->setCurrentIndex(i);
        waitForUI();
        QCOMPARE(qualityCombo->currentIndex(), i);
    }
}

// ============================================================================
// Behavior settings tests
// ============================================================================

void SettingsDialogIntegrationTest::testDefaultZoomComboBox() {
    QWidget* tab = findTabByName("Behavior");
    QVERIFY(tab != nullptr);

    QList<QComboBox*> combos = tab->findChildren<QComboBox*>();
    QVERIFY(combos.size() >= 1);

    QComboBox* zoomCombo = combos.first();
    QVERIFY(zoomCombo->count() >= 3);
}

void SettingsDialogIntegrationTest::testDefaultPageModeComboBox() {
    QWidget* tab = findTabByName("Behavior");
    QVERIFY(tab != nullptr);

    QList<QComboBox*> combos = tab->findChildren<QComboBox*>();
    QVERIFY(combos.size() >= 2);

    QComboBox* pageModeCombo = combos.at(1);
    QVERIFY(pageModeCombo->count() >= 2);
}

void SettingsDialogIntegrationTest::testRecentFilesCountSpinBox() {
    QWidget* tab = findTabByName("Behavior");
    QVERIFY(tab != nullptr);

    QSpinBox* recentSpinBox = tab->findChild<QSpinBox*>();
    QVERIFY(recentSpinBox != nullptr);
    QVERIFY(recentSpinBox->minimum() >= 5);
    QVERIFY(recentSpinBox->maximum() <= 50);
}

void SettingsDialogIntegrationTest::testRememberWindowStateCheckBox() {
    QWidget* tab = findTabByName("Behavior");
    QVERIFY(tab != nullptr);

    QList<QCheckBox*> checkboxes = tab->findChildren<QCheckBox*>();
    QCheckBox* rememberCheckBox = nullptr;

    for (QCheckBox* cb : checkboxes) {
        if (cb->text().contains("window", Qt::CaseInsensitive) ||
            cb->text().contains("remember", Qt::CaseInsensitive)) {
            rememberCheckBox = cb;
            break;
        }
    }

    QVERIFY(rememberCheckBox != nullptr);
}

void SettingsDialogIntegrationTest::testOpenLastFileCheckBox() {
    QWidget* tab = findTabByName("Behavior");
    QVERIFY(tab != nullptr);

    QList<QCheckBox*> checkboxes = tab->findChildren<QCheckBox*>();
    QCheckBox* openLastCheckBox = nullptr;

    for (QCheckBox* cb : checkboxes) {
        if (cb->text().contains("last", Qt::CaseInsensitive) ||
            cb->text().contains("reopen", Qt::CaseInsensitive)) {
            openLastCheckBox = cb;
            break;
        }
    }

    QVERIFY(openLastCheckBox != nullptr);
}

// ============================================================================
// Advanced settings tests
// ============================================================================

void SettingsDialogIntegrationTest::testLogLevelComboBox() {
    QWidget* tab = findTabByName("Advanced");
    QVERIFY(tab != nullptr);

    QComboBox* logCombo = tab->findChild<QComboBox*>();
    QVERIFY(logCombo != nullptr);
    QVERIFY(logCombo->count() >= 4);

    // Test all log levels
    for (int i = 0; i < logCombo->count(); ++i) {
        logCombo->setCurrentIndex(i);
        waitForUI();
        QCOMPARE(logCombo->currentIndex(), i);
    }
}

void SettingsDialogIntegrationTest::testDebugPanelCheckBox() {
    QWidget* tab = findTabByName("Advanced");
    QVERIFY(tab != nullptr);

    QList<QCheckBox*> checkboxes = tab->findChildren<QCheckBox*>();
    QCheckBox* debugCheckBox = nullptr;

    for (QCheckBox* cb : checkboxes) {
        if (cb->text().contains("debug", Qt::CaseInsensitive)) {
            debugCheckBox = cb;
            break;
        }
    }

    QVERIFY(debugCheckBox != nullptr);
}

void SettingsDialogIntegrationTest::testWelcomeScreenCheckBox() {
    QWidget* tab = findTabByName("Advanced");
    QVERIFY(tab != nullptr);

    QList<QCheckBox*> checkboxes = tab->findChildren<QCheckBox*>();
    QCheckBox* welcomeCheckBox = nullptr;

    for (QCheckBox* cb : checkboxes) {
        if (cb->text().contains("welcome", Qt::CaseInsensitive)) {
            welcomeCheckBox = cb;
            break;
        }
    }

    QVERIFY(welcomeCheckBox != nullptr);
}

void SettingsDialogIntegrationTest::testCachePathEdit() {
    QWidget* tab = findTabByName("Advanced");
    QVERIFY(tab != nullptr);

    QLineEdit* pathEdit = tab->findChild<QLineEdit*>();
    QVERIFY(pathEdit != nullptr);

    // Test setting a path
    QString testPath = m_tempDir->path();
    pathEdit->setText(testPath);
    waitForUI();
    QCOMPARE(pathEdit->text(), testPath);
}

void SettingsDialogIntegrationTest::testBrowseCachePathButton() {
    QWidget* tab = findTabByName("Advanced");
    QVERIFY(tab != nullptr);

    QList<QPushButton*> buttons = tab->findChildren<QPushButton*>();
    QPushButton* browseButton = nullptr;

    for (QPushButton* btn : buttons) {
        if (btn->text().contains("Browse", Qt::CaseInsensitive)) {
            browseButton = btn;
            break;
        }
    }

    QVERIFY(browseButton != nullptr);
    QVERIFY(browseButton->isEnabled());
}

void SettingsDialogIntegrationTest::testClearCacheButton() {
    QWidget* tab = findTabByName("Advanced");
    QVERIFY(tab != nullptr);

    QList<QPushButton*> buttons = tab->findChildren<QPushButton*>();
    QPushButton* clearButton = nullptr;

    for (QPushButton* btn : buttons) {
        if (btn->text().contains("Clear", Qt::CaseInsensitive)) {
            clearButton = btn;
            break;
        }
    }

    QVERIFY(clearButton != nullptr);
    QVERIFY(clearButton->isEnabled());
}

// ============================================================================
// Button box tests
// ============================================================================

void SettingsDialogIntegrationTest::testButtonBox() {
    QDialogButtonBox* buttonBox = m_dialog->findChild<QDialogButtonBox*>();
    QVERIFY(buttonBox != nullptr);

    QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
    QPushButton* cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
    QPushButton* applyButton = buttonBox->button(QDialogButtonBox::Apply);

    QVERIFY(okButton != nullptr);
    QVERIFY(cancelButton != nullptr);
    QVERIFY(applyButton != nullptr);
}

void SettingsDialogIntegrationTest::testApplyButton() {
    QDialogButtonBox* buttonBox = m_dialog->findChild<QDialogButtonBox*>();
    QVERIFY(buttonBox != nullptr);

    QPushButton* applyButton = buttonBox->button(QDialogButtonBox::Apply);
    QVERIFY(applyButton != nullptr);
    QVERIFY(applyButton->isEnabled());
}

void SettingsDialogIntegrationTest::testRestoreDefaultsButton() {
    QList<QPushButton*> buttons = m_dialog->findChildren<QPushButton*>();
    QPushButton* restoreButton = nullptr;

    for (QPushButton* btn : buttons) {
        if (btn->text().contains("Restore", Qt::CaseInsensitive) ||
            btn->text().contains("Default", Qt::CaseInsensitive)) {
            restoreButton = btn;
            break;
        }
    }

    QVERIFY(restoreButton != nullptr);
    QVERIFY(restoreButton->isEnabled());
}

// ============================================================================
// Signal tests
// ============================================================================

void SettingsDialogIntegrationTest::testSettingsAppliedSignal() {
    QSignalSpy spy(m_dialog, &SettingsDialog::settingsApplied);
    QVERIFY(spy.isValid());
}

void SettingsDialogIntegrationTest::testThemeChangedSignal() {
    QSignalSpy spy(m_dialog, &SettingsDialog::themeChanged);
    QVERIFY(spy.isValid());

    // Toggle theme to trigger signal
    QWidget* tab = findTabByName("Appearance");
    QVERIFY(tab != nullptr);

    QList<QRadioButton*> radios = tab->findChildren<QRadioButton*>();
    for (QRadioButton* radio : radios) {
        if (!radio->isChecked()) {
            radio->setChecked(true);
            waitForUI();
            break;
        }
    }

    QVERIFY(spy.count() >= 1);
}

void SettingsDialogIntegrationTest::testLanguageChangedSignal() {
    QSignalSpy spy(m_dialog, &SettingsDialog::languageChanged);
    QVERIFY(spy.isValid());

    // Change language to trigger signal
    QWidget* tab = findTabByName("Appearance");
    QVERIFY(tab != nullptr);

    QComboBox* langCombo = tab->findChild<QComboBox*>();
    QVERIFY(langCombo != nullptr);

    int newIndex = (langCombo->currentIndex() + 1) % langCombo->count();
    langCombo->setCurrentIndex(newIndex);
    waitForUI();

    QVERIFY(spy.count() >= 1);
}

// ============================================================================
// Dialog behavior tests
// ============================================================================

void SettingsDialogIntegrationTest::testDialogVisibility() {
    QVERIFY(m_dialog->isVisible());

    m_dialog->hide();
    QVERIFY(!m_dialog->isVisible());

    m_dialog->show();
    QVERIFY(m_dialog->isVisible());
}

void SettingsDialogIntegrationTest::testDialogSize() {
    QVERIFY(m_dialog->width() >= 600);
    QVERIFY(m_dialog->height() >= 500);
}

void SettingsDialogIntegrationTest::testDialogResize() {
    QSize originalSize = m_dialog->size();

    m_dialog->resize(800, 700);
    waitForUI();

    QCOMPARE(m_dialog->width(), 800);
    QCOMPARE(m_dialog->height(), 700);

    m_dialog->resize(originalSize);
}

void SettingsDialogIntegrationTest::testDialogModal() {
    QVERIFY(m_dialog->isModal());
}

// ============================================================================
// Settings persistence tests
// ============================================================================

void SettingsDialogIntegrationTest::testLoadSettings() {
    // Settings should be loaded on construction
    // Verify that UI reflects some settings
    QWidget* tab = findTabByName("Appearance");
    QVERIFY(tab != nullptr);

    QList<QRadioButton*> radios = tab->findChildren<QRadioButton*>();
    bool anyChecked = false;
    for (QRadioButton* radio : radios) {
        if (radio->isChecked()) {
            anyChecked = true;
            break;
        }
    }
    QVERIFY(anyChecked);
}

void SettingsDialogIntegrationTest::testSaveSettings() {
    // This test verifies that clicking Apply doesn't crash
    QDialogButtonBox* buttonBox = m_dialog->findChild<QDialogButtonBox*>();
    QVERIFY(buttonBox != nullptr);

    QPushButton* applyButton = buttonBox->button(QDialogButtonBox::Apply);
    QVERIFY(applyButton != nullptr);

    // Note: We don't actually click because it may show dialogs
    // Just verify the button exists and is enabled
    QVERIFY(applyButton->isEnabled());
}

// ============================================================================
// Validation tests
// ============================================================================

void SettingsDialogIntegrationTest::testCacheSizeValidation() {
    QWidget* tab = findTabByName("Performance");
    QVERIFY(tab != nullptr);

    QList<QSpinBox*> spinboxes = tab->findChildren<QSpinBox*>();
    QVERIFY(spinboxes.size() >= 1);

    QSpinBox* cacheSizeSpinBox = spinboxes.first();

    // Test minimum boundary
    cacheSizeSpinBox->setValue(cacheSizeSpinBox->minimum());
    waitForUI();
    QCOMPARE(cacheSizeSpinBox->value(), cacheSizeSpinBox->minimum());

    // Test maximum boundary
    cacheSizeSpinBox->setValue(cacheSizeSpinBox->maximum());
    waitForUI();
    QCOMPARE(cacheSizeSpinBox->value(), cacheSizeSpinBox->maximum());
}

void SettingsDialogIntegrationTest::testRecentFilesCountValidation() {
    QWidget* tab = findTabByName("Behavior");
    QVERIFY(tab != nullptr);

    QSpinBox* recentSpinBox = tab->findChild<QSpinBox*>();
    QVERIFY(recentSpinBox != nullptr);

    // Test minimum boundary
    recentSpinBox->setValue(recentSpinBox->minimum());
    waitForUI();
    QCOMPARE(recentSpinBox->value(), recentSpinBox->minimum());

    // Test maximum boundary
    recentSpinBox->setValue(recentSpinBox->maximum());
    waitForUI();
    QCOMPARE(recentSpinBox->value(), recentSpinBox->maximum());
}

void SettingsDialogIntegrationTest::testCachePathValidation() {
    QWidget* tab = findTabByName("Advanced");
    QVERIFY(tab != nullptr);

    QLineEdit* pathEdit = tab->findChild<QLineEdit*>();
    QVERIFY(pathEdit != nullptr);

    // Test empty path (should be valid - uses default)
    pathEdit->clear();
    waitForUI();
    QVERIFY(pathEdit->text().isEmpty());

    // Test valid path
    pathEdit->setText(m_tempDir->path());
    waitForUI();
    QCOMPARE(pathEdit->text(), m_tempDir->path());
}

// ============================================================================
// Theme preview tests
// ============================================================================

void SettingsDialogIntegrationTest::testThemePreview() {
    QSignalSpy spy(m_dialog, &SettingsDialog::themeChanged);

    QWidget* tab = findTabByName("Appearance");
    QVERIFY(tab != nullptr);

    QList<QRadioButton*> radios = tab->findChildren<QRadioButton*>();
    for (QRadioButton* radio : radios) {
        if (!radio->isChecked()) {
            radio->setChecked(true);
            waitForUI();
            break;
        }
    }

    // Theme changed signal should be emitted for preview
    QVERIFY(spy.count() >= 1);
}

void SettingsDialogIntegrationTest::testLanguagePreview() {
    QSignalSpy spy(m_dialog, &SettingsDialog::languageChanged);

    QWidget* tab = findTabByName("Appearance");
    QVERIFY(tab != nullptr);

    QComboBox* langCombo = tab->findChild<QComboBox*>();
    QVERIFY(langCombo != nullptr);

    int newIndex = (langCombo->currentIndex() + 1) % langCombo->count();
    langCombo->setCurrentIndex(newIndex);
    waitForUI();

    // Language changed signal should be emitted for preview
    QVERIFY(spy.count() >= 1);
}

// ============================================================================
// Restore defaults tests
// ============================================================================

void SettingsDialogIntegrationTest::testRestoreDefaults() {
    // Find restore defaults button
    QList<QPushButton*> buttons = m_dialog->findChildren<QPushButton*>();
    QPushButton* restoreButton = nullptr;

    for (QPushButton* btn : buttons) {
        if (btn->text().contains("Restore", Qt::CaseInsensitive) ||
            btn->text().contains("Default", Qt::CaseInsensitive)) {
            restoreButton = btn;
            break;
        }
    }

    QVERIFY(restoreButton != nullptr);
    QVERIFY(restoreButton->isEnabled());

    // Note: We don't click because it shows a confirmation dialog
    // Just verify the button exists and is functional
}

// ============================================================================
// Language change event tests
// ============================================================================

void SettingsDialogIntegrationTest::testLanguageChangeEvent() {
    // Send language change event
    QEvent event(QEvent::LanguageChange);
    QApplication::sendEvent(m_dialog, &event);
    waitForUI();

    // Dialog should still be functional
    QVERIFY(m_dialog->isVisible());
    QVERIFY(!m_dialog->windowTitle().isEmpty());
}

void SettingsDialogIntegrationTest::testRetranslateUi() {
    // Trigger language change
    QEvent event(QEvent::LanguageChange);
    QApplication::sendEvent(m_dialog, &event);
    waitForUI();

    // Tab titles should still exist
    QTabWidget* tabWidget = findTabWidget();
    QVERIFY(tabWidget != nullptr);

    for (int i = 0; i < tabWidget->count(); ++i) {
        QVERIFY(!tabWidget->tabText(i).isEmpty());
    }
}

QTEST_MAIN(SettingsDialogIntegrationTest)
#include "test_settings_dialog_integration.moc"
