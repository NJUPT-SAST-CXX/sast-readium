#pragma once

#include <QWidget>

// Forward declarations
class QVBoxLayout;
class ElaToggleSwitch;
class ElaSlider;
class ElaComboBox;
class ElaSpinBox;
class ElaPushButton;
class ElaText;

/**
 * @brief Widget for search settings configuration
 *
 * Provides UI for configuring search behavior including:
 * - Basic search options (case sensitivity, whole words, regex)
 * - Advanced options (fuzzy search, max results)
 * - Performance settings (cache, incremental search)
 * - Display options (highlight color, context length)
 */
class SearchSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit SearchSettingsWidget(QWidget* parent = nullptr);
    ~SearchSettingsWidget() override;

    void loadSettings();
    void saveSettings();
    void resetToDefaults();

signals:
    void settingsChanged();

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onFuzzySearchToggled(bool enabled);
    void onCacheToggled(bool enabled);
    void onIncrementalSearchToggled(bool enabled);

private:
    void setupUi();
    void retranslateUi();
    void updateControlsState();

    // UI Components
    QVBoxLayout* m_mainLayout;

    // Basic options
    ElaToggleSwitch* m_caseSensitiveSwitch;
    ElaToggleSwitch* m_wholeWordsSwitch;
    ElaToggleSwitch* m_regexSwitch;

    // Advanced options
    ElaToggleSwitch* m_fuzzySearchSwitch;
    ElaSpinBox* m_fuzzyThresholdSpin;
    ElaSpinBox* m_maxResultsSpin;
    ElaSpinBox* m_contextLengthSpin;

    // Performance
    ElaToggleSwitch* m_cacheEnabledSwitch;
    ElaSpinBox* m_maxCacheMemorySpin;
    ElaToggleSwitch* m_incrementalSearchSwitch;
    ElaSpinBox* m_incrementalDelaySpin;
    ElaSpinBox* m_searchTimeoutSpin;

    // Display
    ElaComboBox* m_highlightColorCombo;
    ElaToggleSwitch* m_showContextSwitch;
    ElaToggleSwitch* m_highlightAllMatchesSwitch;

    // Background processing
    ElaToggleSwitch* m_backgroundProcessingSwitch;
    ElaSpinBox* m_maxThreadsSpin;
};
