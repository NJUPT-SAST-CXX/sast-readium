#include "LoggingSettingsWidget.h"

#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

#include "ElaComboBox.h"
#include "ElaContentDialog.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "ElaToggleSwitch.h"

#include "logging/SimpleLogging.h"

LoggingSettingsWidget::LoggingSettingsWidget(QWidget* parent)
    : QWidget(parent),
      m_mainLayout(nullptr),
      m_globalLevelCombo(nullptr),
      m_asyncLoggingSwitch(nullptr),
      m_flushIntervalSpin(nullptr),
      m_consoleLoggingSwitch(nullptr),
      m_coloredOutputSwitch(nullptr),
      m_fileLoggingSwitch(nullptr),
      m_logPathEdit(nullptr),
      m_browsePathBtn(nullptr),
      m_maxFileSizeSpin(nullptr),
      m_maxFilesSpin(nullptr),
      m_rotateOnStartupSwitch(nullptr),
      m_perfLoggingSwitch(nullptr),
      m_perfThresholdSpin(nullptr),
      m_memoryLoggingSwitch(nullptr),
      m_threadIdSwitch(nullptr),
      m_sourceLocationSwitch(nullptr),
      m_openLogFolderBtn(nullptr),
      m_clearLogsBtn(nullptr) {
    setupUi();
    loadSettings();
}

LoggingSettingsWidget::~LoggingSettingsWidget() = default;

