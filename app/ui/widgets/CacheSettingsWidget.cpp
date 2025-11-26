#include "CacheSettingsWidget.h"

#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QSettings>
#include <QStandardPaths>
#include <QVBoxLayout>

#include "ElaContentDialog.h"
#include "ElaLineEdit.h"
#include "ElaProgressBar.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaSlider.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "ElaToggleSwitch.h"

#include "logging/SimpleLogging.h"

CacheSettingsWidget::CacheSettingsWidget(QWidget* parent)
    : QWidget(parent),
      m_mainLayout(nullptr),
      m_enableCacheSwitch(nullptr),
      m_cacheSizeSlider(nullptr),
      m_cacheSizeLabel(nullptr),
      m_cachePathEdit(nullptr),
      m_browsePathBtn(nullptr),
      m_preloadPagesSwitch(nullptr),
      m_preloadCountSpin(nullptr),
      m_preloadThumbnailsSwitch(nullptr),
      m_adaptivePreloadSwitch(nullptr),
      m_memoryLimitSpin(nullptr),
      m_gpuCacheSwitch(nullptr),
      m_cacheUsageBar(nullptr),
      m_cacheUsageLabel(nullptr),
      m_thumbnailCountLabel(nullptr),
      m_pageCountLabel(nullptr),
      m_clearCacheBtn(nullptr),
      m_clearThumbnailsBtn(nullptr),
      m_clearPageCacheBtn(nullptr) {
    setupUi();
    loadSettings();
    refreshCacheStats();
}

CacheSettingsWidget::~CacheSettingsWidget() = default;

