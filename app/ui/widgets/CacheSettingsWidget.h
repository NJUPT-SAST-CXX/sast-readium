#pragma once

#include <QWidget>

// Forward declarations
class QVBoxLayout;
class ElaToggleSwitch;
class ElaSlider;
class ElaSpinBox;
class ElaLineEdit;
class ElaPushButton;
class ElaProgressBar;
class ElaText;

/**
 * @brief Widget for cache settings configuration
 *
 * Provides UI for configuring cache behavior including:
 * - Cache size limits
 * - Cache location
 * - Preloading settings
 * - Cache statistics and management
 */
class CacheSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit CacheSettingsWidget(QWidget* parent = nullptr);
    ~CacheSettingsWidget() override;

    void loadSettings();
    void saveSettings();
    void resetToDefaults();
    void refreshCacheStats();

signals:
    void settingsChanged();

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onCacheEnabledToggled(bool enabled);
    void onCacheSizeChanged(int value);
    void onBrowseCachePath();
    void onClearCache();
    void onClearThumbnails();
    void onClearPageCache();

private:
    void setupUi();
    void retranslateUi();
    void updateControlsState();
    void updateCacheUsageDisplay();

    // UI Components
    QVBoxLayout* m_mainLayout;

    // Basic settings
    ElaToggleSwitch* m_enableCacheSwitch;
    ElaSlider* m_cacheSizeSlider;
    ElaText* m_cacheSizeLabel;
    ElaLineEdit* m_cachePathEdit;
    ElaPushButton* m_browsePathBtn;

    // Preloading
    ElaToggleSwitch* m_preloadPagesSwitch;
    ElaSpinBox* m_preloadCountSpin;
    ElaToggleSwitch* m_preloadThumbnailsSwitch;
    ElaToggleSwitch* m_adaptivePreloadSwitch;

    // Memory
    ElaSpinBox* m_memoryLimitSpin;
    ElaToggleSwitch* m_gpuCacheSwitch;

    // Statistics
    ElaProgressBar* m_cacheUsageBar;
    ElaText* m_cacheUsageLabel;
    ElaText* m_thumbnailCountLabel;
    ElaText* m_pageCountLabel;

    // Actions
    ElaPushButton* m_clearCacheBtn;
    ElaPushButton* m_clearThumbnailsBtn;
    ElaPushButton* m_clearPageCacheBtn;
};