void LoggingSettingsWidget::setupUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(16);

    // Global Settings Section
    auto* globalArea = new ElaScrollPageArea(this);
    auto* globalLayout = new QVBoxLayout(globalArea);
    globalLayout->setContentsMargins(16, 12, 16, 12);

    auto* globalTitle = new ElaText(tr("Global Settings"), this);
    globalTitle->setTextPixelSize(14);
    globalLayout->addWidget(globalTitle);

    auto* levelRow = new QHBoxLayout();
    auto* levelLabel = new ElaText(tr("Log Level:"), this);
    levelRow->addWidget(levelLabel);
    m_globalLevelCombo = new ElaComboBox(this);
    m_globalLevelCombo->addItem(tr("Trace"), "trace");
    m_globalLevelCombo->addItem(tr("Debug"), "debug");
    m_globalLevelCombo->addItem(tr("Info"), "info");
    m_globalLevelCombo->addItem(tr("Warning"), "warning");
    m_globalLevelCombo->addItem(tr("Error"), "error");
    m_globalLevelCombo->addItem(tr("Critical"), "critical");
    m_globalLevelCombo->setCurrentIndex(2);  // Info
    levelRow->addWidget(m_globalLevelCombo);
    levelRow->addStretch();
    globalLayout->addLayout(levelRow);

    auto* asyncRow = new QHBoxLayout();
    auto* asyncLabel = new ElaText(tr("Asynchronous logging"), this);
    asyncRow->addWidget(asyncLabel);
    asyncRow->addStretch();
    m_asyncLoggingSwitch = new ElaToggleSwitch(this);
    asyncRow->addWidget(m_asyncLoggingSwitch);
    globalLayout->addLayout(asyncRow);

    auto* flushRow = new QHBoxLayout();
    auto* flushLabel = new ElaText(tr("Flush interval (seconds):"), this);
    flushRow->addWidget(flushLabel);
    m_flushIntervalSpin = new ElaSpinBox(this);
    m_flushIntervalSpin->setRange(1, 60);
    m_flushIntervalSpin->setValue(5);
    flushRow->addWidget(m_flushIntervalSpin);
    flushRow->addStretch();
    globalLayout->addLayout(flushRow);

    m_mainLayout->addWidget(globalArea);

    // Console Logging Section
    auto* consoleArea = new ElaScrollPageArea(this);
    auto* consoleLayout = new QVBoxLayout(consoleArea);
    consoleLayout->setContentsMargins(16, 12, 16, 12);

    auto* consoleTitle = new ElaText(tr("Console Logging"), this);
    consoleTitle->setTextPixelSize(14);
    consoleLayout->addWidget(consoleTitle);

    auto* consoleRow = new QHBoxLayout();
    auto* consoleLabel = new ElaText(tr("Enable console logging"), this);
    consoleRow->addWidget(consoleLabel);
    consoleRow->addStretch();
    m_consoleLoggingSwitch = new ElaToggleSwitch(this);
    consoleRow->addWidget(m_consoleLoggingSwitch);
    consoleLayout->addLayout(consoleRow);

    auto* colorRow = new QHBoxLayout();
    auto* colorLabel = new ElaText(tr("Colored output"), this);
    colorRow->addWidget(colorLabel);
    colorRow->addStretch();
    m_coloredOutputSwitch = new ElaToggleSwitch(this);
    colorRow->addWidget(m_coloredOutputSwitch);
    consoleLayout->addLayout(colorRow);

    m_mainLayout->addWidget(consoleArea);

    // File Logging Section
    auto* fileArea = new ElaScrollPageArea(this);
    auto* fileLayout = new QVBoxLayout(fileArea);
    fileLayout->setContentsMargins(16, 12, 16, 12);

    auto* fileTitle = new ElaText(tr("File Logging"), this);
    fileTitle->setTextPixelSize(14);
    fileLayout->addWidget(fileTitle);

    auto* fileRow = new QHBoxLayout();
    auto* fileLabel = new ElaText(tr("Enable file logging"), this);
    fileRow->addWidget(fileLabel);
    fileRow->addStretch();
    m_fileLoggingSwitch = new ElaToggleSwitch(this);
    fileRow->addWidget(m_fileLoggingSwitch);
    fileLayout->addLayout(fileRow);

    auto* pathRow = new QHBoxLayout();
    auto* pathLabel = new ElaText(tr("Log directory:"), this);
    pathRow->addWidget(pathLabel);
    m_logPathEdit = new ElaLineEdit(this);
    m_logPathEdit->setPlaceholderText(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
        "/logs");
    pathRow->addWidget(m_logPathEdit, 1);
    m_browsePathBtn = new ElaPushButton(tr("Browse..."), this);
    pathRow->addWidget(m_browsePathBtn);
    fileLayout->addLayout(pathRow);

    auto* sizeRow = new QHBoxLayout();
    auto* sizeLabel = new ElaText(tr("Max file size (MB):"), this);
    sizeRow->addWidget(sizeLabel);
    m_maxFileSizeSpin = new ElaSpinBox(this);
    m_maxFileSizeSpin->setRange(1, 100);
    m_maxFileSizeSpin->setValue(10);
    sizeRow->addWidget(m_maxFileSizeSpin);
    sizeRow->addStretch();
    fileLayout->addLayout(sizeRow);

    auto* filesRow = new QHBoxLayout();
    auto* filesLabel = new ElaText(tr("Max log files:"), this);
    filesRow->addWidget(filesLabel);
    m_maxFilesSpin = new ElaSpinBox(this);
    m_maxFilesSpin->setRange(1, 20);
    m_maxFilesSpin->setValue(5);
    filesRow->addWidget(m_maxFilesSpin);
    filesRow->addStretch();
    fileLayout->addLayout(filesRow);

    auto* rotateRow = new QHBoxLayout();
    auto* rotateLabel = new ElaText(tr("Rotate on startup"), this);
    rotateRow->addWidget(rotateLabel);
    rotateRow->addStretch();
    m_rotateOnStartupSwitch = new ElaToggleSwitch(this);
    rotateRow->addWidget(m_rotateOnStartupSwitch);
    fileLayout->addLayout(rotateRow);

    m_mainLayout->addWidget(fileArea);

    // Advanced Section
    auto* advancedArea = new ElaScrollPageArea(this);
    auto* advancedLayout = new QVBoxLayout(advancedArea);
    advancedLayout->setContentsMargins(16, 12, 16, 12);

    auto* advancedTitle = new ElaText(tr("Advanced"), this);
    advancedTitle->setTextPixelSize(14);
    advancedLayout->addWidget(advancedTitle);

    auto* perfRow = new QHBoxLayout();
    auto* perfLabel = new ElaText(tr("Performance logging"), this);
    perfRow->addWidget(perfLabel);
    perfRow->addStretch();
    m_perfLoggingSwitch = new ElaToggleSwitch(this);
    perfRow->addWidget(m_perfLoggingSwitch);
    advancedLayout->addLayout(perfRow);

    auto* thresholdRow = new QHBoxLayout();
    auto* thresholdLabel = new ElaText(tr("Performance threshold (ms):"), this);
    thresholdRow->addWidget(thresholdLabel);
    m_perfThresholdSpin = new ElaSpinBox(this);
    m_perfThresholdSpin->setRange(10, 1000);
    m_perfThresholdSpin->setValue(100);
    thresholdRow->addWidget(m_perfThresholdSpin);
    thresholdRow->addStretch();
    advancedLayout->addLayout(thresholdRow);

    auto* memoryRow = new QHBoxLayout();
    auto* memoryLabel = new ElaText(tr("Memory logging"), this);
    memoryRow->addWidget(memoryLabel);
    memoryRow->addStretch();
    m_memoryLoggingSwitch = new ElaToggleSwitch(this);
    memoryRow->addWidget(m_memoryLoggingSwitch);
    advancedLayout->addLayout(memoryRow);

    auto* threadRow = new QHBoxLayout();
    auto* threadLabel = new ElaText(tr("Include thread ID"), this);
    threadRow->addWidget(threadLabel);
    threadRow->addStretch();
    m_threadIdSwitch = new ElaToggleSwitch(this);
    threadRow->addWidget(m_threadIdSwitch);
    advancedLayout->addLayout(threadRow);

    auto* sourceRow = new QHBoxLayout();
    auto* sourceLabel = new ElaText(tr("Include source location"), this);
    sourceRow->addWidget(sourceLabel);
    sourceRow->addStretch();
    m_sourceLocationSwitch = new ElaToggleSwitch(this);
    sourceRow->addWidget(m_sourceLocationSwitch);
    advancedLayout->addLayout(sourceRow);

    m_mainLayout->addWidget(advancedArea);

    // Actions Section
    auto* actionsArea = new ElaScrollPageArea(this);
    actionsArea->setFixedHeight(60);
    auto* actionsLayout = new QHBoxLayout(actionsArea);
    actionsLayout->setContentsMargins(16, 12, 16, 12);

    m_openLogFolderBtn = new ElaPushButton(tr("Open Log Folder"), this);
    actionsLayout->addWidget(m_openLogFolderBtn);

    m_clearLogsBtn = new ElaPushButton(tr("Clear Logs"), this);
    actionsLayout->addWidget(m_clearLogsBtn);

    actionsLayout->addStretch();

    m_mainLayout->addWidget(actionsArea);
    m_mainLayout->addStretch();

    // Connect signals
    connect(m_fileLoggingSwitch, &ElaToggleSwitch::toggled, this,
            &LoggingSettingsWidget::onFileLoggingToggled);
    connect(m_browsePathBtn, &ElaPushButton::clicked, this,
            &LoggingSettingsWidget::onBrowseLogPath);
    connect(m_openLogFolderBtn, &ElaPushButton::clicked, this,
            &LoggingSettingsWidget::onOpenLogFolder);
    connect(m_clearLogsBtn, &ElaPushButton::clicked, this,
            &LoggingSettingsWidget::onClearLogs);
}