void CacheSettingsWidget::setupUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(16);

    // Basic Settings Section
    auto* basicArea = new ElaScrollPageArea(this);
    auto* basicLayout = new QVBoxLayout(basicArea);
    basicLayout->setContentsMargins(16, 12, 16, 12);

    auto* basicTitle = new ElaText(tr("Cache Settings"), this);
    basicTitle->setTextPixelSize(14);
    basicLayout->addWidget(basicTitle);

    auto* enableRow = new QHBoxLayout();
    auto* enableLabel = new ElaText(tr("Enable caching"), this);
    enableRow->addWidget(enableLabel);
    enableRow->addStretch();
    m_enableCacheSwitch = new ElaToggleSwitch(this);
    enableRow->addWidget(m_enableCacheSwitch);
    basicLayout->addLayout(enableRow);

    auto* sizeRow = new QHBoxLayout();
    auto* sizeLabel = new ElaText(tr("Cache size limit:"), this);
    sizeRow->addWidget(sizeLabel);
    m_cacheSizeSlider = new ElaSlider(Qt::Horizontal, this);
    m_cacheSizeSlider->setRange(50, 2000);
    m_cacheSizeSlider->setValue(500);
    sizeRow->addWidget(m_cacheSizeSlider, 1);
    m_cacheSizeLabel = new ElaText("500 MB", this);
    m_cacheSizeLabel->setFixedWidth(60);
    sizeRow->addWidget(m_cacheSizeLabel);
    basicLayout->addLayout(sizeRow);

    auto* pathRow = new QHBoxLayout();
    auto* pathLabel = new ElaText(tr("Cache location:"), this);
    pathRow->addWidget(pathLabel);
    m_cachePathEdit = new ElaLineEdit(this);
    m_cachePathEdit->setPlaceholderText(
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    pathRow->addWidget(m_cachePathEdit, 1);
    m_browsePathBtn = new ElaPushButton(tr("Browse..."), this);
    pathRow->addWidget(m_browsePathBtn);
    basicLayout->addLayout(pathRow);

    m_mainLayout->addWidget(basicArea);

    // Preloading Section
    auto* preloadArea = new ElaScrollPageArea(this);
    auto* preloadLayout = new QVBoxLayout(preloadArea);
    preloadLayout->setContentsMargins(16, 12, 16, 12);

    auto* preloadTitle = new ElaText(tr("Preloading"), this);
    preloadTitle->setTextPixelSize(14);
    preloadLayout->addWidget(preloadTitle);

    auto* preloadPagesRow = new QHBoxLayout();
    auto* preloadPagesLabel = new ElaText(tr("Preload adjacent pages"), this);
    preloadPagesRow->addWidget(preloadPagesLabel);
    preloadPagesRow->addStretch();
    m_preloadPagesSwitch = new ElaToggleSwitch(this);
    preloadPagesRow->addWidget(m_preloadPagesSwitch);
    preloadLayout->addLayout(preloadPagesRow);

    auto* countRow = new QHBoxLayout();
    auto* countLabel = new ElaText(tr("Pages to preload:"), this);
    countRow->addWidget(countLabel);
    m_preloadCountSpin = new ElaSpinBox(this);
    m_preloadCountSpin->setRange(1, 10);
    m_preloadCountSpin->setValue(2);
    countRow->addWidget(m_preloadCountSpin);
    countRow->addStretch();
    preloadLayout->addLayout(countRow);

    auto* thumbRow = new QHBoxLayout();
    auto* thumbLabel = new ElaText(tr("Preload thumbnails"), this);
    thumbRow->addWidget(thumbLabel);
    thumbRow->addStretch();
    m_preloadThumbnailsSwitch = new ElaToggleSwitch(this);
    thumbRow->addWidget(m_preloadThumbnailsSwitch);
    preloadLayout->addLayout(thumbRow);

    auto* adaptiveRow = new QHBoxLayout();
    auto* adaptiveLabel = new ElaText(tr("Adaptive preloading"), this);
    adaptiveRow->addWidget(adaptiveLabel);
    adaptiveRow->addStretch();
    m_adaptivePreloadSwitch = new ElaToggleSwitch(this);
    adaptiveRow->addWidget(m_adaptivePreloadSwitch);
    preloadLayout->addLayout(adaptiveRow);

    m_mainLayout->addWidget(preloadArea);

    // Memory Section
    auto* memoryArea = new ElaScrollPageArea(this);
    auto* memoryLayout = new QVBoxLayout(memoryArea);
    memoryLayout->setContentsMargins(16, 12, 16, 12);

    auto* memoryTitle = new ElaText(tr("Memory"), this);
    memoryTitle->setTextPixelSize(14);
    memoryLayout->addWidget(memoryTitle);

    auto* memLimitRow = new QHBoxLayout();
    auto* memLimitLabel = new ElaText(tr("Memory limit (MB):"), this);
    memLimitRow->addWidget(memLimitLabel);
    m_memoryLimitSpin = new ElaSpinBox(this);
    m_memoryLimitSpin->setRange(100, 4000);
    m_memoryLimitSpin->setValue(512);
    memLimitRow->addWidget(m_memoryLimitSpin);
    memLimitRow->addStretch();
    memoryLayout->addLayout(memLimitRow);

    auto* gpuRow = new QHBoxLayout();
    auto* gpuLabel = new ElaText(tr("GPU cache acceleration"), this);
    gpuRow->addWidget(gpuLabel);
    gpuRow->addStretch();
    m_gpuCacheSwitch = new ElaToggleSwitch(this);
    gpuRow->addWidget(m_gpuCacheSwitch);
    memoryLayout->addLayout(gpuRow);

    m_mainLayout->addWidget(memoryArea);

    // Statistics Section
    auto* statsArea = new ElaScrollPageArea(this);
    auto* statsLayout = new QVBoxLayout(statsArea);
    statsLayout->setContentsMargins(16, 12, 16, 12);

    auto* statsTitle = new ElaText(tr("Cache Statistics"), this);
    statsTitle->setTextPixelSize(14);
    statsLayout->addWidget(statsTitle);

    auto* usageRow = new QHBoxLayout();
    auto* usageLabel = new ElaText(tr("Cache usage:"), this);
    usageRow->addWidget(usageLabel);
    m_cacheUsageBar = new ElaProgressBar(this);
    m_cacheUsageBar->setMinimum(0);
    m_cacheUsageBar->setMaximum(100);
    m_cacheUsageBar->setValue(0);
    usageRow->addWidget(m_cacheUsageBar, 1);
    m_cacheUsageLabel = new ElaText("0 / 500 MB", this);
    m_cacheUsageLabel->setFixedWidth(100);
    usageRow->addWidget(m_cacheUsageLabel);
    statsLayout->addLayout(usageRow);

    m_thumbnailCountLabel = new ElaText(tr("Cached thumbnails: 0"), this);
    statsLayout->addWidget(m_thumbnailCountLabel);

    m_pageCountLabel = new ElaText(tr("Cached pages: 0"), this);
    statsLayout->addWidget(m_pageCountLabel);

    m_mainLayout->addWidget(statsArea);

    // Actions Section
    auto* actionsArea = new ElaScrollPageArea(this);
    actionsArea->setFixedHeight(60);
    auto* actionsLayout = new QHBoxLayout(actionsArea);
    actionsLayout->setContentsMargins(16, 12, 16, 12);

    m_clearCacheBtn = new ElaPushButton(tr("Clear All Cache"), this);
    actionsLayout->addWidget(m_clearCacheBtn);

    m_clearThumbnailsBtn = new ElaPushButton(tr("Clear Thumbnails"), this);
    actionsLayout->addWidget(m_clearThumbnailsBtn);

    m_clearPageCacheBtn = new ElaPushButton(tr("Clear Page Cache"), this);
    actionsLayout->addWidget(m_clearPageCacheBtn);

    actionsLayout->addStretch();

    m_mainLayout->addWidget(actionsArea);
    m_mainLayout->addStretch();

    // Connect signals
    connect(m_enableCacheSwitch, &ElaToggleSwitch::toggled, this,
            &CacheSettingsWidget::onCacheEnabledToggled);
    connect(m_cacheSizeSlider, &ElaSlider::valueChanged, this,
            &CacheSettingsWidget::onCacheSizeChanged);
    connect(m_browsePathBtn, &ElaPushButton::clicked, this,
            &CacheSettingsWidget::onBrowseCachePath);
    connect(m_clearCacheBtn, &ElaPushButton::clicked, this,
            &CacheSettingsWidget::onClearCache);
    connect(m_clearThumbnailsBtn, &ElaPushButton::clicked, this,
            &CacheSettingsWidget::onClearThumbnails);
    connect(m_clearPageCacheBtn, &ElaPushButton::clicked, this,
            &CacheSettingsWidget::onClearPageCache);
}

void CacheSettingsWidget::loadSettings() {
    QSettings settings("SAST", "Readium");
    settings.beginGroup("Cache");

    m_enableCacheSwitch->setIsToggled(settings.value("enabled", true).toBool());
    m_cacheSizeSlider->setValue(settings.value("size_limit", 500).toInt());
    m_cachePathEdit->setText(settings.value("custom_path", "").toString());
    m_preloadPagesSwitch->setIsToggled(
        settings.value("preload_pages", true).toBool());
    m_preloadCountSpin->setValue(settings.value("preload_count", 2).toInt());
    m_preloadThumbnailsSwitch->setIsToggled(
        settings.value("preload_thumbnails", true).toBool());
    m_adaptivePreloadSwitch->setIsToggled(
        settings.value("adaptive_preload", true).toBool());
    m_memoryLimitSpin->setValue(settings.value("memory_limit", 512).toInt());
    m_gpuCacheSwitch->setIsToggled(settings.value("gpu_cache", true).toBool());

    settings.endGroup();
    updateControlsState();
}

void CacheSettingsWidget::saveSettings() {
    QSettings settings("SAST", "Readium");
    settings.beginGroup("Cache");

    settings.setValue("enabled", m_enableCacheSwitch->getIsToggled());
    settings.setValue("size_limit", m_cacheSizeSlider->value());
    settings.setValue("custom_path", m_cachePathEdit->text());
    settings.setValue("preload_pages", m_preloadPagesSwitch->getIsToggled());
    settings.setValue("preload_count", m_preloadCountSpin->value());
    settings.setValue("preload_thumbnails",
                      m_preloadThumbnailsSwitch->getIsToggled());
    settings.setValue("adaptive_preload",
                      m_adaptivePreloadSwitch->getIsToggled());
    settings.setValue("memory_limit", m_memoryLimitSpin->value());
    settings.setValue("gpu_cache", m_gpuCacheSwitch->getIsToggled());

    settings.endGroup();
    emit settingsChanged();
}

void CacheSettingsWidget::resetToDefaults() {
    m_enableCacheSwitch->setIsToggled(true);
    m_cacheSizeSlider->setValue(500);
    m_cachePathEdit->clear();
    m_preloadPagesSwitch->setIsToggled(true);
    m_preloadCountSpin->setValue(2);
    m_preloadThumbnailsSwitch->setIsToggled(true);
    m_adaptivePreloadSwitch->setIsToggled(true);
    m_memoryLimitSpin->setValue(512);
    m_gpuCacheSwitch->setIsToggled(true);
    updateControlsState();
    emit settingsChanged();
}

void CacheSettingsWidget::refreshCacheStats() {
    // This would query actual cache statistics
    // For now, show placeholder values
    updateCacheUsageDisplay();
}

void CacheSettingsWidget::onCacheEnabledToggled(bool enabled) {
    Q_UNUSED(enabled)
    updateControlsState();
    emit settingsChanged();
}

void CacheSettingsWidget::onCacheSizeChanged(int value) {
    m_cacheSizeLabel->setText(QString("%1 MB").arg(value));
    updateCacheUsageDisplay();
    emit settingsChanged();
}

void CacheSettingsWidget::onBrowseCachePath() {
    QString dir = QFileDialog::getExistingDirectory(
        this, tr("Select Cache Directory"), m_cachePathEdit->text());
    if (!dir.isEmpty()) {
        m_cachePathEdit->setText(dir);
        emit settingsChanged();
    }
}