void LoggingSettingsWidget::loadSettings() {
    QSettings settings("SAST", "Readium");
    settings.beginGroup("Logging");

    int levelIndex = m_globalLevelCombo->findData(
        settings.value("global_level", "info").toString());
    if (levelIndex >= 0) {
        m_globalLevelCombo->setCurrentIndex(levelIndex);
    }

    m_asyncLoggingSwitch->setIsToggled(
        settings.value("async_logging", false).toBool());
    m_flushIntervalSpin->setValue(settings.value("flush_interval", 5).toInt());
    m_consoleLoggingSwitch->setIsToggled(
        settings.value("console_enabled", true).toBool());
    m_coloredOutputSwitch->setIsToggled(
        settings.value("colored_output", true).toBool());
    m_fileLoggingSwitch->setIsToggled(
        settings.value("file_enabled", true).toBool());
    m_logPathEdit->setText(settings.value("log_path", "").toString());
    m_maxFileSizeSpin->setValue(settings.value("max_file_size", 10).toInt());
    m_maxFilesSpin->setValue(settings.value("max_files", 5).toInt());
    m_rotateOnStartupSwitch->setIsToggled(
        settings.value("rotate_on_startup", false).toBool());
    m_perfLoggingSwitch->setIsToggled(
        settings.value("performance_logging", false).toBool());
    m_perfThresholdSpin->setValue(
        settings.value("performance_threshold", 100).toInt());
    m_memoryLoggingSwitch->setIsToggled(
        settings.value("memory_logging", false).toBool());
    m_threadIdSwitch->setIsToggled(settings.value("thread_id", false).toBool());
    m_sourceLocationSwitch->setIsToggled(
        settings.value("source_location", false).toBool());

    settings.endGroup();
    updateControlsState();
}