void CacheSettingsWidget::onClearCache() {
    auto* dialog = new ElaContentDialog(this);
    dialog->setWindowTitle(tr("Clear Cache"));

    auto* centralWidget = new QWidget(dialog);
    auto* layout = new QVBoxLayout(centralWidget);
    auto* label = new ElaText(
        tr("Are you sure you want to clear all cached data?"), centralWidget);
    layout->addWidget(label);
    dialog->setCentralWidget(centralWidget);

    dialog->setLeftButtonText(tr("Cancel"));
    dialog->setRightButtonText(tr("Clear"));

    connect(dialog, &ElaContentDialog::rightButtonClicked, this,
            [this, dialog]() {
                // Clear cache implementation
                refreshCacheStats();
                dialog->close();
            });
    connect(dialog, &ElaContentDialog::leftButtonClicked, dialog,
            &ElaContentDialog::close);
    dialog->exec();
    dialog->deleteLater();
}

void CacheSettingsWidget::onClearThumbnails() {
    auto* dialog = new ElaContentDialog(this);
    dialog->setWindowTitle(tr("Clear Thumbnails"));

    auto* centralWidget = new QWidget(dialog);
    auto* layout = new QVBoxLayout(centralWidget);
    auto* label =
        new ElaText(tr("Are you sure you want to clear all cached thumbnails?"),
                    centralWidget);
    layout->addWidget(label);
    dialog->setCentralWidget(centralWidget);

    dialog->setLeftButtonText(tr("Cancel"));
    dialog->setRightButtonText(tr("Clear"));

    connect(dialog, &ElaContentDialog::rightButtonClicked, this,
            [this, dialog]() {
                // Clear thumbnails implementation
                refreshCacheStats();
                dialog->close();
            });
    connect(dialog, &ElaContentDialog::leftButtonClicked, dialog,
            &ElaContentDialog::close);
    dialog->exec();
    dialog->deleteLater();
}

void CacheSettingsWidget::onClearPageCache() {
    auto* dialog = new ElaContentDialog(this);
    dialog->setWindowTitle(tr("Clear Page Cache"));

    auto* centralWidget = new QWidget(dialog);
    auto* layout = new QVBoxLayout(centralWidget);
    auto* label = new ElaText(
        tr("Are you sure you want to clear all cached pages?"), centralWidget);
    layout->addWidget(label);
    dialog->setCentralWidget(centralWidget);

    dialog->setLeftButtonText(tr("Cancel"));
    dialog->setRightButtonText(tr("Clear"));

    connect(dialog, &ElaContentDialog::rightButtonClicked, this,
            [this, dialog]() {
                // Clear page cache implementation
                refreshCacheStats();
                dialog->close();
            });
    connect(dialog, &ElaContentDialog::leftButtonClicked, dialog,
            &ElaContentDialog::close);
    dialog->exec();
    dialog->deleteLater();
}

void CacheSettingsWidget::updateControlsState() {
    bool enabled = m_enableCacheSwitch->getIsToggled();
    m_cacheSizeSlider->setEnabled(enabled);
    m_cachePathEdit->setEnabled(enabled);
    m_browsePathBtn->setEnabled(enabled);
    m_preloadPagesSwitch->setEnabled(enabled);
    m_preloadCountSpin->setEnabled(enabled &&
                                   m_preloadPagesSwitch->getIsToggled());
    m_preloadThumbnailsSwitch->setEnabled(enabled);
    m_adaptivePreloadSwitch->setEnabled(enabled);
    m_memoryLimitSpin->setEnabled(enabled);
    m_gpuCacheSwitch->setEnabled(enabled);
    m_clearCacheBtn->setEnabled(enabled);
    m_clearThumbnailsBtn->setEnabled(enabled);
    m_clearPageCacheBtn->setEnabled(enabled);
}

void CacheSettingsWidget::updateCacheUsageDisplay() {
    int maxSize = m_cacheSizeSlider->value();
    // Placeholder - actual implementation would query real cache size
    int usedSize = 0;
    int percentage = maxSize > 0 ? (usedSize * 100 / maxSize) : 0;

    m_cacheUsageBar->setValue(percentage);
    m_cacheUsageLabel->setText(
        QString("%1 / %2 MB").arg(usedSize).arg(maxSize));
    m_thumbnailCountLabel->setText(tr("Cached thumbnails: %1").arg(0));
    m_pageCountLabel->setText(tr("Cached pages: %1").arg(0));
}

void CacheSettingsWidget::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void CacheSettingsWidget::retranslateUi() {
    // Retranslation would be implemented here
}