void LoggingSettingsWidget::saveSettings() {
    QSettings settings("SAST", "Readium");
    settings.beginGroup("Logging");

    settings.setValue("global_level",
                      m_globalLevelCombo->currentData().toString());
    settings.setValue("async_logging", m_asyncLoggingSwitch->getIsToggled());
    settings.setValue("flush_interval", m_flushIntervalSpin->value());
    settings.setValue("console_enabled",
                      m_consoleLoggingSwitch->getIsToggled());
    settings.setValue("colored_output", m_coloredOutputSwitch->getIsToggled());
    settings.setValue("file_enabled", m_fileLoggingSwitch->getIsToggled());
    settings.setValue("log_path", m_logPathEdit->text());
    settings.setValue("max_file_size", m_maxFileSizeSpin->value());
    settings.setValue("max_files", m_maxFilesSpin->value());
    settings.setValue("rotate_on_startup",
                      m_rotateOnStartupSwitch->getIsToggled());
    settings.setValue("performance_logging",
                      m_perfLoggingSwitch->getIsToggled());
    settings.setValue("performance_threshold", m_perfThresholdSpin->value());
    settings.setValue("memory_logging", m_memoryLoggingSwitch->getIsToggled());
    settings.setValue("thread_id", m_threadIdSwitch->getIsToggled());
    settings.setValue("source_location",
                      m_sourceLocationSwitch->getIsToggled());

    settings.endGroup();
    emit settingsChanged();
}

void LoggingSettingsWidget::resetToDefaults() {
    m_globalLevelCombo->setCurrentIndex(2);  // Info
    m_asyncLoggingSwitch->setIsToggled(false);
    m_flushIntervalSpin->setValue(5);
    m_consoleLoggingSwitch->setIsToggled(true);
    m_coloredOutputSwitch->setIsToggled(true);
    m_fileLoggingSwitch->setIsToggled(true);
    m_logPathEdit->clear();
    m_maxFileSizeSpin->setValue(10);
    m_maxFilesSpin->setValue(5);
    m_rotateOnStartupSwitch->setIsToggled(false);
    m_perfLoggingSwitch->setIsToggled(false);
    m_perfThresholdSpin->setValue(100);
    m_memoryLoggingSwitch->setIsToggled(false);
    m_threadIdSwitch->setIsToggled(false);
    m_sourceLocationSwitch->setIsToggled(false);
    updateControlsState();
    emit settingsChanged();
}

void LoggingSettingsWidget::onFileLoggingToggled(bool enabled) {
    Q_UNUSED(enabled)
    updateControlsState();
    emit settingsChanged();
}

void LoggingSettingsWidget::onBrowseLogPath() {
    QString dir = QFileDialog::getExistingDirectory(
        this, tr("Select Log Directory"), m_logPathEdit->text());
    if (!dir.isEmpty()) {
        m_logPathEdit->setText(dir);
        emit settingsChanged();
    }
}

void LoggingSettingsWidget::onOpenLogFolder() {
    QString path = m_logPathEdit->text();
    if (path.isEmpty()) {
        path =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
            "/logs";
    }
    QDir dir(path);
    if (dir.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void LoggingSettingsWidget::onClearLogs() {
    auto* dialog = new ElaContentDialog(this);
    dialog->setWindowTitle(tr("Clear Logs"));

    auto* centralWidget = new QWidget(dialog);
    auto* layout = new QVBoxLayout(centralWidget);
    auto* label = new ElaText(
        tr("Are you sure you want to delete all log files?"), centralWidget);
    layout->addWidget(label);
    dialog->setCentralWidget(centralWidget);

    dialog->setLeftButtonText(tr("Cancel"));
    dialog->setRightButtonText(tr("Delete"));

    connect(dialog, &ElaContentDialog::rightButtonClicked, this,
            [this, dialog]() {
                QString path = m_logPathEdit->text();
                if (path.isEmpty()) {
                    path = QStandardPaths::writableLocation(
                               QStandardPaths::AppDataLocation) +
                           "/logs";
                }
                QDir dir(path);
                if (dir.exists()) {
                    QStringList logs = dir.entryList({"*.log"}, QDir::Files);
                    for (const QString& log : logs) {
                        dir.remove(log);
                    }
                }
                dialog->close();
            });
    connect(dialog, &ElaContentDialog::leftButtonClicked, dialog,
            &ElaContentDialog::close);
    dialog->exec();
    dialog->deleteLater();
}

void LoggingSettingsWidget::updateControlsState() {
    bool fileEnabled = m_fileLoggingSwitch->getIsToggled();
    m_logPathEdit->setEnabled(fileEnabled);
    m_browsePathBtn->setEnabled(fileEnabled);
    m_maxFileSizeSpin->setEnabled(fileEnabled);
    m_maxFilesSpin->setEnabled(fileEnabled);
    m_rotateOnStartupSwitch->setEnabled(fileEnabled);

    bool consoleEnabled = m_consoleLoggingSwitch->getIsToggled();
    m_coloredOutputSwitch->setEnabled(consoleEnabled);

    bool perfEnabled = m_perfLoggingSwitch->getIsToggled();
    m_perfThresholdSpin->setEnabled(perfEnabled);
}

void LoggingSettingsWidget::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void LoggingSettingsWidget::retranslateUi() {
    // Retranslation would be implemented here
}
